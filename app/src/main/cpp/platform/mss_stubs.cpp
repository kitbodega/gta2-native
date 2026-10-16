// mss_stubs.cpp - implementacion real del subconjunto de Miles Sound System
// (AIL) que usa GTA2, sobre OpenSL ES (Android).
//
//   * Salida: PCM int16 estereo a 22050 Hz via SLAndroidSimpleBufferQueue.
//   * Sample voices (SFX): buffer RAW compartido, 8-bit unsigned mono, con
//     playback rate / volume / pan / loop block (semantica MSS).
//   * Streams (musica/voces): WAV decodificado a PCM int16 estereo en memoria
//     al abrir. Soporta WAV PCM y IMA ADPCM (format 0x11, los de GTA2).
//
// El mixer corre en el callback de OpenSL (otro hilo); el estado se protege
// con un mutex global.

#include "mss.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include "wintypes.hpp"
#include <android/log.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

const char* G2Platform_GetGameDataDir();

namespace
{

#define MAX_SAMPLES 64
#define MAX_STREAMS 8
#define FRAMES 512
#define AIL_EOF 0xFFFFFFFFu

struct Sample
{
    bool    used;
    bool    playing;
    U32     status;
    const U8* data;
    U32     len;
    double  pos;
    S32     rate;
    S32     volume;
    S32     pan;
    S32     loop_count;   // 0 = infinito, 1 = one-shot, N = N bucles
    S32     loops_left;
    U32     loop_start_byte;
    U32     loop_end_byte; // AIL_EOF = fin del buffer
};

struct Stream
{
    bool    used;
    bool    playing;
    U32     status;
    short*  pcm;          // interleaved L/R int16
    U32     frameCount;   // frames (por canal)
    U32     rate;         // rate del WAV original
    double  pos;          // frame actual
    S32     volume;
    S32     loop_count;   // 0 = infinito
    S32     playback_rate; // 0 = usar rate del WAV
};

struct Driver
{
    U32     rate;
    S32     master_volume;
    short   bufs[2][FRAMES * 2];
    int     curBuf;
};

static Sample g_samples[MAX_SAMPLES];
static Stream g_streams[MAX_STREAMS];
static Driver g_driver;
static bool  g_driverOpen = false;
static double g_outRate = 22050.0;

static pthread_mutex_t g_audioLock = PTHREAD_MUTEX_INITIALIZER;

static const int g_imaIndexTable[16] = {-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};

static const int g_imaStep[89] = {
    7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,
    80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,
    494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,
    2272,2499,2749,3024,3327,3660,4026,4428,4872,5358,5894,6484,7132,7845,
    8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,
    27086,29794,32767
};

static short clampShort(long long v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (short)v;
}

static double volGain(S32 vol)
{
    if (vol <= 0) return 0.0;
    if (vol > 127) vol = 127;
    double g = vol / 127.0;
    return g * g; // curva suave (perceptual-ish)
}

// ---------------------------------------------------------------------------
// OpenSL ES
// ---------------------------------------------------------------------------

static SLObjectItf g_engineObj = 0;
static SLEngineItf g_engine = 0;
static SLObjectItf g_outMixObj = 0;
static SLObjectItf g_playerObj = 0;
static SLPlayItf g_player = 0;
static SLAndroidSimpleBufferQueueItf g_bq = 0;

static void openSlStart(Driver* d)
{
    SLresult r;
    if (!g_engineObj)
    {
        r = slCreateEngine(&g_engineObj, 0, 0, 0, 0, 0);
        if (r != SL_RESULT_SUCCESS) { __android_log_print(ANDROID_LOG_ERROR, "GTA2-audio", "slCreateEngine %d", (int)r); g_engineObj = 0; return; }
        r = (*g_engineObj)->Realize(g_engineObj, SL_BOOLEAN_FALSE);
        if (r != SL_RESULT_SUCCESS) { __android_log_print(ANDROID_LOG_ERROR, "GTA2-audio", "engine Realize %d", (int)r); return; }
        r = (*g_engineObj)->GetInterface(g_engineObj, SL_IID_ENGINE, &g_engine);
        if (r != SL_RESULT_SUCCESS) { __android_log_print(ANDROID_LOG_ERROR, "GTA2-audio", "GetInterface engine %d", (int)r); return; }
    }

    if (!g_outMixObj)
    {
        r = (*g_engine)->CreateOutputMix(g_engine, &g_outMixObj, 0, 0, 0);
        if (r != SL_RESULT_SUCCESS) { __android_log_print(ANDROID_LOG_ERROR, "GTA2-audio", "CreateOutputMix %d", (int)r); return; }
        r = (*g_outMixObj)->Realize(g_outMixObj, SL_BOOLEAN_FALSE);
        if (r != SL_RESULT_SUCCESS) { __android_log_print(ANDROID_LOG_ERROR, "GTA2-audio", "outmix Realize %d", (int)r); return; }
    }

    if (g_playerObj) return;

    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {
        SL_DATAFORMAT_PCM, 2,
        (SLuint32)((int)g_outRate * 1000),  // samplesPerSec va en milihercios
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
        SL_BYTEORDER_LITTLEENDIAN
    };
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, g_outMixObj};
    SLDataSink audioSnk = {&loc_outmix, 0};
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};

    r = (*g_engine)->CreateAudioPlayer(g_engine, &g_playerObj, &audioSrc, &audioSnk, 1, ids, req);
    if (r != SL_RESULT_SUCCESS) { __android_log_print(ANDROID_LOG_ERROR, "GTA2-audio", "CreateAudioPlayer %d", (int)r); g_playerObj = 0; return; }
    r = (*g_playerObj)->Realize(g_playerObj, SL_BOOLEAN_FALSE);
    if (r != SL_RESULT_SUCCESS) { __android_log_print(ANDROID_LOG_ERROR, "GTA2-audio", "player Realize %d", (int)r); return; }
    r = (*g_playerObj)->GetInterface(g_playerObj, SL_IID_PLAY, &g_player);
    if (r != SL_RESULT_SUCCESS) { __android_log_print(ANDROID_LOG_ERROR, "GTA2-audio", "GetInterface play %d", (int)r); return; }
    r = (*g_playerObj)->GetInterface(g_playerObj, SL_IID_BUFFERQUEUE, &g_bq);
    if (r != SL_RESULT_SUCCESS) { __android_log_print(ANDROID_LOG_ERROR, "GTA2-audio", "GetInterface bq %d", (int)r); return; }
    (void)d;
}

