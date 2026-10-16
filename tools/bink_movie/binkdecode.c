// binkdecode.c - extrae data/Movie/intro.bik (Bink 1.x) a un .raw de deltas
// (w*h*2 bytes por frame) usando el binkw32.dll del juego. mkintro.exe
// (C#/.NET, DEFLATE) comprime cada frame y arma el intro.mv (formato G2MV v2).
//
// Compilar (x86, el binkw32.dll del GTA2 es 32-bit):
//   vcvarsall.bat x86
//   cl /O2 binkdecode.c /Fe:binkdecode.exe
//
// Uso:
//   binkdecode <binkw32.dll> <intro.bik> <intro.raw>
//   csc mkintro.cs  ->  mkintro <intro.raw> <intro.mv> <w> <h> <frames> <fps> <delayMs>
//
// Formato intro.mv v2 (G2MV):
//   header 64B: magic "G2MV", ver=2, pixfmt=1(RGB565), pad u8, w u32, h u32,
//               frames u32, fps u32, dataSize u32, pad2[40] (pad2[0..3]=delayMs)
//   datos: por marco: u32 csize, luego csize bytes DEFLATE (raw) del array de
//          deltas (w*h*2). csize==0 => frame identico al anterior.
//   decodificar: cur[i] = prev[i] + delta[i] (aritmetica byte, wrap)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

typedef void* (__stdcall *FnBinkOpen)(const char* name, unsigned long flags);
typedef void  (__stdcall *FnBinkDoFrame)(void* hb);
typedef void  (__stdcall *FnBinkNextFrame)(void* hb);
typedef void  (__stdcall *FnBinkClose)(void* hb);
typedef int   (__stdcall *FnBinkWait)(void* hb);
typedef int   (__stdcall *FnBinkCopyToBuffer)(void* hb, void* dest, int destpitch,
                                              unsigned long destheight, unsigned long destx,
                                              unsigned long desty, unsigned long flags);
typedef int   (__stdcall *FnBinkGetSummary)(void* hb, long* summary);

#pragma pack(push,1)
typedef struct { char  id[8]; } PADHDR;
typedef struct {
    char  magic[4]; // "G2MV"
    unsigned char ver;
    unsigned char pixfmt;
    unsigned short pad;
    unsigned long w, h, frames, fps, dataSize;
    unsigned char pad2[40];
} G2MVHeader;
#pragma pack(pop)

#define BINKCOPYALL 0x4000000
#define BINK565     0x3  // Bink 1.x: 0=RFB24 1=RGB32 2=RGB555 3=RGB565 4=655 5=664

// BinkGetSummary (Bink 1.x, binkw32.dll del GTA2):
//   s[0]=width  s[1]=height  s[2]=(no usado: frame interno)
//   s[3]=frameRate  s[4]=frameRateBase  s[5]=s[3]  s[6]=s[4]  s[7]=0
//   s[8]=totalFrames
unsigned long get_summary_long(void* hb, FnBinkGetSummary fn, int idx)
{
    long s[20];
    memset(s, 0, sizeof(s));
    fn(hb, s);
    return (unsigned long)s[idx];
}

static void put16(FILE* f, unsigned short v) { fputc(v & 0xFF, f); fputc((v >> 8) & 0xFF, f); }

// Delta cuantizado (pérdida): cambios pequeños -> 0 (mantiene prev); cambios
// medios -> múltiplo de 16 (agrupa para RLE); cambios grandes -> exactos.
static unsigned char q_delta(unsigned char c, unsigned char p)
{
    int d = (int)c - (int)p;
    if (d > -8 && d < 8) return 0;
    int q = (d >= 0) ? ((d + 8) & ~15) : -(((-d) + 8) & ~15);
    if (q == 0) q = (d > 0) ? 8 : -8;
    if (q > 240 || q < -240) q = d;
    return (unsigned char)q;
}

// Emite el array de deltas crudos (w*h*2 bytes) del frame. El compresor
// (mkintro.exe, DEFLATE via .NET) produce el G2MV v2 con un stream por frame.
static void encode_frame(FILE* out, const unsigned char* cur, const unsigned char* prev, unsigned long n)
{
    for (unsigned long i = 0; i < n; i++)
    {
        fputc(q_delta(cur[i], prev[i]), out);
    }
}