static void openSlStop()
{
    if (g_playerObj)
    {
        if (g_player) (*g_player)->SetPlayState(g_player, SL_PLAYSTATE_STOPPED);
        (*g_playerObj)->Destroy(g_playerObj);
        g_playerObj = 0; g_player = 0; g_bq = 0;
    }
    if (g_outMixObj)
    {
        (*g_outMixObj)->Destroy(g_outMixObj);
        g_outMixObj = 0;
    }
    if (g_engineObj)
    {
        (*g_engineObj)->Destroy(g_engineObj);
        g_engineObj = 0; g_engine = 0;
    }
}

// ---------------------------------------------------------------------------
// Mixer (corre en el hilo de audio)
// ---------------------------------------------------------------------------

static void mixOneFrame(short* out)
{
    long long accL = 0, accR = 0;

    for (int i = 0; i < MAX_SAMPLES; i++)
    {
        Sample* s = &g_samples[i];
        if (!s->used || !s->playing || !s->data || !s->len) continue;

        double step = 2.0 * s->rate / g_outRate; // 16-bit mono: 2 bytes por muestra
        double npos = s->pos + step;

        U32 endB = (s->loop_end_byte == AIL_EOF) ? s->len : s->loop_end_byte;

        if (s->loop_count == 0)
        {
            U32 span = endB - s->loop_start_byte;
            if (span == 0 || endB <= s->loop_start_byte) span = s->len;
            if (span <= 0) { s->playing = false; s->status = SMP_DONE; continue; }
            while (npos >= (double)endB) npos -= (double)span;
        }
        else if (s->loop_count > 1)
        {
            if (npos >= (double)endB)
            {
                s->loops_left--;
                if (s->loops_left > 0)
                    npos -= (double)(endB - s->loop_start_byte);
                else
                {
                    s->playing = false;
                    s->status = SMP_DONE;
                    continue;
                }
            }
        }
        else
        {
            if (npos >= (double)s->len)
            {
                s->playing = false;
                s->status = SMP_DONE;
                continue;
            }
        }

        s->pos = npos;
        int ip = (int)npos;
        if (ip < 0) ip = 0;
        if ((U32)(ip + 1) >= s->len)
        {
            if ((U32)ip >= s->len)
            {
                s->playing = false;
                s->status = SMP_DONE;
                continue;
            }
            ip = (int)s->len - 2; // ultimo byte par valido
        }
        short v16 = (short)((U8)s->data[ip] | ((U8)s->data[ip + 1] << 8)); // S16LE mono
        double gv = volGain(s->volume);
        if (gv <= 0.0) continue;
        double ang = s->pan * (M_PI / 2.0 / 127.0);
        double gl = cos(ang) * gv;
        double gr = sin(ang) * gv;
        accL += (long long)(v16 * gl);
        accR += (long long)(v16 * gr);
    }

    for (int i = 0; i < MAX_STREAMS; i++)
    {
        Stream* st = &g_streams[i];
        if (!st->used || !st->playing || !st->pcm || !st->frameCount) continue;

        double effRate = st->playback_rate > 0 ? st->playback_rate : st->rate;
        double step = effRate / g_outRate;
        double npos = st->pos + step;

        if (st->loop_count == 0)
        {
            while (npos >= (double)st->frameCount) npos -= (double)st->frameCount;
        }
        else
        {
            if (npos >= (double)st->frameCount)
            {
                st->playing = false;
                st->status = SMP_DONE;
                continue;
            }
        }

        st->pos = npos;
        int ip = (int)npos;
        if (ip < 0) ip = 0;
        if ((U32)ip >= st->frameCount)
        {
            st->playing = false;
            st->status = SMP_DONE;
            continue;
        }
        double gv = volGain(st->volume);
        if (gv <= 0.0) continue;
        accL += (long long)(st->pcm[2 * ip] * gv);
        accR += (long long)(st->pcm[2 * ip + 1] * gv);
    }

    double mg = volGain(g_driver.master_volume);
    out[0] = clampShort((long long)(accL * mg));
    out[1] = clampShort((long long)(accR * mg));
}

static void g_bqCallback(SLAndroidSimpleBufferQueueItf bq, void* /*ctx*/)
{
    short* out = g_driver.bufs[g_driver.curBuf];
    pthread_mutex_lock(&g_audioLock);
    for (int f = 0; f < FRAMES; f++)
    {
        mixOneFrame(out + 2 * f);
    }
    pthread_mutex_unlock(&g_audioLock);
    g_driver.curBuf ^= 1;
    (*bq)->Enqueue(bq, out, FRAMES * 2 * sizeof(short));

    // DEBUG temporal: volcar la salida del mixer (interleaved int16) a archivo.
    static FILE* g_dump = 0;
    static U32 g_dumpLeft = 0;
    if (!g_dump)
    {
        const char* dd = G2Platform_GetGameDataDir();
        if (dd && dd[0])
        {
            char p[1024];
            snprintf(p, sizeof(p), "%s/mix_dump.raw", dd);
            g_dump = fopen(p, "wb");
            g_dumpLeft = 22050 * 5 * 2 * sizeof(short);
            if (g_dump) __android_log_print(ANDROID_LOG_INFO, "GTA2-audio", "mix_dump: %s", p);
        }
    }
    if (g_dump && g_dumpLeft)
    {
        size_t n = FRAMES * 2 * sizeof(short);
        if (n > g_dumpLeft) n = g_dumpLeft;
        fwrite(out, 1, n, g_dump);
        g_dumpLeft -= (U32)n;
        if (g_dumpLeft == 0)
        {
            fclose(g_dump);
            g_dump = 0;
            __android_log_print(ANDROID_LOG_INFO, "GTA2-audio", "mix_dump done");
        }
    }
}

// ---------------------------------------------------------------------------
// Decodificador WAV (PCM + IMA ADPCM) -> int16 estereo interleaved
// ---------------------------------------------------------------------------

static int imaDecode(int* pred, int* index, int nib)
{
    int step = g_imaStep[*index];
    int diff = step >> 3;
    if (nib & 4) diff += step;
    if (nib & 2) diff += step >> 1;
    if (nib & 1) diff += step >> 2;
    if (nib & 8) *pred -= diff; else *pred += diff;
    if (*pred > 32767) *pred = 32767;
    else if (*pred < -32768) *pred = -32768;
    *index += g_imaIndexTable[nib & 7];
    if (*index < 0) *index = 0;
    else if (*index > 88) *index = 88;
    return *pred;
}