// Downscale 2x2 por promediado de canales RGB565 (los .bik del GTA2 son
// 640x480; se emiten a 320x240 para que intro.mv quede ~26MB).
static void downscale2x(const unsigned char* src, int w, int h, unsigned char* dst)
{
    int nw = w / 2, nh = h / 2;
    for (int y = 0; y < nh; y++)
    {
        const unsigned short* prow = (const unsigned short*)src + (y * 2) * w;
        for (int x = 0; x < nw; x++)
        {
            unsigned short a = prow[x * 2], b = prow[x * 2 + 1];
            unsigned short c = prow[x * 2 + w], d = prow[x * 2 + w + 1];
            int r = ((a >> 11) & 0x1F) + ((b >> 11) & 0x1F) + ((c >> 11) & 0x1F) + ((d >> 11) & 0x1F);
            int g = ((a >> 5) & 0x3F) + ((b >> 5) & 0x3F) + ((c >> 5) & 0x3F) + ((d >> 5) & 0x3F);
            int bl = (a & 0x1F) + (b & 0x1F) + (c & 0x1F) + (d & 0x1F);
            ((unsigned short*)dst)[y * nw + x] = (unsigned short)(((r >> 2) << 11) | ((g >> 2) << 5) | (bl >> 2));
        }
    }
}

// Mientras no haya frames decodificados, BinkGetSummary puede no estar
// estable; las dimensiones se toman del header del propio .bik:
//   off8  = frames         off20 = width
//   off24 = height         off28 = frameRate   off32 = frameRateBase
static int read_bik_header(const char* path, unsigned long* w, unsigned long* h,
                           unsigned long* total, long* fr, long* fbr)
{
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    unsigned char b[36];
    if (fread(b, 1, 36, f) != 36) { fclose(f); return -1; }
    fclose(f);
    if (memcmp(b, "BIKf", 4) != 0 && memcmp(b, "BIKb", 4) != 0) return -1;
#define RD32(o) ((unsigned long)(b[(o)]) | ((unsigned long)b[(o)+1] << 8) | \
                 ((unsigned long)b[(o)+2] << 16) | ((unsigned long)b[(o)+3] << 24))
    *total = RD32(8);
    *w = RD32(20);
    *h = RD32(24);
    *fr = (long)(unsigned long)RD32(28);
    *fbr = (long)(unsigned long)RD32(32);
#undef RD32
    return 0;
}

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc != 4)
    {
        fprintf(stderr, "uso: binkdecode <binkw32.dll> <intro.bik> <intro.mv>\n");
        return 2;
    }
    HMODULE hdll = LoadLibraryA(argv[1]);
    if (!hdll) { fprintf(stderr, "no se pudo cargar %s\n", argv[1]); return 1; }
    FnBinkOpen      fnOpen  = 0;
    FnBinkDoFrame   fnDo    = 0;
    FnBinkNextFrame fnNext  = 0;
    FnBinkClose     fnClose = 0;
    FnBinkWait      fnWait  = 0;
    FnBinkCopyToBuffer fnCopy = 0;
    FnBinkGetSummary   fnSum  = 0;
    {
        const char* names[] = { "BinkOpen", "_BinkOpen@8", 0 };
        for (int i = 0; names[i]; i++) { fnOpen = (FnBinkOpen)GetProcAddress(hdll, names[i]); if (fnOpen) break; }
    }
    {
        const char* names[] = { "BinkDoFrame", "_BinkDoFrame@4", 0 };
        for (int i = 0; names[i]; i++) { fnDo = (FnBinkDoFrame)GetProcAddress(hdll, names[i]); if (fnDo) break; }
    }
    {
        const char* names[] = { "BinkNextFrame", "_BinkNextFrame@4", 0 };
        for (int i = 0; names[i]; i++) { fnNext = (FnBinkNextFrame)GetProcAddress(hdll, names[i]); if (fnNext) break; }
    }
    {
        const char* names[] = { "BinkClose", "_BinkClose@4", 0 };
        for (int i = 0; names[i]; i++) { fnClose = (FnBinkClose)GetProcAddress(hdll, names[i]); if (fnClose) break; }
    }
    {
        const char* names[] = { "BinkWait", "_BinkWait@4", 0 };
        for (int i = 0; names[i]; i++) { fnWait = (FnBinkWait)GetProcAddress(hdll, names[i]); if (fnWait) break; }
    }
    {
        const char* names[] = { "BinkCopyToBuffer", "_BinkCopyToBuffer@28", 0 };
        for (int i = 0; names[i]; i++) { fnCopy = (FnBinkCopyToBuffer)GetProcAddress(hdll, names[i]); if (fnCopy) break; }
    }
    {
        const char* names[] = { "BinkGetSummary", "_BinkGetSummary@8", 0 };
        for (int i = 0; names[i]; i++) { fnSum = (FnBinkGetSummary)GetProcAddress(hdll, names[i]); if (fnSum) break; }
    }
    if (!fnOpen || !fnDo || !fnNext || !fnClose || !fnWait || !fnCopy || !fnSum)
    {
        fprintf(stderr, "faltan exports del binkw32.dll (open=%d do=%d next=%d close=%d wait=%d copy=%d sum=%d)\n",
                !!(fnOpen), !!(fnDo), !!(fnNext), !!(fnClose), !!(fnWait), !!(fnCopy), !!(fnSum));
        return 1;
    }

    void* hb = fnOpen(argv[2], BINKCOPYALL);
    if (!hb) { fprintf(stderr, "BinkOpen fallo en %s\n", argv[2]); return 1; }

    unsigned long w = 0, h = 0, total = 0;
    long fr = 0, fbr = 0;
    if (read_bik_header(argv[2], &w, &h, &total, &fr, &fbr) != 0)
    {
        fprintf(stderr, "no se pudo leer el header del .bik\n");
        fnClose(hb);
        return 1;
    }
    unsigned long fps = 30, delayMs = 66;
    if (fr > 0 && fbr > 0)
    {
        fps = (unsigned long)((fr + fbr - 1) / fbr);
        if (fps == 0) fps = 1;
        delayMs = (unsigned long)(((unsigned long long)fbr * 1000) / fr);
        if (delayMs == 0) delayMs = 1;
    }
    if (fps == 0 || fps > 120) fps = 30;