// Decodifica un WAV completo de 'buf' a est*ereo int16. Devuelve pcm (malloc) y
// frameCount. Devuelve 0 si el formato no se puede decodificar.
static short* decodeWav(const U8* buf, U32 size, U32* rateOut, U32* framesOut)
{
    *rateOut = 0; *framesOut = 0;
    if (size < 44 || memcmp(buf, "RIFF", 4) || memcmp(buf + 8, "WAVE", 4)) return 0;

    U16 fmtTag = 0, nCh = 0, blockAlign = 0, bits = 0;
    U32 rate = 0;
    const U8* data = 0; U32 dataSize = 0;

    U32 p = 12;
    while (p + 8 <= size)
    {
        U32 cid = *(const U32*)(buf + p);
        U32 csz = (U32)(buf[p + 4] | (buf[p + 5] << 8) | (buf[p + 6] << 16) | (buf[p + 7] << 24));
        if (cid == 0x20746D66u) // 'fmt '
        {
            if (csz >= 16 && p + 8 + 16 <= size)
            {
                fmtTag = (U16)(buf[p + 8] | (buf[p + 9] << 8));
                nCh = (U16)(buf[p + 10] | (buf[p + 11] << 8));
                rate = (U32)(buf[p + 12] | (buf[p + 13] << 8) | (buf[p + 14] << 16) | (buf[p + 15] << 24));
                blockAlign = (U16)(buf[p + 20] | (buf[p + 21] << 8));
                bits = (U16)(buf[p + 22] | (buf[p + 23] << 8));
            }
        }
        else if (cid == 0x61746164u) // 'data'
        {
            data = buf + p + 8;
            dataSize = csz;
        }
        p += 8 + csz + (csz & 1);
        if (p >= size) break;
    }

    if (!fmtTag || !nCh || !rate) return 0;
    if (nCh < 1 || nCh > 2) return 0;

    U32 frames = 0;
    short* pcm = 0;
    U32 outCh = nCh; // canales en el buffer temporal (1 o 2)

    if (fmtTag == 1) // PCM
    {
        if (bits == 16)
        {
            frames = dataSize / (2 * nCh);
            pcm = (short*)malloc((size_t)frames * outCh * sizeof(short));
            if (!pcm) return 0;
            for (U32 f = 0; f < frames; f++)
            {
                for (U32 c = 0; c < nCh; c++)
                {
                    const U8* s = data + (f * nCh + c) * 2;
                    short v = (short)(s[0] | (s[1] << 8));
                    pcm[f * outCh + c] = v;
                }
            }
        }
        else if (bits == 8)
        {
            frames = dataSize / nCh;
            pcm = (short*)malloc((size_t)frames * outCh * sizeof(short));
            if (!pcm) return 0;
            for (U32 f = 0; f < frames; f++)
            {
                for (U32 c = 0; c < nCh; c++)
                {
                    short v = (short)(((int)data[f * nCh + c] - 128) << 8);
                    pcm[f * outCh + c] = v;
                }
            }
        }
        else return 0;
    }
    else if (fmtTag == 0x11) // IMA ADPCM
    {
        if (blockAlign < (U16)(4 * nCh)) return 0;
        U32 spb = ((blockAlign / nCh) - 4) * 2;            // muestras por canal por bloque
        U32 numBlocks = dataSize / blockAlign;
        U32 total = numBlocks * spb;
        pcm = (short*)malloc((size_t)total * outCh * sizeof(short));
        if (!pcm) return 0;

        int pred[2] = {0, 0}, idx[2] = {0, 0};
        U32 out = 0;
        for (U32 b = 0; b < numBlocks; b++)
        {
            const U8* blk = data + b * blockAlign;
            for (U32 c = 0; c < nCh; c++)
            {
                pred[c] = (short)(blk[c * 4] | (blk[c * 4 + 1] << 8));
                idx[c] = blk[c * 4 + 2];
                if (idx[c] > 88) idx[c] = 88;
            }
            const U8* body = blk + 4 * nCh;
            for (U32 j = 0; j < spb; j++)
            {
                for (U32 c = 0; c < nCh; c++)
                {
                    const U8* bt = body + (j * nCh + c) / 2;
                    int nib = ((j * nCh + c) & 1) ? (*bt >> 4) : (*bt & 0xF);
                    int v = imaDecode(&pred[c], &idx[c], nib);
                    pcm[out * outCh + c] = (short)v;
                }
                out++;
            }
        }
        frames = out;
    }
    else return 0;

    if (!pcm || frames == 0)
    {
        free(pcm);
        return 0;
    }

    // Normalizar a est*ereo: si era mono (outCh==1), expandir L->R.
    if (outCh == 1)
    {
        short* st = (short*)malloc((size_t)frames * 2 * sizeof(short));
        if (!st) { free(pcm); return 0; }
        for (U32 f = 0; f < frames; f++)
        {
            st[f * 2] = pcm[f];
            st[f * 2 + 1] = pcm[f];
        }
        free(pcm);
        pcm = st;
    }

    *rateOut = rate;
    *framesOut = frames;
    return pcm;
}

// ---------------------------------------------------------------------------
// AIL API
// ---------------------------------------------------------------------------

} // namespace

extern "C"
{

M3DRESULT AIL_startup(void)
{
    return M3D_NOERR;
}

M3DRESULT AIL_shutdown(void)
{
    pthread_mutex_lock(&g_audioLock);
    if (g_driverOpen)
    {
        openSlStop();
        g_driverOpen = false;
    }
    pthread_mutex_unlock(&g_audioLock);
    return M3D_NOERR;
}

void AIL_set_digital_master_volume(HDIGDRIVER /*drvr*/, S32 volume)
{
    pthread_mutex_lock(&g_audioLock);
    g_driver.master_volume = volume;
    pthread_mutex_unlock(&g_audioLock);
}

void AIL_set_preference(U32 /*number*/, S32 /*value*/)
{
}

S32 AIL_waveOutOpen(HDIGDRIVER* drvr, U32 /*device*/, S32 /*flags*/, WAVEFORMAT* wf)
{
    if (!drvr) return 1;
    pthread_mutex_lock(&g_audioLock);
    if (g_driverOpen)
    {
        openSlStop();
        g_driverOpen = false;
    }
    g_driver.rate = wf ? wf->nSamplesPerSec : 22050;
    if (!g_driver.rate) g_driver.rate = 22050;
    g_outRate = (double)g_driver.rate;
    g_driver.master_volume = 127;
    g_driver.curBuf = 0;
    for (int i = 0; i < 2; i++) memset(g_driver.bufs[i], 0, sizeof(g_driver.bufs[i]));
    pthread_mutex_unlock(&g_audioLock);

    openSlStart(&g_driver);
    pthread_mutex_lock(&g_audioLock);
    bool startOk = g_playerObj && g_bq && g_player;
    if (startOk)
    {
        (*g_bq)->RegisterCallback(g_bq, g_bqCallback, &g_driver);
        (*g_bq)->Enqueue(g_bq, g_driver.bufs[0], FRAMES * 2 * sizeof(short));
        (*g_bq)->Enqueue(g_bq, g_driver.bufs[1], FRAMES * 2 * sizeof(short));
        (*g_player)->SetPlayState(g_player, SL_PLAYSTATE_PLAYING);
        g_driverOpen = true;
        __android_log_print(ANDROID_LOG_INFO, "GTA2-audio", "waveOutOpen OK rate=%u", (unsigned)g_driver.rate);
    }
    pthread_mutex_unlock(&g_audioLock);

    if (!startOk)
    {
        openSlStop();
        *drvr = 0;
        return 1;
    }
    *drvr = &g_driver;
    return 0;
}

void AIL_waveOutClose(HDIGDRIVER /*drvr*/)
{
    pthread_mutex_lock(&g_audioLock);
    if (g_driverOpen)
    {
        openSlStop();
        g_driverOpen = false;
    }
    pthread_mutex_unlock(&g_audioLock);
}

void* AIL_mem_alloc_lock(U32 size)
{
    void* p = calloc(1, size ? size : 1);
    return p;
}

void AIL_mem_free_lock(void* mem)
{
    free(mem);
}

S32 AIL_enumerate_3D_providers(HPROENUM* /*enumerator*/, HPROVIDER* /*prov*/, char** /*name*/)
{
    return 0; // sin proveedores 3D -> GTA2 usa solo 2D
}

U32 AIL_stream_status(HSTREAM stream)
{
    Stream* st = (Stream*)stream;
    pthread_mutex_lock(&g_audioLock);
    U32 s = st ? st->status : SMP_DONE;
    pthread_mutex_unlock(&g_audioLock);
    return s;
}

HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER /*drvr*/)
{
    pthread_mutex_lock(&g_audioLock);
    HSAMPLE h = 0;
    for (int i = 0; i < MAX_SAMPLES; i++)
    {
        if (!g_samples[i].used)
        {
            g_samples[i].used = true;
            h = (HSAMPLE)&g_samples[i];
            break;
        }
    }
    pthread_mutex_unlock(&g_audioLock);
    return h;
}