printf("bik: %lux%lu frames=%lu fr=%ld base=%ld -> fps=%lu delay=%lums\n", w, h, total, fr, fbr, fps, delayMs);
    if (w == 0 || h == 0 || total == 0 || total > 100000) { fprintf(stderr, "resumen invalido\n"); fnClose(hb); return 1; }

    // Salida: solo los deltas crudos consecutivos (por frame, w*h*2 bytes);
    // mkintro.exe comprime cada frame con DEFLATE y genera el .mv final.
    FILE* out = fopen(argv[3], "wb");
    if (!out) { fnClose(hb); return 1; }

    // Emite a 320x240 (downscale 2x2 del frame 640x480 del Bink).
    int outW = (int)w / 2, outH = (int)h / 2;
    unsigned long nbytes = (unsigned long)outW * outH * 2;
    unsigned char *full = (unsigned char*)malloc((size_t)w * h * 2);
    unsigned char *cur = (unsigned char*)malloc(nbytes);
    unsigned char *prev = (unsigned char*)malloc(nbytes);
    if (!full || !cur || !prev) { fprintf(stderr, "sin memoria\n"); return 1; }
    memset(prev, 0, nbytes);

    unsigned long frame = 0;
    while (frame < total)
    {
        fnDo(hb);
        fnCopy(hb, full, (int)(w * 2), h, 0, 0, BINK565 | BINKCOPYALL);
        downscale2x(full, (int)w, (int)h, cur);
        if (frame == 2 || frame == 60 || frame == 600 || frame == 1200 || frame == 1650)
        {
            unsigned long nzraw = 0, nzq = 0;
            for (unsigned long z = 0; z < nbytes; z++)
            {
                if (cur[z] != prev[z]) nzraw++;
                if (q_delta(cur[z], prev[z]) != 0) nzq++;
            }
            printf("diagnostico frame=%lu: raw-diff=%lu (%.2f%%) q-diff=%lu (%.2f%%)\n",
                   frame, nzraw, 100.0 * nzraw / nbytes, nzq, 100.0 * nzq / nbytes);
        }
        encode_frame(out, cur, prev, nbytes);
        unsigned char* t = prev; prev = cur; cur = t;
        frame++;
        fnNext(hb);
    }
    fclose(out);
    fnClose(hb);
    long fsz = 0;
    FILE* fmv = fopen(argv[3], "rb");
    if (fmv) { fseek(fmv, 0L, SEEK_END); fsz = ftell(fmv); fclose(fmv); }
    printf("listo: %lu frames, deltas=%.1f MB (%ld bytes)\n", frame, fsz / 1048576.0, fsz);
    return 0;
}