void AIL_init_sample(HSAMPLE samp)
{
    Sample* s = (Sample*)samp;
    if (!s) return;
    pthread_mutex_lock(&g_audioLock);
    s->playing = false;
    s->status = SMP_DONE;
    s->data = 0;
    s->len = 0;
    s->pos = 0.0;
    s->rate = (S32)g_outRate;
    s->volume = 127;
    s->pan = 64;
    s->loop_count = 1;
    s->loops_left = 0;
    s->loop_start_byte = 0;
    s->loop_end_byte = AIL_EOF;
    pthread_mutex_unlock(&g_audioLock);
}

void AIL_set_sample_type(HSAMPLE samp, S32 /*format*/, S32 /*bits*/)
{
    // Los SFX de GTA2 (RAW) son 16-bit signed LE mono; se ignora el formato
    // declarado (el juego pasa MONO_16 en los canales).
    (void)samp;
}

void AIL_release_sample_handle(HSAMPLE samp)
{
    Sample* s = (Sample*)samp;
    if (!s) return;
    pthread_mutex_lock(&g_audioLock);
    s->used = false;
    s->playing = false;
    s->status = SMP_DONE;
    pthread_mutex_unlock(&g_audioLock);
}

S32 AIL_stream_volume(HSTREAM stream)
{
    Stream* st = (Stream*)stream;
    pthread_mutex_lock(&g_audioLock);
    S32 v = st ? st->volume : 0;
    pthread_mutex_unlock(&g_audioLock);
    return v;
}

void AIL_set_stream_volume(HSTREAM stream, S32 volume)
{
    Stream* st = (Stream*)stream;
    if (!st) return;
    pthread_mutex_lock(&g_audioLock);
    st->volume = volume;
    pthread_mutex_unlock(&g_audioLock);
}

void AIL_delay(U32 ms)
{
    usleep(ms * 1000u);
}

void AIL_close_stream(HSTREAM stream)
{
    Stream* st = (Stream*)stream;
    if (!st) return;
    pthread_mutex_lock(&g_audioLock);
    st->playing = false;
    st->status = SMP_DONE;
    if (st->pcm) free(st->pcm);
    st->pcm = 0;
    st->used = false;
    pthread_mutex_unlock(&g_audioLock);
}

void AIL_release_3D_sample_handle(H3DSAMPLE /*samp*/)
{
}

void AIL_set_sample_address(HSAMPLE samp, void* data, U32 len)
{
    Sample* s = (Sample*)samp;
    if (!s) return;
    pthread_mutex_lock(&g_audioLock);
    s->data = (const U8*)data;
    s->len = len;
    s->pos = 0.0;
    if (s->loop_end_byte == AIL_EOF) s->loop_end_byte = len;
    pthread_mutex_unlock(&g_audioLock);
}

void AIL_set_sample_volume(HSAMPLE samp, S32 volume)
{
    Sample* s = (Sample*)samp;
    if (!s) return;
    pthread_mutex_lock(&g_audioLock);
    s->volume = volume;
    pthread_mutex_unlock(&g_audioLock);
}

void AIL_set_sample_pan(HSAMPLE samp, S32 pan)
{
    Sample* s = (Sample*)samp;
    if (!s) return;
    pthread_mutex_lock(&g_audioLock);
    s->pan = pan;
    pthread_mutex_unlock(&g_audioLock);
}

void AIL_set_sample_playback_rate(HSAMPLE samp, S32 rate)
{
    Sample* s = (Sample*)samp;
    if (!s) return;
    pthread_mutex_lock(&g_audioLock);
    s->rate = rate > 0 ? rate : (S32)g_outRate;
    pthread_mutex_unlock(&g_audioLock);
}

void AIL_set_sample_loop_block(HSAMPLE samp, S32 loop_start, S32 loop_end)
{
    Sample* s = (Sample*)samp;
    if (!s) return;
    pthread_mutex_lock(&g_audioLock);
    s->loop_start_byte = (U32)loop_start;
    s->loop_end_byte = (loop_end == -1) ? AIL_EOF : (U32)loop_end;
    pthread_mutex_unlock(&g_audioLock);
}

void AIL_set_sample_loop_count(HSAMPLE samp, S32 count)
{
    Sample* s = (Sample*)samp;
    if (!s) return;
    pthread_mutex_lock(&g_audioLock);
    s->loop_count = count;
    pthread_mutex_unlock(&g_audioLock);
}

U32 AIL_sample_status(HSAMPLE samp)
{
    Sample* s = (Sample*)samp;
    pthread_mutex_lock(&g_audioLock);
    U32 st = s ? s->status : SMP_DONE;
    pthread_mutex_unlock(&g_audioLock);
    return st;
}

void AIL_start_sample(HSAMPLE samp)
{
    Sample* s = (Sample*)samp;
    if (!s) return;
    pthread_mutex_lock(&g_audioLock);
    if (s->data && s->len)
    {
        s->playing = true;
        s->status = SMP_PLAYING;
        s->pos = 0.0;
        s->loops_left = s->loop_count > 1 ? s->loop_count - 1 : 0;
    }
    pthread_mutex_unlock(&g_audioLock);
}

void AIL_end_sample(HSAMPLE samp)
{
    Sample* s = (Sample*)samp;
    if (!s) return;
    pthread_mutex_lock(&g_audioLock);
    s->playing = false;
    s->status = SMP_DONE;
    pthread_mutex_unlock(&g_audioLock);
}

U32 AIL_set_3D_sample_info(H3DSAMPLE /*samp*/, AILSOUNDINFO* /*info*/)
{
    return 1;
}

void AIL_set_3D_sample_volume(H3DSAMPLE /*samp*/, S32 /*volume*/)
{
}

void AIL_set_3D_position(H3DPOBJECT /*obj*/, F32 /*x*/, F32 /*y*/, F32 /*z*/)
{
}

void AIL_set_3D_sample_distances(H3DSAMPLE /*samp*/, F32 /*max_d*/, F32 /*min_d*/, F32 /*max_v*/, F32 /*min_v*/)
{
}

void AIL_set_3D_sample_playback_rate(H3DSAMPLE /*samp*/, S32 /*rate*/)
{
}

void AIL_set_3D_sample_loop_block(H3DSAMPLE /*samp*/, S32 /*loop_start*/, S32 /*loop_end*/)
{
}

void AIL_set_3D_sample_loop_count(H3DSAMPLE /*samp*/, S32 /*count*/)
{
}

U32 AIL_3D_sample_status(H3DSAMPLE /*samp*/)
{
    return SMP_DONE;
}

void AIL_start_3D_sample(H3DSAMPLE /*samp*/)
{
}

void AIL_end_3D_sample(H3DSAMPLE /*samp*/)
{
}

S32 AIL_set_3D_provider_preference(HPROVIDER /*lib*/, const char* /*attribute*/, void* /*value*/)
{
    return 0;
}

void AIL_3D_provider_attribute(HPROVIDER /*lib*/, const char* /*attribute*/, void* /*value*/)
{
}

M3DRESULT AIL_open_3D_provider(HPROVIDER /*lib*/)
{
    return M3D_NOERR;
}

void AIL_close_3D_provider(HPROVIDER /*lib*/)
{
}

void AIL_set_stream_loop_count(HSTREAM stream, S32 count)
{
    Stream* st = (Stream*)stream;
    if (!st) return;
    pthread_mutex_lock(&g_audioLock);
    st->loop_count = count;
    pthread_mutex_unlock(&g_audioLock);
}

void AIL_start_stream(HSTREAM stream)
{
    Stream* st = (Stream*)stream;
    if (!st) return;
    pthread_mutex_lock(&g_audioLock);
    if (st->pcm && st->frameCount)
    {
        st->playing = true;
        st->status = SMP_PLAYING;
        st->pos = 0.0;
    }
    pthread_mutex_unlock(&g_audioLock);
}

void AIL_set_stream_ms_position(HSTREAM stream, S32 ms_pos)
{
    Stream* st = (Stream*)stream;
    if (!st) return;
    pthread_mutex_lock(&g_audioLock);
    double effRate = st->playback_rate > 0 ? st->playback_rate : st->rate;
    if (effRate > 0)
    {
        st->pos = ((double)ms_pos * effRate) / 1000.0;
        if (st->pos >= st->frameCount) st->pos = (double)(st->frameCount - 1);
    }
    pthread_mutex_unlock(&g_audioLock);
}

S32 AIL_stream_playback_rate(HSTREAM stream)
{
    Stream* st = (Stream*)stream;
    pthread_mutex_lock(&g_audioLock);
    S32 r = st ? (st->playback_rate > 0 ? st->playback_rate : st->rate) : 22050;
    pthread_mutex_unlock(&g_audioLock);
    return r;
}

void AIL_stream_ms_position(HSTREAM stream, void* ms_pos, void* ms_len)
{
    Stream* st = (Stream*)stream;
    if (!st) return;
    pthread_mutex_lock(&g_audioLock);
    double effRate = st->playback_rate > 0 ? st->playback_rate : st->rate;
    if (ms_len)
    {
        *(S32*)ms_len = (effRate > 0) ? (S32)((st->frameCount * 1000) / effRate) : 0;
    }
    if (ms_pos)
    {
        *(S32*)ms_pos = (effRate > 0) ? (S32)((st->pos * 1000.0) / effRate) : 0;
    }
    pthread_mutex_unlock(&g_audioLock);
}

void AIL_set_stream_playback_rate(HSTREAM stream, S32 rate)
{
    Stream* st = (Stream*)stream;
    if (!st) return;
    pthread_mutex_lock(&g_audioLock);
    st->playback_rate = rate;
    pthread_mutex_unlock(&g_audioLock);
}

HSTREAM AIL_open_stream(HDIGDRIVER /*drvr*/, const char* filename, S32 /*memory_type*/)
{
    if (!filename) return 0;

    pthread_mutex_lock(&g_audioLock);
    Stream* freeSt = 0;
    for (int i = 0; i < MAX_STREAMS; i++)
    {
        if (!g_streams[i].used) { freeSt = &g_streams[i]; break; }
    }
    pthread_mutex_unlock(&g_audioLock);
    if (!freeSt) return 0;

    FILE* f = fopen(filename, "rb");
    if (!f)
    {
        __android_log_print(ANDROID_LOG_WARN, "GTA2-audio", "stream open FAILED: %s", filename);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize <= 0)
    {
        fclose(f);
        return 0;
    }
    U8* buf = (U8*)malloc((size_t)fsize);
    if (!buf)
    {
        fclose(f);
        return 0;
    }
    size_t rd = fread(buf, 1, (size_t)fsize, f);
    fclose(f);

    U32 rate = 0, frames = 0;
    short* pcm = decodeWav(buf, (U32)rd, &rate, &frames);
    free(buf);
    if (!pcm)
    {
        __android_log_print(ANDROID_LOG_WARN, "GTA2-audio", "stream decode FAILED: %s", filename);
        return 0;
    }

    pthread_mutex_lock(&g_audioLock);
    freeSt->used = true;
    freeSt->playing = false;
    freeSt->status = SMP_DONE;
    freeSt->pcm = pcm;
    freeSt->frameCount = frames;
    freeSt->rate = rate;
    freeSt->pos = 0.0;
    freeSt->volume = 127;
    freeSt->loop_count = 1;
    freeSt->playback_rate = 0;
    pthread_mutex_unlock(&g_audioLock);

    __android_log_print(ANDROID_LOG_INFO, "GTA2-audio", "stream open OK: %s rate=%u frames=%u", filename, (unsigned)rate, (unsigned)frames);
    return (HSTREAM)freeSt;
}

S32 AIL_digital_handle_reacquire(HDIGDRIVER /*drvr*/)
{
    return 1;
}

void AIL_digital_handle_release(HDIGDRIVER /*drvr*/)
{
}

H3DSAMPLE AIL_allocate_3D_sample_handle(HPROVIDER /*lib*/)
{
    return 0;
}

} // extern "C"