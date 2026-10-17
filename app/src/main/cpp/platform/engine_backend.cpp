

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(x, msg)
#endif

#include "Bink.hpp"
#include "dma_video.hpp"
#include "gbh_graphics.hpp"
#include "winmain.hpp"
#include "Frontend.hpp"
#include "debug.hpp"
#include "keybrd_0x204.hpp"
#include "input.hpp"
#include "BurgerKing_67F8B0.hpp"
#include "jolly_poitras_0x2BC0.hpp"
#include "lucid_hamilton.hpp"
#include "root_sound.hpp"
#include "registry.hpp"
#include "Game_0x40.hpp"
#include "Player.hpp"
#include "Ped.hpp"
#include "char.hpp"
#include "sharp_bose_0x54.hpp"

extern Fix16 dword_703A38;
extern Fix16 dword_6FD80C;
extern Fix16 dword_6FDAC8;
extern u8 byte_6FDB51;
extern u8 byte_6FDB52;

#include <android/log.h>
#include <android/native_window.h>
#include <android/hardware_buffer.h>
#include <zlib.h>
#include <Dinput.h>
#include <wchar.h>
#include <unistd.h>
#include <time.h>
#include <mutex>
#include <string.h>
#include <stdlib.h>
#include <map>
#include <set>

#define LOGT "GTA2-port"

#include <android/native_window.h>
#include "gles_glue.hpp"

static ANativeWindow* gSurface = 0;
static u16* gFramebuffer = 0;
static int gFBW = 640;
static int gFBH = 480;
static int gFBStride = 640;
static std::mutex gVidMutex;

static void* gGlesCtx = 0;
static ANativeWindow* gGlesWin = 0;
struct GlesTexRec { unsigned glid; const WORD* palAtUpload; int wAtUpload, hAtUpload; const void* srcAtUpload; unsigned hashAtUpload; };
static std::map<STexture*, GlesTexRec> gGlesTex;
static std::set<STexture*> gGlesTexDirty;
static void* gles_ensure_context();

static int sFrQuads = 0, sFrTris = 0, sFrZeroUv = 0, sFrBlankTex = 0, sFrStaleTex = 0, sFrClip0 = 0, sFrTriDrawn = 0, sFrShadows = 0;

static u32 gsLastDrawFlags = 0;

void G2Platform_SetSurface(void* anw)
{
    ANativeWindow* old = 0;
    {
        std::lock_guard<std::mutex> lock(gVidMutex);
        old = gSurface;
        gSurface = (ANativeWindow*)anw;
    }

    if (old && old != anw)
        ANativeWindow_release(old);
}

extern "C" std::atomic<bool> g_stop;
extern "C" bool G2Platform_InVehicle()
{
    if (!gGame_0x40_67E008 || !gGame_0x40_67E008->field_38_orf1) return false;
    Player* p = gGame_0x40_67E008->field_38_orf1;
    Ped* ped = p->field_2C4_player_ped;
    if (!ped) return false;
    return ped->field_16C_car != 0;
}

extern "C" void G2Platform_DeleteRegistrySubtree(const char* path);
extern "C" void G2Platform_SetDebugString(const char* valueName, const char* value);
extern "C" void G2Platform_SetDebugDword(const char* valueName, unsigned long value);
void Init_Phi_54_array();
void Init_Phi_6C_array();
void Init_trigonometry_tables();

HINSTANCE gHInstance_708220 = 0;
char_type gWorkingDir_707F64[652] = {0};
s32 gGTA2VersionMajor_708280 = 0;
s32 gGTA2VersionMajor_708284 = 0;
s32 bStartNetworkGame_7081F0 = 0;
bool bDoFrontEnd_626B68 = true;
u32 gLightingDrawFlag_7068F4 = 0;
char_type gLighting_626A09 = 0;
s32 window_width_706630 = 640;
s32 window_height_706B50 = 480;

BINK* gBinkHandleSlot2_6F83B0 = 0;
BINK* gBinkHandleSlot1_6F8168 = 0;
BINKBUFFER* gBinkBufferSlot2_6F80C4 = 0;
BINKBUFFER* gBinkBufferSlot1_6F8170 = 0;
char_type gBinkDDState_6F83FE = 0;
char_type gBinkActiveSlot_6F83FF = 0;
s32 gBinkSummary_6F8250 = 0;
s32 gBinkPixelFormat_6F81B0 = 0;

void __stdcall Bink::Reset_513210()
{
    gBinkHandleSlot2_6F83B0 = 0;
    gBinkHandleSlot1_6F8168 = 0;
    gBinkBufferSlot2_6F80C4 = 0;
    gBinkBufferSlot1_6F8170 = 0;
    gBinkDDState_6F83FE = 0;
    gBinkActiveSlot_6F83FF = 0;
}

void Bink::CloseSlot1_513340()
{
    gBinkBufferSlot1_6F8170 = 0;
    gBinkHandleSlot1_6F8168 = 0;
    gBinkDDState_6F83FE = 0;
    gBinkActiveSlot_6F83FF = 0;
}

void Bink::CloseSlot2_513390()
{
    gBinkBufferSlot2_6F80C4 = 0;
    gBinkHandleSlot2_6F83B0 = 0;
}

void __stdcall Bink::CheckWindowPos_5136D0(s32* pNewWindowX, s32* pNewWindowY)
{
    if (pNewWindowX) *pNewWindowX = 0;
    if (pNewWindowY) *pNewWindowY = 0;
}

BOOL Bink::IsUsingDDBuffer_513770()
{
    return FALSE;
}

BOOL Bink::IsDDBufferOpen_513760()
{
    return FALSE;
}

void Bink::ResetBufferOffset_513720()
{
}

char_type Bink::TickFrame_513240()
{
    return 0;
}

char_type Bink::GetActiveSlot_513790()
{
    return gBinkActiveSlot_6F83FF;
}

void __stdcall Bink::SetActiveSlot_5137A0(char_type slot)
{
    gBinkActiveSlot_6F83FF = slot;
}

void __stdcall Bink::OpenSlot2_5133E0(const char_type* pFileName, HDIGDRIVER a2)
{
    (void)pFileName;
    (void)a2;
}

void __stdcall Bink::SetDDState_5137B0(char_type state)
{
    gBinkDDState_6F83FE = state;
}

void __stdcall Bink::OpenSlot1_513560(const char_type* pFileName, HDIGDRIVER a2)
{
    (void)pFileName;
    (void)a2;
}

struct NetPlay
{
    void GetPlayerName_521100(wchar_t* Destination, u32 idx);
    unsigned char _payload[0xFFC - 0x1C];
};

void NetPlay::GetPlayerName_521100(wchar_t* Destination, u32 idx)
{
    if (!Destination) return;
    wchar_t buf[16] = L"Player0";
    buf[6] = L'1' + (wchar_t)(idx % 9);
    buf[7] = 0;
    wcsncpy(Destination, buf, 15);
    Destination[15] = 0;
}

NetPlay gNetPlay_7071E8;

static SVideo gVidSys_7071D0_storage;

SVideo* gVidSys_7071D0 = &gVidSys_7071D0_storage;

struct SGbhImageEntry
{
    s32 bLoaded;
    u32 field_4_w;
    u32 field_8_h;
    STexture* field_C_pTexture;
    void* field_10_pData;
};

struct SGbhPal
{
    DWORD* mPOriginalData;
    WORD* mPData;
    s32 mbLoaded;
};

static SGbhImageEntry* gGbhImageTable = 0;
static u32 gGbhImageTableCount = 0;
static u32 gGbhTextureId = 0;
static SGbhPal gGbhPals[512];
static f32 gGbhWindowE43E08_l = 0;
static f32 gGbhWindowE43E0C_r = 640;
static f32 gGbhWindowE43E10_t = 0;
static f32 gGbhWindowE43E14_b = 480;
static s32* gGbhGlobals = 0;

static inline u16 vid_rgb565(u8 r, u8 g, u8 b);

static u32 CC gbh_real_initdll(SVideo*)
{
    __android_log_print(ANDROID_LOG_INFO, LOGT, "gbh_InitDLL (real)");
    return 1;
}

static s32 CC gbh_real_init(s32)
{
    return 0;
}

static void CC gbh_real_closedll()
{
}

static void CC gbh_real_setwindow(f32 l, f32 t, f32 r, f32 b)
{
    gGbhWindowE43E08_l = l;
    gGbhWindowE43E10_t = t;
    gGbhWindowE43E0C_r = r;
    gGbhWindowE43E14_b = b;
}

static u32* CC gbh_real_getglobals()
{
    static u32 storage[32] = {0};
    return storage;
}

static void CC gbh_real_resetlights()
{
}

static s32 CC gbh_real_addlight(SLight* pLight)
{
    (void)pLight;
    return 0;
}

static void CC gbh_real_setambient(f32)
{
}

static s32 CC gbh_real_getusedcache(s32)
{
    return 0;
}

static s32 CC gbh_real_setcamera(f32, f32, f32, f32)
{
    return 0;
}

static s32 CC gbh_real_setcolourdepth()
{
    return 16;
}

static u32 CC gbh_real_convertcolour(u8 r, u8 g, u8 b)
{
    return vid_rgb565(r, g, b);
}

static void CC gbh_real_beginscene()
{
    sFrQuads = 0; sFrTris = 0; sFrZeroUv = 0; sFrBlankTex = 0; sFrStaleTex = 0; sFrClip0 = 0; sFrTriDrawn = 0; sFrShadows = 0;


    if (gFramebuffer)
    {
        std::lock_guard<std::mutex> lock(gVidMutex);
        memset(gFramebuffer, 0, (size_t)gFBH * gFBStride * sizeof(u16));
    }

    if (gles_ensure_context())
    {
        gles_begin_frame(gGlesCtx);
    }
}

static void CC gbh_real_endscene()
{
}

static void CC gbh_real_beginlevel()
{
}

static void CC gbh_real_endlevel()
{
}

static void CC gbh_real_plot(s32 x, s32 y, s32 colour, s32)
{
    if (!gFramebuffer) return;
    if (x < 0 || x >= gFBW || y < 0 || y >= gFBH) return;
    gFramebuffer[y * gFBStride + x] = (u16)colour;
}

static s32* CC gbh_real_makescreentable()
{
    static s32 dummy = 0;
    return &dummy;
}

static STexture* CC gbh_real_registertexture(s32 width, s32 height, BYTE* pData, s32 pal_idx, char_type flag)
{
    STexture* pTexture = (STexture*)calloc(1, sizeof(STexture));
    if (!pTexture) return 0;

    pTexture->field_0_id = gGbhTextureId++;
    pTexture->field_E_width = width;
    pTexture->field_10_height = height;
    pTexture->field_14_original_pixel_data_ptr = pData;

    static int maxPal = -1;
    if (pal_idx > maxPal) { maxPal = pal_idx; __android_log_print(ANDROID_LOG_INFO, LOGT, "RTX pal_idx=%d flag=%d newmax", pal_idx, (int)flag); }
    if (pal_idx >= 0 && pal_idx < 512)
    {
        pTexture->field_12_bPalIsValid = gGbhPals[pal_idx].mbLoaded;
        pTexture->field_18_pPaltData = gGbhPals[pal_idx].mPData;
    }
    else if (pal_idx >= 512 && pal_idx < 1000000)
    {
        __android_log_print(ANDROID_LOG_WARN, LOGT, "RTX pal_idx OUT OF RANGE: %d", pal_idx);
    }
    pTexture->field_1C_ptr = 0;
    return pTexture;
}

static STexture* CC gbh_real_locktexture(STexture* pTexture)
{
    if (!pTexture) return 0;
    pTexture->field_8_locked_pixels_ptr = pTexture->field_14_original_pixel_data_ptr;
    pTexture->field_13_flags |= 1;
    pTexture->field_6_pal_size = 256;
    return pTexture;
}

static STexture* CC gbh_real_unlocktexture(STexture* pTexture)
{
    if (!pTexture) return 0;
    if (gGlesCtx) gGlesTexDirty.insert(pTexture);
    pTexture->field_6_pal_size = 0;
    pTexture->field_8_locked_pixels_ptr = 0;
    pTexture->field_13_flags &= 0xFEu;
    return pTexture;
}

static void CC gbh_real_freetexture(STexture* pTexture)
{
    if (!pTexture) return;
    if (gGlesCtx)
    {
        auto it = gGlesTex.find(pTexture);
        if (it != gGlesTex.end())
        {
            if (it->second.glid) gles_delete_texture(gGlesCtx, it->second.glid);
            gGlesTex.erase(it);
        }
        gGlesTexDirty.erase(pTexture);
    }
    free(pTexture);
}

static u32 CC gbh_real_registerpalette(s32 palId, DWORD* pData)
{
    if (palId < 0 || palId >= 512 || !pData) return 0;

    static int shown = 0;
    if (shown < 30) { __android_log_print(ANDROID_LOG_INFO, LOGT, "RPAL palId=%d", palId); shown++; }

    s32 i;
    for (i = 0; i < 256; i++)
    {
        if (i == 0)
        {
            *pData = 0;
        }
        else if (*pData == 0)
        {
            *pData = 0x10000;
        }
        pData += 64;
    }

    WORD* pAllocatedData = (WORD*)malloc(256 * 2 * sizeof(WORD));
    if (!pAllocatedData) return 0;
    if (gGbhPals[palId].mPData) free(gGbhPals[palId].mPData);
    gGbhPals[palId].mPOriginalData = pData - 16384;
    gGbhPals[palId].mbLoaded = 1;
    gGbhPals[palId].mPData = pAllocatedData;

    DWORD* pSrc = gGbhPals[palId].mPOriginalData;
    for (i = 0; i < 256; i++)
    {
        pAllocatedData[i] = vid_rgb565((u8)((*pSrc >> 16) & 0xFF), (u8)((*pSrc >> 8) & 0xFF), (u8)(*pSrc & 0xFF));
        pSrc += 64;
    }
    return palId;
}

static void CC gbh_real_freepalette(s32 palId)
{
    if (palId >= 0 && palId < 512 && gGbhPals[palId].mPData)
    {
        free(gGbhPals[palId].mPData);
        gGbhPals[palId].mPData = 0;
        gGbhPals[palId].mbLoaded = 0;
    }
}

static void CC gbh_real_assignpalette(STexture* pTexture, s32 palId)
{
    if (!pTexture) return;
    if (palId < 0 || palId >= 512) return;
    pTexture->field_18_pPaltData = gGbhPals[palId].mPData;
    pTexture->field_12_bPalIsValid = gGbhPals[palId].mbLoaded;
}

static s32 CC gbh_real_initimagetable(s32 tableSize)
{
    if (tableSize <= 0) return -1;
    gGbhImageTable = (SGbhImageEntry*)calloc(tableSize, sizeof(SGbhImageEntry));
    if (!gGbhImageTable) return -1;
    gGbhImageTableCount = tableSize;
    return 0;
}

static s32 CC gbh_real_freeimagetable()
{
    if (gGbhImageTable)
    {
        for (u32 i = 0; i < gGbhImageTableCount; i++)
        {
            if (gGbhImageTable[i].field_10_pData)
            {
                free(gGbhImageTable[i].field_10_pData);
                gGbhImageTable[i].field_10_pData = 0;
            }
            if (gGbhImageTable[i].field_C_pTexture)
            {
                if (gGlesCtx)
                {
                    auto it = gGlesTex.find(gGbhImageTable[i].field_C_pTexture);
                    if (it != gGlesTex.end())
                    {
                        if (it->second.glid) gles_delete_texture(gGlesCtx, it->second.glid);
                        gGlesTex.erase(it);
                    }
                    gGlesTexDirty.erase(gGbhImageTable[i].field_C_pTexture);
                }
                free(gGbhImageTable[i].field_C_pTexture);
                gGbhImageTable[i].field_C_pTexture = 0;
            }
            gGbhImageTable[i].bLoaded = 0;
        }
        free(gGbhImageTable);
        gGbhImageTable = 0;
    }
    gGbhImageTableCount = 0;
    return 0;
}

static void* gles_ensure_context()
{
#define GTA2_FORCE_CPU 0
    if (GTA2_FORCE_CPU) return 0;
    ANativeWindow* win = 0;
    {
        std::lock_guard<std::mutex> lock(gVidMutex);
        win = gSurface;
        if (win) ANativeWindow_acquire(win);
    }
    if (gGlesCtx)
    {

        if (win == gGlesWin)
        {
            if (win) ANativeWindow_release(win);
            return gGlesCtx;
        }





        __android_log_print(ANDROID_LOG_INFO, LOGT,
            "GLES superficie %p (era %p): recreando contexto",
            (void*)win, (void*)gGlesWin);
        gles_destroy(gGlesCtx);
        gGlesCtx = 0;
        gGlesTex.clear();
        gGlesTexDirty.clear();
        if (gGlesWin)
        {
            ANativeWindow_release(gGlesWin);
            gGlesWin = 0;
        }
        if (!win) { ANativeWindow_release(win); return 0; }
    }
    else if (!win)
    {
        return 0;
    }
    gGlesCtx = gles_create(win);
    if (gGlesCtx)
    {
        gGlesWin = win;
        __android_log_print(ANDROID_LOG_INFO, LOGT, "GLES contexto listo (render nativo)");
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR, LOGT, "GLES contexto FALLO: se usa rasterizador CPU");
        ANativeWindow_release(win);
    }
    return gGlesCtx;
}

static unsigned fast_content_hash(const BYTE* p, size_t n)
{
    unsigned h = 2166136261u;
    size_t stride = n / 4096 + 1;
    for (size_t i = 0; i < n; i += stride)
        h = (h ^ p[i]) * 16777619u;
    return h;
}

static void gles_alpha_bleed(unsigned char* rgba, int w, int h)
{
    unsigned char* tmp = (unsigned char*)malloc((size_t)w * h * 4);
    if (!tmp) return;
    memcpy(tmp, rgba, (size_t)w * h * 4);
    for (int pass = 0; pass < 8; pass++)
    {
        bool any = false;
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                const unsigned char* p = tmp + ((size_t)y * w + x) * 4;
                if (p[3] != 0) continue;
                int rr = 0, gg = 0, bb = 0, n = 0;
                for (int dy = -1; dy <= 1; dy++)
                {
                    const int ny = y + dy;
                    if (ny < 0 || ny >= h) continue;
                    for (int dx = -1; dx <= 1; dx++)
                    {
                        if (dx == 0 && dy == 0) continue;
                        const int nx = x + dx;
                        if (nx < 0 || nx >= w) continue;
                        const unsigned char* q = tmp + ((size_t)ny * w + nx) * 4;
                        if (q[3] != 0) { rr += q[0]; gg += q[1]; bb += q[2]; n++; }
                    }
                }
                if (n)
                {
                    unsigned char* o = rgba + ((size_t)y * w + x) * 4;
                    o[0] = (unsigned char)(rr / n);
                    o[1] = (unsigned char)(gg / n);
                    o[2] = (unsigned char)(bb / n);
                    any = true;
                }
            }
        }
        if (!any) break;
        memcpy(tmp, rgba, (size_t)w * h * 4);
    }
    free(tmp);
}

static unsigned gles_ensure_texture(STexture* pTex)
{
    if (!gGlesCtx || !pTex || !pTex->field_14_original_pixel_data_ptr) return 0;
    const int w = pTex->field_E_width;
    const int h = pTex->field_10_height;
    if (w <= 0 || h <= 0 || w > 8192 || h > 8192)
    {
        static int sBadN = 0;
        if (sBadN++ < 20)
            __android_log_print(ANDROID_LOG_WARN, LOGT, "RTXBAD w=%d h=%d pal=%p (tex %p)", w, h, (void*)pTex->field_18_pPaltData, (void*)pTex);
        return 0;
    }

    auto it = gGlesTex.find(pTex);
    const bool clean = it != gGlesTex.end() && it->second.glid &&
                       it->second.palAtUpload == pTex->field_18_pPaltData &&
                       it->second.wAtUpload == w && it->second.hAtUpload == h &&
                       gGlesTexDirty.find(pTex) == gGlesTexDirty.end();
    if (clean)
    {


        const BYTE* src = (const BYTE*)pTex->field_14_original_pixel_data_ptr;
        const size_t n = pTex->field_18_pPaltData ? (size_t)w * h : (size_t)w * h * 2;
        const unsigned cur = fast_content_hash(src, n);
        if (it->second.srcAtUpload == src && it->second.hashAtUpload == cur)
            return it->second.glid;
        if (sFrStaleTex++ < 20)
            __android_log_print(ANDROID_LOG_WARN, LOGT, "TEXSTALE w=%d h=%d pal=%p src=%p->%p h=%08x->%08x glid=%u F%d",
                                w, h, (void*)pTex->field_18_pPaltData,
                                it->second.srcAtUpload, (const void*)src,
                                it->second.hashAtUpload, cur, it->second.glid, gles_frame_seq());
    }



    const unsigned prevGlid = (it != gGlesTex.end()) ? it->second.glid : 0;
    gGlesTexDirty.erase(pTex);

    unsigned char* rgba = (unsigned char*)calloc((size_t)w * h, 4);
    if (!rgba) return 0;

    int opaque = 0;
    if (pTex->field_18_pPaltData)
    {

        const WORD* pal = pTex->field_18_pPaltData;
        const BYTE* src = (const BYTE*)pTex->field_14_original_pixel_data_ptr;
        for (int y = 0; y < h; y++)
        {
            const BYTE* row = src + (size_t)y * 256;
            unsigned char* out = rgba + (size_t)y * w * 4;
            for (int x = 0; x < w; x++)
            {
                const BYTE idx = row[x];
                if (idx == 0) continue;
                const WORD p = pal[idx];
                const u8 r5 = (u8)((p >> 11) & 0x1F), g6 = (u8)((p >> 5) & 0x3F), b5 = (u8)(p & 0x1F);
                out[x * 4 + 0] = (unsigned char)((r5 << 3) | (r5 >> 2));
                out[x * 4 + 1] = (unsigned char)((g6 << 2) | (g6 >> 4));
                out[x * 4 + 2] = (unsigned char)((b5 << 3) | (b5 >> 2));
                out[x * 4 + 3] = 255;
                opaque++;
            }
        }
    }
    else
    {

        const WORD* src16 = (const WORD*)pTex->field_14_original_pixel_data_ptr;
        for (int y = 0; y < h; y++)
        {
            unsigned char* out = rgba + (size_t)y * w * 4;
            for (int x = 0; x < w; x++)
            {
                const WORD px = src16[(size_t)y * w + x];
                const u8 r5 = (u8)((px >> 10) & 0x1F), g5 = (u8)((px >> 5) & 0x1F), b5 = (u8)(px & 0x1F);
                out[x * 4 + 0] = (unsigned char)((r5 << 3) | (r5 >> 2));
                out[x * 4 + 1] = (unsigned char)((g5 << 3) | (g5 >> 2));
                out[x * 4 + 2] = (unsigned char)((b5 << 3) | (b5 >> 2));
                out[x * 4 + 3] = 255;
                opaque++;
            }
        }
    }

    if ((size_t)opaque <= (size_t)w * h / 100)
    {
        sFrBlankTex++;
        if (sFrBlankTex <= 40)
            __android_log_print(ANDROID_LOG_WARN, LOGT, "TEXBLANK w=%d h=%d pal=%p src=%p (opaque=%d/%d) F%d",
                                w, h, (void*)pTex->field_18_pPaltData,
                                (const void*)pTex->field_14_original_pixel_data_ptr, opaque, w * h, gles_frame_seq());
    }



    if ((size_t)opaque < (size_t)w * h)
        gles_alpha_bleed(rgba, w, h);

    unsigned glid = 0;
    if (prevGlid && gles_upload_texture_into(gGlesCtx, prevGlid, w, h, rgba))
        glid = prevGlid;
    if (!glid)
        glid = gles_upload_texture(gGlesCtx, w, h, rgba);
    free(rgba);
    static int sUpN = 0;
    if (glid && (sUpN++ < 40 || (sUpN & 0x7F) == 0))
        __android_log_print(ANDROID_LOG_INFO, LOGT, "TEXUP %5d w=%d h=%d pal=%p -> glid=%u", sUpN - 1, w, h, (void*)pTex->field_18_pPaltData, glid);
    GlesTexRec rec;
    rec.glid = glid;
    rec.palAtUpload = pTex->field_18_pPaltData;
    rec.wAtUpload = w;
    rec.hAtUpload = h;
    if (glid)
    {
        const BYTE* src = (const BYTE*)pTex->field_14_original_pixel_data_ptr;
        rec.srcAtUpload = src;
        rec.hashAtUpload = fast_content_hash(src, pTex->field_18_pPaltData ? (size_t)w * h : (size_t)w * h * 2);
    }
    gGlesTex[pTex] = rec;
    return glid;
}

struct GlesQuadCoeffs { float a, b, c, d, e, f, g, h; bool ok; };

static GlesQuadCoeffs gles_solve_quad(const Vert order[4])
{
    GlesQuadCoeffs q;
    q.ok = false;





    float cx = 0, cy = 0;
    for (int i = 0; i < 4; i++) { cx += order[i].x; cy += order[i].y; }
    cx *= 0.25f; cy *= 0.25f;
    float sc = 0;
    for (int i = 0; i < 4; i++) sc += fabsf(order[i].x - cx) + fabsf(order[i].y - cy);
    sc *= 0.25f;
    if (sc < 1e-3f) sc = 1.0f;

    float m[8][9];
    for (int i = 0; i < 4; i++)
    {
        const float px = (order[i].x - cx) / sc;
        const float py = (order[i].y - cy) / sc;
        const float u = order[i].u, vv = order[i].v;
        for (int k = 0; k < 9; k++) { m[2 * i][k] = 0.0f; m[2 * i + 1][k] = 0.0f; }
        m[2 * i][0] = px;   m[2 * i][1] = py; m[2 * i][2] = 1.0f;
        m[2 * i][6] = -u * px; m[2 * i][7] = -u * py; m[2 * i][8] = u;
        m[2 * i + 1][3] = px; m[2 * i + 1][4] = py; m[2 * i + 1][5] = 1.0f;
        m[2 * i + 1][6] = -vv * px; m[2 * i + 1][7] = -vv * py; m[2 * i + 1][8] = vv;
    }
    for (int col = 0; col < 8; col++)
    {
        int piv = col;
        for (int r = col + 1; r < 8; r++)
            if (fabsf(m[r][col]) > fabsf(m[piv][col])) piv = r;
        const float pv = m[piv][col];
        if (pv == 0.0f) return q;
        if (piv != col)
            for (int k = 0; k < 9; k++) { float t = m[piv][k]; m[piv][k] = m[col][k]; m[col][k] = t; }
        for (int r = 0; r < 8; r++)
        {
            if (r == col) continue;
            const float f = m[r][col] / pv;
            if (f == 0.0f) continue;
            for (int k = col; k < 9; k++) m[r][k] -= f * m[col][k];
        }
    }
    const float A = m[0][8] / m[0][0], B = m[1][8] / m[1][1];
    const float Cn = m[2][8] / m[2][2];
    const float D = m[3][8] / m[3][3], E = m[4][8] / m[4][4];
    const float F = m[5][8] / m[5][5];
    const float G = m[6][8] / m[6][6], H = m[7][8] / m[7][7];





    const float K = sc - G * cx - H * cy;
    if (fabsf(K) < 1e-9f) return q;
    q.a = A / K;
    q.b = B / K;
    q.c = (sc * Cn - A * cx - B * cy) / K;
    q.d = D / K;
    q.e = E / K;
    q.f = (sc * F - D * cx - E * cy) / K;
    q.g = G / K;
    q.h = H / K;
    q.ok = true;
    return q;
}

struct GlesClipV { float x, y, s, t, w; };

static int gles_clip_quad_to_view(const GlesVertex in[4], GlesClipV out[16])
{
    const float e = 2.0f;
    const float xmin = -e, xmax = 640.0f + e, ymin = -e, ymax = 480.0f + e;

    GlesClipV cur[16];
    int n = 4;
    for (int i = 0; i < 4; i++)
    {
        cur[i].x = in[i].x; cur[i].y = in[i].y;
        cur[i].s = in[i].s; cur[i].t = in[i].t; cur[i].w = in[i].w;
    }


    for (int plane = 0; plane < 4; plane++)
    {
        const bool useY = (plane >= 2);
        const bool keepLT = (plane == 0) || (plane == 2);
        const float bound = (plane == 0) ? xmin : (plane == 1) ? xmax : (plane == 2) ? ymin : ymax;
        GlesClipV tmp[16];
        int tn = 0;
        for (int i = 0; i < n; i++)
        {
            const GlesClipV& a = cur[i];
            const GlesClipV& b = cur[(i + 1) % n];
            const float av = useY ? a.y : a.x;
            const float bv = useY ? b.y : b.x;
            const bool inA = keepLT ? (av >= bound) : (av <= bound);
            const bool inB = keepLT ? (bv >= bound) : (bv <= bound);
            if (inA && inB)
            {
                tmp[tn++] = b;
            }
            else if (inA && !inB)
            {
                const float t2 = (bound - av) / (bv - av);
                GlesClipV p;
                p.x = a.x + t2 * (b.x - a.x); p.y = a.y + t2 * (b.y - a.y);
                p.s = a.s + t2 * (b.s - a.s); p.t = a.t + t2 * (b.t - a.t);
                p.w = a.w + t2 * (b.w - a.w);
                tmp[tn++] = p;
            }
            else if (!inA && inB)
            {
                const float t2 = (bound - av) / (bv - av);
                GlesClipV p;
                p.x = a.x + t2 * (b.x - a.x); p.y = a.y + t2 * (b.y - a.y);
                p.s = a.s + t2 * (b.s - a.s); p.t = a.t + t2 * (b.t - a.t);
                p.w = a.w + t2 * (b.w - a.w);
                tmp[tn++] = p;
                tmp[tn++] = b;
            }
        }
        if (tn < 3) return 0;
        for (int i = 0; i < tn; i++) cur[i] = tmp[i];
        n = tn;
        if (n > 16) return 0;
    }

    if (n < 3) return 0;
    for (int i = 0; i < n; i++) out[i] = cur[i];
    return n;
}

static int gles_persp_mode(void)
{
    static int sMode = -1;
    if (sMode == -1)
    {
        FILE* f = fopen("/sdcard/affine.flag", "r");
        sMode = (f != NULL) ? 0 : 1;
        if (f) fclose(f);
        __android_log_print(ANDROID_LOG_WARN, LOGT, "PERSPMODE %s",
                            sMode ? "perspective" : "affine(flag)");
    }
    return sMode;
}

static float gles_build_w(float z)
{
    if (!gles_persp_mode()) return 1.0f;

    return (z > 0.0f && z == z) ? z : 1.0f;
}

static bool gles_quad_w(const Vert order[4], float texW, float texH, float wOut[4])
{
    GlesQuadCoeffs q = gles_solve_quad(order);
    if (!q.ok) return false;
    float wmin = 1e30f, wmax = -1e30f;
    int sign = 0;
    for (int i = 0; i < 4; i++)
    {
        const float den = q.g * order[i].x + q.h * order[i].y + 1.0f;
        if (fabsf(den) < 1e-3f) return false;
        const int s = (den > 0.0f) ? 1 : -1;
        if (i == 0) sign = s;
        else if (s != sign) return false;
        float w = fminf(texW, texH) * den;
        wOut[i] = w;
        if (w < wmin) wmin = w;
        if (w > wmax) wmax = w;
    }
    if (wmax / (wmin > 0 ? wmin : -wmin) > 1e4f) return false;
    return true;
}

static bool gles_is_shadow(const Vert* p, int n)
{
    for (int i = 0; i < n; i++)
        if ((unsigned)p[i].diff != 0xA0000000u) return false;
    return true;
}

static void gles_emit(STexture* pTex, unsigned glid, Vert* pVerts, s32 nverts)
{
    const int texW = pTex->field_E_width;
    const int texH = pTex->field_10_height;
    if (texW <= 0 || texH <= 0) return;
    const float invW = 1.0f / (float)texW, invH = 1.0f / (float)texH;
    const float hw = 0.5f * invW, hh = 0.5f * invH;


    if (nverts != 3 && nverts != 4)
    {
        static int sPolyN = 0;
        if (sPolyN < 16)
        {
            sPolyN++;
            __android_log_print(ANDROID_LOG_WARN, LOGT,
                "POLYDROP nverts=%d tex=%dx%d xy0=(%.0f,%.0f)", (int)nverts, texW, texH, pVerts[0].x, pVerts[0].y);
        }
        return;
    }

    if (nverts == 3)
    {
        sFrTris++;

        static int sTriUV = 0;
        if (sTriUV < 10)
        {
            for (int i = 0; i < 3; i++)
            {
                if (pVerts[i].u < 0 || pVerts[i].u > texW || pVerts[i].v < 0 || pVerts[i].v > texH)
                {
                    sTriUV++;
                    __android_log_print(ANDROID_LOG_WARN, LOGT,
                        "TRIUV tex=%dx%d v0=(%.1f,%.1f) v1=(%.1f,%.1f) v2=(%.1f,%.1f) xy=(%.0f,%.0f)",
                        texW, texH,
                        pVerts[0].u, pVerts[0].v, pVerts[1].u, pVerts[1].v,
                        pVerts[2].u, pVerts[2].v,
                        pVerts[0].x, pVerts[0].y);
                    break;
                }
            }
        }
        GlesVertex gv[3];
        for (int i = 0; i < 3; i++)
        {
            gv[i].x = pVerts[i].x; gv[i].y = pVerts[i].y;
            gv[i].s = pVerts[i].u * invW + hw;
            gv[i].t = pVerts[i].v * invH + hh;
            gv[i].w = gles_build_w(pVerts[i].z);
        }
        if (gles_is_shadow(pVerts, 3))
            gles_draw_poly_shadow(gGlesCtx, glid, gv, 3);
        else
            gles_draw_poly(gGlesCtx, glid, gv, 3);
        sFrTriDrawn++;
        return;
    }
    if (nverts != 4) return;
    sFrQuads++;

    const bool isShadow = gles_is_shadow(pVerts, 4);
    if (isShadow) sFrShadows++;


    static int sBandN = 0;
    if (sBandN < 10)
    {
        float mnx = pVerts[0].x, mxx = pVerts[0].x, mny = pVerts[0].y, mxy = pVerts[0].y;
        for (int i = 1; i < 4; i++)
        {
            if (pVerts[i].x < mnx) mnx = pVerts[i].x;
            if (pVerts[i].x > mxx) mxx = pVerts[i].x;
            if (pVerts[i].y < mny) mny = pVerts[i].y;
            if (pVerts[i].y > mxy) mxy = pVerts[i].y;
        }
        if ((mxy - mny) > 240.0f && (mxx - mnx) < 150.0f)
        {
            sBandN++;
            __android_log_print(ANDROID_LOG_WARN, LOGT,
                "BAND tex=%dx%d uv=(%.1f,%.1f)(%.1f,%.1f)(%.1f,%.1f)(%.1f,%.1f) xy=(%.0f,%.0f)..(%.0f,%.0f)",
                texW, texH,
                pVerts[0].u, pVerts[0].v, pVerts[1].u, pVerts[1].v,
                pVerts[2].u, pVerts[2].v, pVerts[3].u, pVerts[3].v,
                mnx, mny, mxx, mxy);
        }
    }


    static int sWDiag = 0;
    if (sWDiag < 10)
    {
        sWDiag++;
        __android_log_print(ANDROID_LOG_WARN, LOGT,
            "WDIAG tex=%dx%d n=%d w=(%.4f,%.4f,%.4f,%.4f) diff=(%08x,%08x,%08x,%08x) xy=(%.0f,%.0f)..(%.0f,%.0f)",
            texW, texH, (int)nverts,
            pVerts[0].w, pVerts[1].w, pVerts[2].w, pVerts[3].w,
            pVerts[0].diff, pVerts[1].diff, pVerts[2].diff, pVerts[3].diff,
            pVerts[0].x, pVerts[0].y, pVerts[2].x, pVerts[2].y);
    }


    static int sUVout = 0;
    if (sUVout < 14)
    {
        for (int i = 0; i < 4; i++)
        {
            if (pVerts[i].u < 0 || pVerts[i].u > texW || pVerts[i].v < 0 || pVerts[i].v > texH)
            {
                sUVout++;
                __android_log_print(ANDROID_LOG_WARN, LOGT,
                    "UVOUT tex=%dx%d v0=(%.1f,%.1f) v1=(%.1f,%.1f) v2=(%.1f,%.1f) v3=(%.1f,%.1f) xy=(%.0f,%.0f..%.0f,%.0f)",
                    texW, texH,
                    pVerts[0].u, pVerts[0].v, pVerts[1].u, pVerts[1].v,
                    pVerts[2].u, pVerts[2].v, pVerts[3].u, pVerts[3].v,
                    pVerts[0].x, pVerts[0].y, pVerts[2].x, pVerts[2].y);
                break;
            }
        }
    }



    {
        bool allZero = true;
        for (int i = 0; i < 4; i++)
            if (fabsf(pVerts[i].u) > 0.01f || fabsf(pVerts[i].v) > 0.01f) { allZero = false; break; }
        if (allZero)
        {
            float mnx = pVerts[0].x, mxx = pVerts[0].x, mny = pVerts[0].y, mxy = pVerts[0].y;
            for (int i = 1; i < 4; i++)
            {
                if (pVerts[i].x < mnx) mnx = pVerts[i].x;
                if (pVerts[i].x > mxx) mxx = pVerts[i].x;
                if (pVerts[i].y < mny) mny = pVerts[i].y;
                if (pVerts[i].y > mxy) mxy = pVerts[i].y;
            }
            sFrZeroUv++;
            if (sFrZeroUv <= 8 && mnx < 640 && mxx > 0 && mny < 480 && mxy > 0)
                __android_log_print(ANDROID_LOG_WARN, LOGT,
                    "UVZERO-VISIBLE tex=%dx%d xy=(%.0f,%.0f)..(%.0f,%.0f) nv=%d flags=0x%x F%d",
                    texW, texH, mnx, mny, mxx, mxy, nverts, gsLastDrawFlags, gles_frame_seq());
        }
    }


    const float cx = (pVerts[0].x + pVerts[1].x + pVerts[2].x + pVerts[3].x) * 0.25f;
    const float cy = (pVerts[0].y + pVerts[1].y + pVerts[2].y + pVerts[3].y) * 0.25f;
    int idx[4] = { 0, 1, 2, 3 };
    for (int a = 1; a < 4; a++)
    {
        const float aa = atan2f(pVerts[idx[a]].y - cy, pVerts[idx[a]].x - cx);
        int b = a - 1;
        while (b >= 0)
        {
            const float bb = atan2f(pVerts[idx[b]].y - cy, pVerts[idx[b]].x - cx);
            if (bb <= aa) break;
            const int t = idx[b]; idx[b] = idx[b + 1]; idx[b + 1] = t;
            b--;
        }
    }
    Vert order[4];
    for (int i = 0; i < 4; i++) order[i] = pVerts[idx[i]];








    bool useAffine = true;

    GlesVertex gv[4];
    float qw[4];
    const bool useQuadW = gles_quad_w(order, (float)texW, (float)texH, qw);
    for (int i = 0; i < 4; i++)
    {
        gv[i].x = order[i].x;
        gv[i].y = order[i].y;
        gv[i].s = order[i].u * invW + hw;
        gv[i].t = order[i].v * invH + hh;
        gv[i].w = useQuadW ? qw[i] : gles_build_w(order[i].z);
    }




    {
        static int sDn = 0;
        if (sDn < 60)
        {
            float mnx = order[0].x, mxx = order[0].x, mny = order[0].y, mxy = order[0].y;
            for (int i = 1; i < 4; i++)
            {
                if (order[i].x < mnx) mnx = order[i].x;
                if (order[i].x > mxx) mxx = order[i].x;
                if (order[i].y < mny) mny = order[i].y;
                if (order[i].y > mxy) mxy = order[i].y;
            }
            if (mnx >= 120.0f && mxx <= 520.0f && mny >= 60.0f && mxy <= 420.0f)
            {
                sDn++;
                int quad = 1;
                for (int i = 0; i < 4; i++)
                {
                    const float uu = order[i].u, vv = order[i].v;
                    const bool uok = (fabsf(uu) < 0.1f) || (fabsf(uu - 32.0f) < 0.1f) || (fabsf(uu - 63.999901f) < 0.1f);
                    const bool vok = (fabsf(vv) < 0.1f) || (fabsf(vv - 32.0f) < 0.1f) || (fabsf(vv - 63.999901f) < 0.1f);
                    if (!uok || !vok) quad = 0;
                }
                __android_log_print(ANDROID_LOG_WARN, LOGT,
                    "QUADZZ F%d tex=%dx%d uv=(%.2f,%.2f)(%.2f,%.2f)(%.2f,%.2f)(%.2f,%.2f) z=(%.4f,%.4f,%.4f,%.4f) xy=(%.0f,%.0f)..(%.0f,%.0f) q%d=%d",
                    gles_frame_seq(), texW, texH,
                    order[0].u, order[0].v, order[1].u, order[1].v,
                    order[2].u, order[2].v, order[3].u, order[3].v,
                    order[0].z, order[1].z, order[2].z, order[3].z,
mnx, mny, mxx, mxy, texW, quad);
            }
        }
    }
    if (useAffine)
    {
        static int sAffN = 0;
        sAffN++;
        if ((sAffN - 1) % 200 == 0 && (sAffN - 1) / 200 < 8)
            __android_log_print(ANDROID_LOG_WARN, LOGT, "AFFBK total=%d tex=%dx%d xy=(%.0f,%.0f)..(%.0f,%.0f)",
                sAffN, texW, texH, order[0].x, order[0].y, order[2].x, order[2].y);
    }


    static int sGiantN = 0;
    if (sGiantN < 10)
    {
        float mnx = order[0].x, mxx = order[0].x, mny = order[0].y, mxy = order[0].y;
        for (int i = 1; i < 4; i++)
        {
            if (order[i].x < mnx) mnx = order[i].x;
            if (order[i].x > mxx) mxx = order[i].x;
            if (order[i].y < mny) mny = order[i].y;
            if (order[i].y > mxy) mxy = order[i].y;
        }
        const float bw = mxx - mnx, bh = mxy - mny;
        if (bw * bh > 0.35f * 640.0f * 480.0f)
        {
            sGiantN++;
            __android_log_print(ANDROID_LOG_WARN, LOGT,
                "GIANT tex=%dx%d bbox=%.0fx%.0f xy=(%.0f,%.0f)..(%.0f,%.0f) uv=(%.1f,%.1f)(%.1f,%.1f)(%.1f,%.1f)(%.1f,%.1f) aff=%d",
                texW, texH, bw, bh, mnx, mny, mxx, mxy,
                order[0].u, order[0].v, order[1].u, order[1].v,
                order[2].u, order[2].v, order[3].u, order[3].v, (int)useAffine);
        }
    }


    static int sWDeg = 0;
    if (sWDeg < 12 && false)
    {
        float wmin = 1e9f, wmax = -1e9f;
        for (int i = 0; i < 4; i++)
        {
            const float w = gv[i].w;
            if (w < wmin) wmin = w;
            if (w > wmax) wmax = w;
        }
        if (wmin < 0.25f)
        {
            sWDeg++;
            __android_log_print(ANDROID_LOG_WARN, LOGT,
                "WDEG tex=%dx%d wmin=%.4f wmax=%.4f xy=(%.0f,%.0f)..(%.0f,%.0f)",
                texW, texH, wmin, wmax,
                order[0].x, order[0].y, order[2].x, order[2].y);
        }
    }







    GlesClipV cl[16];
    const int cn = gles_clip_quad_to_view(gv, cl);
    if (cn > 0)
    {
        for (int i = 1; i + 1 < cn; i++)
        {
            GlesVertex t[3];
            t[0].x = cl[0].x; t[0].y = cl[0].y; t[0].s = cl[0].s; t[0].t = cl[0].t; t[0].w = cl[0].w;
            t[1].x = cl[i].x; t[1].y = cl[i].y; t[1].s = cl[i].s; t[1].t = cl[i].t; t[1].w = cl[i].w;
            t[2].x = cl[i + 1].x; t[2].y = cl[i + 1].y; t[2].s = cl[i + 1].s; t[2].t = cl[i + 1].t; t[2].w = cl[i + 1].w;
            if (isShadow)
                gles_draw_poly_shadow(gGlesCtx, glid, t, 3);
            else
                gles_draw_poly(gGlesCtx, glid, t, 3);
            sFrTriDrawn++;
        }
    }
    else
    {


        float mnx = order[0].x, mxx = order[0].x, mny = order[0].y, mxy = order[0].y;
        for (int i = 1; i < 4; i++)
        {
            if (order[i].x < mnx) mnx = order[i].x;
            if (order[i].x > mxx) mxx = order[i].x;
            if (order[i].y < mny) mny = order[i].y;
            if (order[i].y > mxy) mxy = order[i].y;
        }
        if (mnx < 640.0f && mxx > 0.0f && mny < 480.0f && mxy > 0.0f)
        {
            sFrClip0++;
            if (sFrClip0 <= 12)
                __android_log_print(ANDROID_LOG_WARN, LOGT,
                    "CLIPZERO-VISIBLE tex=%dx%d aff=%d uv=(%.1f,%.1f)(%.1f,%.1f)(%.1f,%.1f)(%.1f,%.1f) xy=(%.0f,%.0f)..(%.0f,%.0f) flags=0x%x F%d",
                    texW, texH, (int)useAffine,
                    order[0].u, order[0].v, order[1].u, order[1].v,
                    order[2].u, order[2].v, order[3].u, order[3].v,
                    mnx, mny, mxx, mxy, gsLastDrawFlags, gles_frame_seq());
        }
    }



    static int sDumpOn = 0;
    static struct timespec sDumpStart = {};
    static struct timespec sLastCheck = {};
    if (!sDumpOn)
    {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double since = (double)(now.tv_sec - sLastCheck.tv_sec) * 1e9 +
                       (double)(now.tv_nsec - sLastCheck.tv_nsec);
        if (since > 300e6)
        {
            sLastCheck = now;
            FILE* df = fopen("/sdcard/dump.flag", "r");
            if (df) { fclose(df); remove("/sdcard/dump.flag"); sDumpOn = 1; sDumpStart = now; }
        }
    }
    if (sDumpOn && cn > 0)
    {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double dt = (double)(now.tv_sec - sDumpStart.tv_sec) * 1e9 + (double)(now.tv_nsec - sDumpStart.tv_nsec);
        if (dt > 600e6) sDumpOn = 0;
        else
        {
            float bx0 = cl[0].x, bx1 = cl[0].x, by0 = cl[0].y, by1 = cl[0].y;
            for (int i = 1; i < cn; i++)
            {
                bx0 = fminf(bx0, cl[i].x); bx1 = fmaxf(bx1, cl[i].x);
                by0 = fminf(by0, cl[i].y); by1 = fmaxf(by1, cl[i].y);
            }
            static int sDumpN = 0;
            if (sDumpN++ < 4000)
                __android_log_print(ANDROID_LOG_WARN, LOGT,
                    "BANDQ F%d tex=%dx%d bbox=(%.0f,%.0f)..(%.0f,%.0f) uv=(%.1f,%.1f)(%.1f,%.1f)(%.1f,%.1f)(%.1f,%.1f) aff=%d x0hw=%d",
                    gles_frame_seq(), texW, texH, bx0, by0, bx1, by1,
                    order[0].u, order[0].v, order[1].u, order[1].v,
                    order[2].u, order[2].v, order[3].u, order[3].v,
                    (int)useAffine, nverts);
        }
    }

    static int sBigN = 0;
    if (sBigN < 25)
    {
        float mnx = gv[0].x, mxx = gv[0].x, mny = gv[0].y, mxy = gv[0].y;
        float mns = gv[0].s, mxs = gv[0].s, mnt = gv[0].t, mxt = gv[0].t;
        for (int i = 1; i < 4; i++)
        {
            mnx = fminf(mnx, gv[i].x); mxx = fmaxf(mxx, gv[i].x);
            mny = fminf(mny, gv[i].y); mxy = fmaxf(mxy, gv[i].y);
            mns = fminf(mns, gv[i].s); mxs = fmaxf(mxs, gv[i].s);
            mnt = fminf(mnt, gv[i].t); mxt = fmaxf(mxt, gv[i].t);
        }
        const float area = (mxx - mnx) * (mxy - mny);
        const bool bigScreen = area > 180000.0f;
        const bool bigUV = (mxs - mns) * texW > 4.0f * texW || (mxt - mnt) * texH > 4.0f * texH;
        if (bigScreen || bigUV)
        {
            sBigN++;
            __android_log_print(ANDROID_LOG_WARN, LOGT, "BIGQUAD n=%d tex=%dx%d pal=%p bbox=%.0fx%.0f src=(%.0f,%.0f)..(%.0f,%.0f) uvRng=(%.2f,%.2f)",
                                sBigN, texW, texH, (void*)pTex->field_18_pPaltData,
                                mxx - mnx, mxy - mny, mnx, mny, mxx, mxy, (mxs - mns) * texW, (mxt - mnt) * texH);
        }
    }
}

static void CC gbh_soft_quad(STexture* pTexture, Vert* pVerts, s32 vertsCount)
{
    if (!pTexture || !pTexture->field_14_original_pixel_data_ptr)
    {
        static int sNoPtr = 0;
        sNoPtr++;
        if (sNoPtr < 64 || (sNoPtr % 5000) == 0)
            __android_log_print(ANDROID_LOG_WARN, LOGT, "NOPTR n=%d w=%d h=%d pal=%p nv=%d",
                sNoPtr, pTexture ? pTexture->field_E_width : -1, pTexture ? pTexture->field_10_height : -1,
                pTexture ? (void*)pTexture->field_18_pPaltData : nullptr, vertsCount);
        return;
    }


    if (gles_ensure_context())
    {
        const unsigned glid = gles_ensure_texture(pTexture);
        if (glid)
        {
            gles_emit(pTexture, glid, pVerts, vertsCount);
            return;
        }
        static int sTexMiss = 0;
        sTexMiss++;
        if (sTexMiss < 64 || (sTexMiss % 5000) == 0)
            __android_log_print(ANDROID_LOG_WARN, LOGT, "TEXMISS n=%d w=%d h=%d pal=%p nv=%d",
                sTexMiss, pTexture->field_E_width, pTexture->field_10_height,
                (void*)pTexture->field_18_pPaltData, vertsCount);
    }


    if (!gFramebuffer) return;

    const s32 texW = pTexture->field_E_width;
    const s32 texH = pTexture->field_10_height;
    if (texW <= 0 || texH <= 0) return;

    const BYTE* pSrc = (const BYTE*)pTexture->field_14_original_pixel_data_ptr;
    const WORD* pPal = pTexture->field_18_pPaltData;

    if (vertsCount == 4)
    {




        float cx = (pVerts[0].x + pVerts[1].x + pVerts[2].x + pVerts[3].x) * 0.25f;
        float cy = (pVerts[0].y + pVerts[1].y + pVerts[2].y + pVerts[3].y) * 0.25f;
        int idx[4] = { 0, 1, 2, 3 };
        for (int a = 1; a < 4; a++)
        {
            float aa = atan2f(pVerts[idx[a]].y - cy, pVerts[idx[a]].x - cx);
            int b = a - 1;
            while (b >= 0)
            {
                float bb = atan2f(pVerts[idx[b]].y - cy, pVerts[idx[b]].x - cx);
                if (bb <= aa) break;
                int t = idx[b]; idx[b] = idx[b + 1]; idx[b + 1] = t;
                b--;
            }
        }
        Vert order[4];
        for (int i = 0; i < 4; i++) order[i] = pVerts[idx[i]];




        float m[8][9];
        const Vert* v = order;
        for (int i = 0; i < 4; i++)
        {
            const float px = v[i].x, py = v[i].y;
            const float u = v[i].u, vv = v[i].v;


            for (int k = 0; k < 9; k++) m[2 * i][k] = 0.0f;
            for (int k = 0; k < 9; k++) m[2 * i + 1][k] = 0.0f;
            m[2 * i][0] = px;     m[2 * i][1] = py;     m[2 * i][2] = 1.0f;
            m[2 * i][6] = -u * px; m[2 * i][7] = -u * py;
            m[2 * i][8] = u;
            m[2 * i + 1][3] = px;  m[2 * i + 1][4] = py;  m[2 * i + 1][5] = 1.0f;
            m[2 * i + 1][6] = -vv * px; m[2 * i + 1][7] = -vv * py;
            m[2 * i + 1][8] = vv;
        }

        bool affineOk = true;
        for (int col = 0; col < 8; col++)
        {
            int piv = col;
            for (int r = col + 1; r < 8; r++)
            {
                if (fabsf(m[r][col]) > fabsf(m[piv][col])) piv = r;
            }
            const float pv = m[piv][col];
            if (pv == 0.0f) { affineOk = false; break; }
            if (piv != col)
            {
                for (int k = 0; k < 9; k++) { float t = m[piv][k]; m[piv][k] = m[col][k]; m[col][k] = t; }
            }
            for (int r = 0; r < 8; r++)
            {
                if (r == col) continue;
                const float f = m[r][col] / pv;
                if (f == 0.0f) continue;
                for (int k = col; k < 9; k++) m[r][k] -= f * m[col][k];
            }
        }
        if (!affineOk)
        {

            const Vert& A = order[0];
            const Vert& B = order[1];
            const Vert& C = order[2];
            const Vert& D = order[3];
            for (int pass = 0; pass < 2; pass++)
            {
                const Vert *pA, *pB, *pC;
                if (pass == 0) { pA = &A; pB = &B; pC = &C; }
                else           { pA = &A; pB = &C; pC = &D; }
                int ix0 = (int)std::min(std::min(pA->x, pB->x), pC->x); if (ix0 < 0) ix0 = 0;
                int iy0 = (int)std::min(std::min(pA->y, pB->y), pC->y); if (iy0 < 0) iy0 = 0;
                int ix1 = (int)std::max(std::max(pA->x, pB->x), pC->x) + 1; if (ix1 > gFBW) ix1 = gFBW;
                int iy1 = (int)std::max(std::max(pA->y, pB->y), pC->y) + 1; if (iy1 > gFBH) iy1 = gFBH;
                for (int y = iy0; y < iy1; y++)
                {
                    u16* row = gFramebuffer + (size_t)y * gFBStride;
                    for (int x = ix0; x < ix1; x++)
                    {
                        const float e0x = pB->x - pA->x, e0y = pB->y - pA->y;
                        const float e1x = pC->x - pA->x, e1y = pC->y - pA->y;
                        const float det = e0x * e1y - e1x * e0y;
                        if (det == 0.0f) continue;
                        const float dx = (float)(x + 0.5f) - pA->x;
                        const float dy = (float)(y + 0.5f) - pA->y;
                        const float s = (dx * e1y - e1x * dy) / det;
                        const float t = (e0x * dy - dx * e0y) / det;
                        if (s < 0 || t < 0 || s + t > 1) continue;
                        const float u = pA->u + s * (pB->u - pA->u) + t * (pC->u - pA->u);
                        const float vv2 = pA->v + s * (pB->v - pA->v) + t * (pC->v - pA->v);
                        const int col = (int)u;
                        const int rw = (int)vv2;
                        if (col < 0 || col >= texW || rw < 0 || rw >= texH) continue;
                        if (pPal)
                        {
                            const size_t srcStride = 256;
                            BYTE idx = pSrc[(size_t)rw * srcStride + col];
                            if (idx == 0) continue;
                            row[x] = pPal[idx];
                        }
                        else
                        {
                            WORD px = ((const WORD*)pSrc)[(size_t)rw * texW + col];
                            u8 r5 = (u8)((px >> 10) & 0x1F);
                            u8 g5 = (u8)((px >> 5) & 0x1F);
                            u8 b5 = (u8)(px & 0x1F);
                            u16 g6 = (u16)((g5 << 1) | (g5 >> 4));
                            row[x] = (u16)((r5 << 11) | (g6 << 5) | b5);
                        }
                    }
                }
            }
            return;
        }
        float a = m[0][8] / m[0][0];
        float b = m[1][8] / m[1][1];
        float c = m[2][8] / m[2][2];
        float d = m[3][8] / m[3][3];
        float e = m[4][8] / m[4][4];
        float f_ = m[5][8] / m[5][5];
        float g = m[6][8] / m[6][6];
        float h = m[7][8] / m[7][7];

        {
        const Vert& p0 = order[0];
        const Vert& p1 = order[1];
        const Vert& p2 = order[2];
        const Vert& p3 = order[3];
        float minX = std::min(std::min(p0.x, p1.x), std::min(p2.x, p3.x));
        float maxX = std::max(std::max(p0.x, p1.x), std::max(p2.x, p3.x));
        float minY = std::min(std::min(p0.y, p1.y), std::min(p2.y, p3.y));
        float maxY = std::max(std::max(p0.y, p1.y), std::max(p2.y, p3.y));
        int ix0 = (int)minX; if (ix0 < 0) ix0 = 0;
        int iy0 = (int)minY; if (iy0 < 0) iy0 = 0;
        int ix1 = (int)maxX + 1; if (ix1 > gFBW) ix1 = gFBW;
        int iy1 = (int)maxY + 1; if (iy1 > gFBH) iy1 = gFBH;


        const float ex0 = p1.x - p0.x, ey0 = p1.y - p0.y;
        const float ex1 = p2.x - p1.x, ey1 = p2.y - p1.y;
        const float ex2 = p3.x - p2.x, ey2 = p3.y - p2.y;
        const float ex3 = p0.x - p3.x, ey3 = p0.y - p3.y;

        for (int y = iy0; y < iy1; y++)
        {
            const float fy = (float)(y + 0.5f);
            u16* row = gFramebuffer + (size_t)y * gFBStride;
            for (int x = ix0; x < ix1; x++)
            {
                const float fx = (float)(x + 0.5f);
                const float q0x = fx - p0.x, q0y = fy - p0.y;
                const float q1x = fx - p1.x, q1y = fy - p1.y;
                const float q2x = fx - p2.x, q2y = fy - p2.y;
                const float q3x = fx - p3.x, q3y = fy - p3.y;
                const float cr0 = ex0 * q0y - ey0 * q0x;
                const float cr1 = ex1 * q1y - ey1 * q1x;
                const float cr2 = ex2 * q2y - ey2 * q2x;
                const float cr3 = ex3 * q3y - ey3 * q3x;
                const bool b0 = cr0 >= -0.5f, b1 = cr1 >= -0.5f, b2 = cr2 >= -0.5f, b3 = cr3 >= -0.5f;
                const bool c0 = cr0 <= 0.5f, c1 = cr1 <= 0.5f, c2 = cr2 <= 0.5f, c3 = cr3 <= 0.5f;
                if (!((b0 && b1 && b2 && b3) || (c0 && c1 && c2 && c3))) continue;

                const float den = g * fx + h * fy + 1.0f;
                if (den == 0.0f) continue;
                const float u = (a * fx + b * fy + c) / den;
                const float vv2 = (d * fx + e * fy + f_) / den;
                const int col = (int)u;
                const int rw = (int)vv2;
                if (col < 0 || col >= texW || rw < 0 || rw >= texH) continue;

                if (pPal)
                {


                    const size_t srcStride = 256;
                    BYTE idx = pSrc[(size_t)rw * srcStride + col];
                    if (idx == 0) continue;
                    row[x] = pPal[idx];
                }
                else
                {


                    WORD px = ((const WORD*)pSrc)[(size_t)rw * texW + col];
                    u8 r5 = (u8)((px >> 10) & 0x1F);
                    u8 g5 = (u8)((px >> 5) & 0x1F);
                    u8 b5 = (u8)(px & 0x1F);
                    u16 g6 = (u16)((g5 << 1) | (g5 >> 4));
                    row[x] = (u16)((r5 << 11) | (g6 << 5) | b5);
                }
            }
        }
        }
        return;
    }


    if (vertsCount == 3)
    {
        const Vert& A = pVerts[0];
        const Vert& B = pVerts[1];
        const Vert& C = pVerts[2];
        int ix0 = (int)std::min(std::min(A.x, B.x), C.x); if (ix0 < 0) ix0 = 0;
        int iy0 = (int)std::min(std::min(A.y, B.y), C.y); if (iy0 < 0) iy0 = 0;
        int ix1 = (int)std::max(std::max(A.x, B.x), C.x) + 1; if (ix1 > gFBW) ix1 = gFBW;
        int iy1 = (int)std::max(std::max(A.y, B.y), C.y) + 1; if (iy1 > gFBH) iy1 = gFBH;

        for (int y = iy0; y < iy1; y++)
        {
            u16* row = gFramebuffer + (size_t)y * gFBStride;
            for (int x = ix0; x < ix1; x++)
            {
                const float e0x = B.x - A.x, e0y = B.y - A.y;
                const float e1x = C.x - A.x, e1y = C.y - A.y;
                const float det = e0x * e1y - e1x * e0y;
                if (det == 0.0f) continue;
                const float dx = (float)(x + 0.5f) - A.x;
                const float dy = (float)(y + 0.5f) - A.y;
                const float s = (dx * e1y - e1x * dy) / det;
                const float t = (e0x * dy - dx * e0y) / det;
                if (s < 0 || t < 0 || s + t > 1) continue;

                const float u = A.u + s * (B.u - A.u) + t * (C.u - A.u);
                const float v = A.v + s * (B.v - A.v) + t * (C.v - A.v);

                const int col = (int)u;
                const int rw = (int)v;
                if (col < 0 || col >= texW || rw < 0 || rw >= texH) continue;

                if (pPal)
                {
                    const size_t srcStride = 256;
                    BYTE idx = pSrc[(size_t)rw * srcStride + col];
                    if (idx == 0) continue;
                    row[x] = pPal[idx];
                }
                else
                {
                    WORD px = ((const WORD*)pSrc)[(size_t)rw * texW + col];
                    u8 r5 = (u8)((px >> 10) & 0x1F);
                    u8 g5 = (u8)((px >> 5) & 0x1F);
                    u8 b5 = (u8)(px & 0x1F);
                    u16 g6 = (u16)((g5 << 1) | (g5 >> 4));
                    row[x] = (u16)((r5 << 11) | (g6 << 5) | b5);
                }
            }
        }
        return;
    }
}

static void CC gbh_real_drawquad(s32 flags, STexture* pTexture, Vert* pVerts, s32 baseColour)
{
    (void)baseColour;
    gsLastDrawFlags = (u32)flags;
    static int sQuadN = 0;
    if (sQuadN < 3)
    {
        sQuadN++;
        __android_log_print(ANDROID_LOG_WARN, "GTA2-quad", "quad %d tex=%dx%d pal=%p u/v t0=%f,%f t1=%f,%f t2=%f,%f t3=%f,%f  x0=%f y0=%f",
                            sQuadN,
                            pTexture ? (int)pTexture->field_E_width : -1,
                            pTexture ? (int)pTexture->field_10_height : -1,
                            (void*)(pTexture ? pTexture->field_18_pPaltData : 0),
                            pVerts ? pVerts[0].u : -1, pVerts ? pVerts[0].v : -1,
                            pVerts ? pVerts[1].u : -1, pVerts ? pVerts[1].v : -1,
                            pVerts ? pVerts[2].u : -1, pVerts ? pVerts[2].v : -1,
                            pVerts ? pVerts[3].u : -1, pVerts ? pVerts[3].v : -1,
                            pVerts ? pVerts[0].x : -1, pVerts ? pVerts[0].y : -1);
    }
    if (!pVerts) return;
    gbh_soft_quad(pTexture, pVerts, 4);
}

static void CC gbh_real_drawquadclipped(s32, s32, s32, s32, s32)
{
}

static int CC gbh_real_drawtile(u32 flags, STexture* pTexture, Vert* pData, s32 diffuseColour)
{
    (void)diffuseColour;
    if (!pData) return 0;
    gsLastDrawFlags = flags;
    static int sTileN = 0;
    if (sTileN == 200 || sTileN == 600 || sTileN == 1200)
    {
        sTileN++;
        __android_log_print(ANDROID_LOG_WARN, "GTA2-tile", "tile[%d] tex=%dx%d pal=%p  v0=(%.1f,%.1f)u=%.1f,v=%.1f  v1=(%.1f,%.1f)u=%.1f,v=%.1f  v2=(%.1f,%.1f)u=%.1f,v=%.1f  v3=(%.1f,%.1f)u=%.1f,v=%.1f",
                            sTileN - 1,
                            pTexture ? (int)pTexture->field_E_width : -1,
                            pTexture ? (int)pTexture->field_10_height : -1,
                            (void*)(pTexture ? pTexture->field_18_pPaltData : 0),
                            pData[0].x, pData[0].y, pData[0].u, pData[0].v,
                            pData[1].x, pData[1].y, pData[1].u, pData[1].v,
                            pData[2].x, pData[2].y, pData[2].u, pData[2].v,
                            pData[3].x, pData[3].y, pData[3].u, pData[3].v);
    }
    else sTileN++;
    gbh_soft_quad(pTexture, pData, 4);
    return 0;
}

static s32 CC gbh_real_drawtilepart(u32 flags, STexture* pTexture, Vert* pData, s32 diffuseColour)
{
    (void)diffuseColour;
    if (!pData) return 0;
    gsLastDrawFlags = flags;
    gbh_soft_quad(pTexture, pData, 4);
    return 0;
}

static void CC gbh_real_drawtriangle(s32 triFlags, STexture* pTexture, Vert* pVerts, u8& diffuseColour)
{
    gbh_soft_quad(pTexture, pVerts, 3);
}

static s32 CC gbh_real_drawflatrect(s32, s32)
{
    return 0;
}

static s32 CC gbh_real_printbitmap(s32, s32)
{
    return 0;
}

static s32 CC gbh_real_blitbuffer(s32, s32, s32, s32, s32, s32)
{
    return 0;
}

static s32 CC gbh_real_convertingedge()
{
    return 0;
}

static s32 CC gbh_real_loadimage(SImage* pToLoad)
{
    if (!pToLoad || !gGbhImageTable) return -1;

    s32 freeIndex = -1;
    for (u32 i = 0; i < gGbhImageTableCount; i++)
    {
        if (!gGbhImageTable[i].bLoaded) { freeIndex = i; break; }
    }
    if (freeIndex == -1) return -1;

    if (pToLoad->field_1 != 0 || pToLoad->field_2 != 2 || pToLoad->field_10 != 16)
    {
        __android_log_print(ANDROID_LOG_WARN, LOGT, "gbh_LoadImage: unsupported tga f1=%d f2=%d bpp=%d",
                            pToLoad->field_1, pToLoad->field_2, pToLoad->field_10);
        return -2;
    }

    __android_log_print(ANDROID_LOG_INFO, LOGT, "gbh_LoadImage idx=%d w=%d h=%d", freeIndex,
                        pToLoad->field_C_width, pToLoad->field_E_height);




    const u32 w = pToLoad->field_C_width;
    const u32 h = pToLoad->field_E_height;
    const BYTE* pSrc = (const BYTE*)&pToLoad->field_12;
    WORD* pOut = (WORD*)malloc((size_t)w * h * 2);
    if (!pOut) return -3;
    for (u32 y = 0; y < h; y++)
    {
        memcpy(&pOut[(h - 1 - y) * w], &pSrc[(size_t)y * w * 2], (size_t)w * 2);
    }

    STexture* pTex = gbh_real_registertexture(w, h, (BYTE*)pOut, -1, 0);
    if (!pTex)
    {
        free(pOut);
        return -3;
    }

    gGbhImageTable[freeIndex].bLoaded = 1;
    gGbhImageTable[freeIndex].field_4_w = w;
    gGbhImageTable[freeIndex].field_8_h = h;
    gGbhImageTable[freeIndex].field_C_pTexture = pTex;
    gGbhImageTable[freeIndex].field_10_pData = pOut;
    return freeIndex;
}

static s32 CC gbh_real_blitimage(s32 imageIndex, s32 srcLeft, s32 srcTop, s32 srcRight, s32 srcBottom, s32 dstX, s32 dstY)
{
    if (!gGbhImageTable)
    {
        __android_log_print(ANDROID_LOG_WARN, LOGT, "gbh_BlitImage: no table idx=%d", imageIndex);
        return -1;
    }
    if (imageIndex < 0 || imageIndex >= (s32)gGbhImageTableCount)
    {
        __android_log_print(ANDROID_LOG_WARN, LOGT, "gbh_BlitImage: invalid idx %d", imageIndex);
        return -1;
    }
    if (!gGbhImageTable[imageIndex].bLoaded || !gGbhImageTable[imageIndex].field_C_pTexture)
    {

        __android_log_print(ANDROID_LOG_WARN, LOGT, "gbh_BlitImage: not loaded idx=%d", imageIndex);
        return -10;
    }

    __android_log_print(ANDROID_LOG_INFO, LOGT, "gbh_BlitImage idx=%d (%d,%d)-(%d,%d)->(%d,%d)", imageIndex,
                        srcLeft, srcTop, srcRight, srcBottom, dstX, dstY);
    Vert verts[4];
    memset(verts, 0, sizeof(verts));

    verts[0].x = (f32)dstX;            verts[0].y = (f32)dstY;            verts[0].u = (f32)srcLeft;  verts[0].v = (f32)srcTop;
    verts[1].x = (f32)(dstX + (srcRight - srcLeft)); verts[1].y = (f32)dstY;  verts[1].u = (f32)srcRight; verts[1].v = (f32)srcTop;
    verts[2].x = (f32)(dstX + (srcRight - srcLeft)); verts[2].y = (f32)(dstY + (srcBottom - srcTop)); verts[2].u = (f32)srcRight; verts[2].v = (f32)srcBottom;
    verts[3].x = (f32)dstX; verts[3].y = (f32)(dstY + (srcBottom - srcTop)); verts[3].u = (f32)srcLeft; verts[3].v = (f32)srcBottom;

    gbh_soft_quad(gGbhImageTable[imageIndex].field_C_pTexture, verts, 4);
    return 0;
}

static void CC gbh_stub_void()
{
}

static s32 CC gbh_stub_s32_00()
{
    return 0;
}

static s32 CC vid_stub_s32_0(SVideo*)
{
    return 0;
}

static s32 CC vid_stub_s32_1(SVideo*)
{
    return 1;
}

static s32 CC vid_stub_checks(SVideo*, s32, s32, s32)
{
    return 1;
}

static SVideo* CC vid_stub_self_init(HINSTANCE, u16)
{


    __android_log_print(ANDROID_LOG_INFO, LOGT, "Vid_Init_SYS (real)");
    return &gVidSys_7071D0_storage;
}

static s32 CC vid_real_setdevice(SVideo*, s32)
{
    return 0;
}

static void CC vid_real_closescreen(SVideo*)
{
    std::lock_guard<std::mutex> lock(gVidMutex);
    if (gFramebuffer)
    {
        free(gFramebuffer);
        gFramebuffer = 0;
    }
}

static s32 CC vid_real_setmode(SVideo* pVideo, HWND, s32 modeId)
{
    (void)modeId;
    std::lock_guard<std::mutex> lock(gVidMutex);
    if (!gFramebuffer)
    {
        gFramebuffer = (u16*)calloc(gFBW * gFBH, 2);
        if (!gFramebuffer) return -1;
    }
    gFBStride = gFBW;
    if (pVideo)
    {
        pVideo->field_48_rect_right = (DWORD)gFBW;
        pVideo->field_4C_rect_bottom = (DWORD)gFBH;
        pVideo->field_74 = (DWORD)gFBW;
    }
    __android_log_print(ANDROID_LOG_INFO, LOGT, "Vid_SetMode: %d x %d mode=%d", gFBW, gFBH, modeId);
    return 1;
}

static s32 CC vid_real_enablewrites(SVideo* pVideo)
{
    if (pVideo)
    {
        pVideo->field_50_surface_pixels_ptr = gFramebuffer;
        pVideo->field_54_surface_pixels_pitch = (DWORD)(gFBStride * 2);
    }
    return 0;
}

static s32 CC vid_real_disablewrites(SVideo* pVideo)
{
    if (pVideo)
    {
        pVideo->field_50_surface_pixels_ptr = 0;
    }
    return 0;
}

static inline u16 vid_rgb565(u8 r, u8 g, u8 b)
{
    return (u16)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static s32 CC vid_real_clearscreen(SVideo*, u8 r, u8 g, u8 b, s32 left, s32 top, s32 right, s32 bottom)
{

    if (gGlesCtx)
    {
        gles_set_clear_color(gGlesCtx, r / 255.0f, g / 255.0f, b / 255.0f);
        gles_clear_game_rect(gGlesCtx, left, top, right, bottom);
    }
    std::lock_guard<std::mutex> lock(gVidMutex);
    if (!gFramebuffer) return -1;
    u16 c = vid_rgb565(r, g, b);
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > gFBW) right = gFBW;
    if (bottom > gFBH) bottom = gFBH;
    for (int y = top; y < bottom; y++)
    {
        u16* row = gFramebuffer + y * gFBStride;
        for (int x = left; x < right; x++)
        {
            row[x] = c;
        }
    }
    return 0;
}

static void vid_copy_to_surface(ANativeWindow* win)
{
    ANativeWindow_Buffer buf;
    if (ANativeWindow_lock(win, &buf, 0) != 0) return;

    static bool sLoggedSize = false;
    if (!sLoggedSize)
    {
        sLoggedSize = true;
        __android_log_print(ANDROID_LOG_INFO, LOGT, "surface buffer: %dx%d format=%d",
                            buf.width, buf.height, (int)buf.format);
    }



    const u16* fb = gFramebuffer;
    const int srcW = gFBW, srcH = gFBH;
    int dstW, dstH;
    int scaleFit = (buf.width / srcW < buf.height / srcH) ? (buf.width / srcW) : (buf.height / srcH);
    if (scaleFit >= 3)
    {

        dstW = srcW * scaleFit;
        dstH = srcH * scaleFit;
    }
    else
    {

        dstW = buf.width;
        dstH = (dstW * srcH) / srcW;
        if (dstH > buf.height)
        {
            dstH = buf.height;
            dstW = (dstH * srcW) / srcH;
        }
    }
    int ox = (buf.width - dstW) / 2;
    int oy = (buf.height - dstH) / 2;

    switch (buf.format)
    {
        case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM:
        {
            u16* dst = (u16*)buf.bits;
            for (int y = 0; y < dstH; y++)
            {
                const float sy = ((float)y + 0.5f) * srcH / dstH - 0.5f;
                const int sy0 = (sy >= 0) ? (int)sy : 0;
                const int sy1 = (sy0 + 1 < srcH) ? sy0 + 1 : sy0;
                const float fy = sy - sy0;
                const u16* s0 = fb + (size_t)sy0 * gFBStride;
                const u16* s1 = fb + (size_t)sy1 * gFBStride;
                u16* drow = dst + (size_t)(y + oy) * buf.stride + ox;
                for (int x = 0; x < dstW; x++)
                {
                    const float sx = ((float)x + 0.5f) * srcW / dstW - 0.5f;
                    const int sx0 = (sx >= 0) ? (int)sx : 0;
                    const int sx1 = (sx0 + 1 < srcW) ? sx0 + 1 : sx0;
                    const float fx = sx - sx0;
                    const u16 p00 = s0[sx0], p01 = s0[sx1], p10 = s1[sx0], p11 = s1[sx1];
                    int r = (int)((((p00 >> 11) & 0x1F) * (1 - fx) + ((p01 >> 11) & 0x1F) * fx) * (1 - fy) +
                                  (((p10 >> 11) & 0x1F) * (1 - fx) + ((p11 >> 11) & 0x1F) * fx) * fy);
                    int g = (int)((((p00 >> 5) & 0x3F) * (1 - fx) + ((p01 >> 5) & 0x3F) * fx) * (1 - fy) +
                                  (((p10 >> 5) & 0x3F) * (1 - fx) + ((p11 >> 5) & 0x3F) * fx) * fy);
                    int b = (int)(((p00 & 0x1F) * (1 - fx) + (p01 & 0x1F) * fx) * (1 - fy) +
                                  ((p10 & 0x1F) * (1 - fx) + (p11 & 0x1F) * fx) * fy);
                    if (r > 31) r = 31; if (g > 63) g = 63; if (b > 31) b = 31;
                    drow[x] = (u16)((r << 11) | (g << 5) | b);
                }
            }
            break;
        }
        case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM:
        {
            u32* dst = (u32*)buf.bits;
            for (int y = 0; y < dstH; y++)
            {
                const float sy = ((float)y + 0.5f) * srcH / dstH - 0.5f;
                const int sy0 = (sy >= 0) ? (int)sy : 0;
                const int sy1 = (sy0 + 1 < srcH) ? sy0 + 1 : sy0;
                const float fy = sy - sy0;
                const u16* s0 = fb + (size_t)sy0 * gFBStride;
                const u16* s1 = fb + (size_t)sy1 * gFBStride;
                u32* drow = dst + (size_t)(y + oy) * buf.stride + ox;
                for (int x = 0; x < dstW; x++)
                {
                    const float sx = ((float)x + 0.5f) * srcW / dstW - 0.5f;
                    const int sx0 = (sx >= 0) ? (int)sx : 0;
                    const int sx1 = (sx0 + 1 < srcW) ? sx0 + 1 : sx0;
                    const float fx = sx - sx0;
                    const u16 p00 = s0[sx0], p01 = s0[sx1], p10 = s1[sx0], p11 = s1[sx1];
                    int r = (int)((((p00 >> 11) & 0x1F) * (1 - fx) + ((p01 >> 11) & 0x1F) * fx) * (1 - fy) +
                                  (((p10 >> 11) & 0x1F) * (1 - fx) + ((p11 >> 11) & 0x1F) * fx) * fy);
                    int g = (int)((((p00 >> 5) & 0x3F) * (1 - fx) + ((p01 >> 5) & 0x3F) * fx) * (1 - fy) +
                                  (((p10 >> 5) & 0x3F) * (1 - fx) + ((p11 >> 5) & 0x3F) * fx) * fy);
                    int b = (int)(((p00 & 0x1F) * (1 - fx) + (p01 & 0x1F) * fx) * (1 - fy) +
                                  ((p10 & 0x1F) * (1 - fx) + (p11 & 0x1F) * fx) * fy);
                    if (r > 31) r = 31; if (g > 63) g = 63; if (b > 31) b = 31;
                    drow[x] = 0xFF000000u | ((u32)((r << 3) | (r >> 2)) << 16) |
                             ((u32)((g << 2) | (g >> 4)) << 8) |
                             ((u32)((b << 3) | (b >> 2)));
                }
            }
            break;
        }
        case AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM:
        {
            u8* dst = (u8*)buf.bits;
            for (int y = 0; y < dstH; y++)
            {
                const float sy = ((float)y + 0.5f) * srcH / dstH - 0.5f;
                const int sy0 = (sy >= 0) ? (int)sy : 0;
                const int sy1 = (sy0 + 1 < srcH) ? sy0 + 1 : sy0;
                const float fy = sy - sy0;
                const u16* s0 = fb + (size_t)sy0 * gFBStride;
                const u16* s1 = fb + (size_t)sy1 * gFBStride;
                u8* drow = dst + (size_t)(y + oy) * buf.stride * 3 + ox * 3;
                for (int x = 0; x < dstW; x++)
                {
                    const float sx = ((float)x + 0.5f) * srcW / dstW - 0.5f;
                    const int sx0 = (sx >= 0) ? (int)sx : 0;
                    const int sx1 = (sx0 + 1 < srcW) ? sx0 + 1 : sx0;
                    const float fx = sx - sx0;
                    const u16 p00 = s0[sx0], p01 = s0[sx1], p10 = s1[sx0], p11 = s1[sx1];
                    int r = (int)((((p00 >> 11) & 0x1F) * (1 - fx) + ((p01 >> 11) & 0x1F) * fx) * (1 - fy) +
                                  (((p10 >> 11) & 0x1F) * (1 - fx) + ((p11 >> 11) & 0x1F) * fx) * fy);
                    int g = (int)((((p00 >> 5) & 0x3F) * (1 - fx) + ((p01 >> 5) & 0x3F) * fx) * (1 - fy) +
                                  (((p10 >> 5) & 0x3F) * (1 - fx) + ((p11 >> 5) & 0x3F) * fx) * fy);
                    int b = (int)(((p00 & 0x1F) * (1 - fx) + (p01 & 0x1F) * fx) * (1 - fy) +
                                  ((p10 & 0x1F) * (1 - fx) + (p11 & 0x1F) * fx) * fy);
                    if (r > 31) r = 31; if (g > 63) g = 63; if (b > 31) b = 31;
                    drow[x * 3 + 0] = (u8)((r << 3) | (r >> 2));
                    drow[x * 3 + 1] = (u8)((g << 2) | (g >> 4));
                    drow[x * 3 + 2] = (u8)((b << 3) | (b >> 2));
                }
            }
            break;
        }
    }
    ANativeWindow_unlockAndPost(win);
}

static void CC vid_real_flipbuffers(SVideo*)
{
    static long flipCount = 0;
    flipCount++;
    if (flipCount <= 2 || (flipCount % 120) == 0)
        __android_log_print(ANDROID_LOG_INFO, LOGT, "FSTAT F%d quads=%d tris=%d tridrawn=%d zerouv=%d blanktex=%d staletex=%d clip0=%d shdw=%d",
                            gles_frame_seq(), sFrQuads, sFrTris, sFrTriDrawn, sFrZeroUv, sFrBlankTex, sFrStaleTex, sFrClip0, sFrShadows);
    if ((flipCount % 120) == 0)
        __android_log_print(ANDROID_LOG_INFO, LOGT, "Vid_FlipBuffers (real) frames=%ld surface=%s", flipCount,
                            gSurface ? "yes" : "no");

    if (gles_ensure_context())
    {
        gles_end_frame(gGlesCtx);
        return;
    }

    std::lock_guard<std::mutex> lock(gVidMutex);
    if (!gFramebuffer || !gSurface) return;
    vid_copy_to_surface(gSurface);
}

static void CC vid_real_grabsurface(SVideo*)
{
}

static void CC vid_real_releasesurface(SVideo*)
{
}

static void CC vid_real_shutdown(SVideo*)
{
    std::lock_guard<std::mutex> lock(gVidMutex);
    if (gFramebuffer)
    {
        free(gFramebuffer);
        gFramebuffer = 0;
    }
}

static s32 CC vid_stub_clearscreen(SVideo*, u8, u8, u8, s32, s32, s32, s32)
{
    return 0;
}

T_Vid_Init_SYS pVid_Init_SYS = vid_stub_self_init;
T_Vid_CheckMode pVid_CheckMode = vid_stub_checks;
T_Vid_FindMode pVid_FindMode = 0;
T_Vid_FindFirstMode pVid_FindFirstMode = 0;
T_Vid_FindNextMode pVid_FindNextMode = 0;
T_Vid_FindDevice pVid_FindDevice = 0;
T_Vid_SetDevice pVid_SetDevice = (T_Vid_SetDevice)vid_real_setdevice;
T_Vid_CloseScreen pVid_CloseScreen = vid_real_closescreen;
T_Vid_SetMode pVid_SetMode = vid_real_setmode;
T_Vid_FlipBuffers pVid_FlipBuffers = vid_real_flipbuffers;
T_Vid_ReleaseSurface pVid_ReleaseSurface = vid_real_releasesurface;
T_Vid_GrabSurface pVid_GrabSurface = vid_real_grabsurface;
T_Vid_ShutDown_SYS pVid_ShutDown_SYS = vid_real_shutdown;
T_Vid_EnableWrites pVid_EnableWrites = vid_real_enablewrites;
T_Vid_DisableWrites pVid_DisableWrites = vid_real_disablewrites;
T_Vid_GetSurface pVid_GetSurface = (T_Vid_GetSurface)vid_stub_s32_0;
T_Vid_FreeSurface pVid_FreeSurface = (T_Vid_FreeSurface)vid_stub_s32_0;
T_Vid_ClearScreen pVid_ClearScreen = vid_real_clearscreen;
T_Vid_WindowProc pVid_WindowProc = (T_Vid_WindowProc)vid_stub_s32_0;
T_Vid_InitDLL pVid_InitDLL = (T_Vid_InitDLL)vid_stub_s32_1;
T_Vid_SetGamma pVid_SetGamma = (T_Vid_SetGamma)vid_stub_s32_1;
T_Vid_GetVersion pVid_GetVersion = 0;

T_gbh_InitDLL pgbh_InitDLL = gbh_real_initdll;
T_gbh_Init pgbh_Init = gbh_real_init;
T_gbh_CloseDLL pgbh_CloseDLL = gbh_real_closedll;
T_gbh_SetWindow pgbh_SetWindow = gbh_real_setwindow;
T_gbh_GetGlobals pgbh_GetGlobals = gbh_real_getglobals;
T_gbh_DrawTile pgbh_DrawTile = (T_gbh_DrawTile)gbh_real_drawtile;
T_gbh_DrawTilePart pgbh_DrawTilePart = (T_gbh_DrawTilePart)gbh_real_drawtilepart;
T_gbh_DrawQuad pgbh_DrawQuad = gbh_real_drawquad;
T_gbh_DrawQuadClipped pgbh_DrawQuadClipped = gbh_real_drawquadclipped;
T_gbh_DrawFlatRect pgbh_DrawFlatRect = gbh_real_drawflatrect;
T_gbh_DrawTriangle pgbh_DrawTriangle = gbh_real_drawtriangle;
T_gbh_PrintBitmap pgbh_PrintBitmap = gbh_real_printbitmap;
T_gbh_SetColourDepth pgbh_SetColourDepth = gbh_real_setcolourdepth;
T_gbh_Plot pgbh_Plot = gbh_real_plot;
T_gbh_ConvertColour pgbh_ConvertColour = gbh_real_convertcolour;
T_gbh_RegisterTexture pgbh_RegisterTexture = gbh_real_registertexture;
T_gbh_SetCamera pgbh_SetCamera = gbh_real_setcamera;
T_gbh_ResetLights pgbh_ResetLights = gbh_real_resetlights;
T_gbh_AddLight pgbh_AddLight = gbh_real_addlight;
T_gbh_SetAmbient pgbh_SetAmbient = gbh_real_setambient;
T_gbh_GetUsedCache pgbh_GetUsedCache = gbh_real_getusedcache;
T_gbh_FreeTexture pgbh_FreeTexture = gbh_real_freetexture;
T_gbh_RegisterPalette pgbh_RegisterPalette = gbh_real_registerpalette;
T_gbh_FreePalette pgbh_FreePalette = gbh_real_freepalette;
T_gbh_AssignPalette pgbh_AssignPalette = gbh_real_assignpalette;
T_gbh_LockTexture pgbh_LockTexture = gbh_real_locktexture;
T_gbh_UnlockTexture pgbh_UnlockTexture = gbh_real_unlocktexture;
T_gbh_BeginScene pgbh_BeginScene = gbh_real_beginscene;
T_gbh_EndScene pgbh_EndScene = gbh_real_endscene;
T_gbh_BeginLevel pgbh_BeginLevel = gbh_real_beginlevel;
T_gbh_EndLevel pgbh_EndLevel = gbh_real_endlevel;
T_MakeScreenTable pMakeScreenTable = (T_MakeScreenTable)gbh_real_makescreentable;
T_ConvertColourBank pConvertColourBank = (T_ConvertColourBank)gbh_stub_void;
T_SetShadeTableA pSetShadeTableA = (T_SetShadeTableA)gbh_stub_void;
T_gbh_InitImageTable pgbh_InitImageTable = gbh_real_initimagetable;
T_gbh_FreeImageTable pgbh_FreeImageTable = gbh_real_freeimagetable;
T_gbh_LoadImage pgbh_LoadImage = gbh_real_loadimage;
T_gbh_BlitImage pgbh_BlitImage = gbh_real_blitimage;
T_gbh_BlitBuffer pgbh_BlitBuffer = gbh_real_blitbuffer;

void __stdcall ErrorMsgBox_5E4EC0(LPCSTR lpText)
{
    __android_log_print(ANDROID_LOG_ERROR, LOGT, "ErrorMsgBox: %s", lpText ? lpText : "(null)");
}

void __stdcall sub_4DA740()
{
}

void sub_4DA830()
{
}

void sub_5D8E00()
{
}

void __stdcall GBH_Graphis_DMA_Video_Free_5D9830()
{
}

void Start_GTA2Manager_5E4DE0()
{
    __android_log_print(ANDROID_LOG_INFO, LOGT, "Start_GTA2Manager (stub)");
}

void GetGTA2Version_5E5D60(int* pVerMinor, int* pVerMajor)
{
    if (pVerMinor) *pVerMinor = 2;
    if (pVerMajor) *pVerMajor = 0;
}

BYTE bReplayMode_6F5B71 = 0;
u32 gMatchStartTime_6F5A28 = 0;
u8 byte_6F58D8 = 0;
u8 byte_6F5760 = 0;
u8 byte_6F5880 = 0;
u8 max_frame_rate_626A08 = 1;
u8 min_frame_rate_706C50 = 0;

void __stdcall sub_5D96C0()
{
}

void __stdcall j_gbh_init_5D7CA0()
{
    __android_log_print(ANDROID_LOG_INFO, LOGT, "j_gbh_init_5D7CA0 (stub)");
}

void __stdcall UpdateWinXY_5D8E70()
{
}

void __stdcall sub_5D9690()
{
}

void __stdcall do_network_and_local_inputs_4DAF30()
{

    const u32 inputs = gBurgerKing_67F8B0.get_input_bits_4CEAC0();
    if (gGame_0x40_67E008 && gGame_0x40_67E008->field_38_orf1)
    {
        gGame_0x40_67E008->field_38_orf1->SetInputs_565740(inputs);
    }
}

char ExecuteGame_4DA780()
{
    char v0 = gGame_0x40_67E008->ExecuteGame_4B9640();
    if (gsharp_bose_0x54_7055D4)
    {
        gsharp_bose_0x54_7055D4->field_18.sub_5BEBF0();
    }
    return v0;
}

void __stdcall Draw_4DA7B0()
{
    gGame_0x40_67E008->Draw_4B92D0();
    pVid_FlipBuffers(gVidSys_7071D0);
    if (bSkip_lid_67D546 || bSkip_top_67D574 || bSkip_bottom_67D4E7 || bSkip_left_67D6BF || bSkip_right_67D4E4)
    {
        pVid_ClearScreen(gVidSys_7071D0, 0, 0, 0, 0, 0,
                         gVidSys_7071D0->field_48_rect_right, gVidSys_7071D0->field_4C_rect_bottom);
    }
}

void __stdcall CleanUpInputAndOthers_4DA700()
{
    gBurgerKing_67F8B0.Shutdown_4CEA00();
    if (gGame_0x40_67E008)
    {
        delete gGame_0x40_67E008;
        gGame_0x40_67E008 = 0;
    }
    gRoot_sound_66B038.Set3DSound_40F160(0);
}

void __stdcall InitializeGame_4DA4D0()
{
    __android_log_print(ANDROID_LOG_INFO, LOGT, "InitializeGame_4DA4D0: input_init...");
    if (bReplayMode_6F5B71)
    {
        gBurgerKing_67F8B0.input_init_replay_4CE740(gHInstance_708220);
        bReplayMode_6F5B71 = 0;
    }
    else
    {
        gBurgerKing_67F8B0.input_init_live_4CE880(gHInstance_708220);
    }

    gRoot_sound_66B038.Set3DSound_40F160(gRegistry_6FF968.Get_Sound_Settting_586A70("do_3d_sound"));
    gRegistry_6FF968.Clear_Or_Delete_Sound_Setting_586BF0("do_3d_sound", gRoot_sound_66B038.Get3DSound_40F180());

    gGame_0x40_67E008 = new Game_0x40(1, 0);
    __android_log_print(ANDROID_LOG_INFO, LOGT, "InitializeGame_4DA4D0: Game_0x40 creado=%p", (void*)gGame_0x40_67E008);

    Init_Phi_54_array();
    Init_Phi_6C_array();
    Init_trigonometry_tables();
    __android_log_print(ANDROID_LOG_INFO, LOGT, "InitializeGame_4DA4D0: Phi_54/Phi_6C arrays loaded");

    gGame_0x40_67E008->LoadGameFiles_4B8C40();
    __android_log_print(ANDROID_LOG_INFO, LOGT, "InitializeGame_4DA4D0: LoadGameFiles done");

    __android_log_print(ANDROID_LOG_INFO, LOGT, "InitializeGame_4DA4D0: BootGame START");
    gGame_0x40_67E008->BootGame_4B8EB0();
    __android_log_print(ANDROID_LOG_INFO, LOGT, "InitializeGame_4DA4D0: BootGame END");

    gMatchStartTime_6F5A28 = timeGetTime();
    byte_6F58D8 = 0;
    byte_6F5880 = 0;
    byte_6F5760 = 1;
}

u8 sub_4DA850()
{
    u8 a2 = 0;
    u8 bContinue = 0;
    u8 unk_0xc = 0;
    DWORD Time = timeGetTime();

    if (!max_frame_rate_626A08 && !byte_6F58D8 || Time >= gMatchStartTime_6F5A28)
    {
        unk_0xc = 1;
    }
    if (byte_6F58D8 >= 8u || !min_frame_rate_706C50 && byte_6F58D8)
    {
        unk_0xc = 0;
    }
    if (byte_6F5880 || bSkip_draw_67D4EA)
    {
        unk_0xc = 1;
    }

    if (unk_0xc)
    {
        if (!min_frame_rate_706C50)
        {
            gMatchStartTime_6F5A28 = Time;
        }
        if (byte_6F5760)
        {
            do_network_and_local_inputs_4DAF30();
            byte_6F5760 = 0;
        }
        bContinue = ExecuteGame_4DA780();
        if (g_stop.load()) bContinue = 0;
        {
            static int dbgN = 0;
            if (((++dbgN % 6000) == 0) && gGame_0x40_67E008)
            {
                Player* dbgP = gGame_0x40_67E008->get_player_4219E0(0);
                Ped* dbgPed = dbgP ? dbgP->field_2C4_player_ped : 0;
                Char_B4* dbgB4 = dbgPed ? dbgPed->field_168_game_object : 0;
                Sprite* dbgSpr = dbgB4 ? dbgB4->field_80_sprite_ptr : 0;
                if (dbgB4 && dbgSpr)
                {
                    Fix16 r1, r2;
                    Ang16::PolarToCartesian_41FC20(dbgB4->field_40_rotation, dword_6FDAC8, r1, r2);
                    r1 += gCharB4_Saved_Xpos_6FD7F8 + dbgB4->field_4C;
                    r2 += gCharB4_Saved_Ypos_6FD800 + dbgB4->field_50;
                    int v90 = dbgB4->CanStepDiagonal_54EF60(r1.ToInt(), r2.ToInt());
                    if (v90 < 0)
                    {
                        __android_log_print(ANDROID_LOG_ERROR, LOGT,
                            "ped dbg dt=%d vel=%d xr=%d yr=%d rot=%d st1=%d st2=%d chst=%d f18=%d f24=%d coll=%d 5532=%d sX=%d sY=%d b51=%d b52=%d dtd=%d v90=%d r1t=%d r2t=%d",
                            dword_703A38.mValue,
                            dbgB4->field_38_velocity.mValue,
                            dbgSpr->field_14_xy.x.mValue,
                            dbgSpr->field_14_xy.y.mValue,
                            dbgB4->field_40_rotation.rValue,
                            dbgB4->field_8_ped_state_1,
                            dbgB4->field_C_ped_state_2,
                            dbgB4->field_10_char_state,
                            dbgB4->field_18,
                            dbgB4->field_24,
                            dbgB4->field_69_is_colliding_with_sprite,
                            dbgB4->sub_5532C0(),
                            gCharB4_Saved_Xpos_6FD7F8.mValue,
                            gCharB4_Saved_Ypos_6FD800.mValue,
                            byte_6FDB51,
                            byte_6FDB52,
                            dword_6FD80C.mValue,
                            v90,
                            r1.ToInt(),
                            r2.ToInt());
                    }
                }
            }
        }
        s32 v2 = gGame_0x40_67E008->sub_4B8BB0();
        byte_6F5880 = 0;
        gMatchStartTime_6F5A28 += v2;
        ++byte_6F58D8;
        byte_6F5760 = 1;
    }
    else if (byte_6F58D8)
    {
        Draw_4DA7B0();
        byte_6F58D8 = 0;
        a2 = 1;
    }
    else if (byte_6F5760)
    {
        do_network_and_local_inputs_4DAF30();
        byte_6F5760 = 0;
    }

    if (gsharp_bose_0x54_7055D4)
    {
        gsharp_bose_0x54_7055D4->sub_5BECF0(a2, unk_0xc);
    }
    return bContinue;
}

void __stdcall Init_keybrd_jolly_and_sound_4DA440();

struct G2MvHeader
{
    char magic[4];
    u8 ver;
    u8 pixfmt;
    u16 pad;
    u32 w, h, frames, fps, dataSize;
    u8 pad2[36];
};

static void G2_PlayIntroMovie(const char* path)
{
    {
        std::lock_guard<std::mutex> lock(gVidMutex);
        if (!gSurface)
        {
            __android_log_print(ANDROID_LOG_INFO, LOGT, "intro: sin superficie aun, se omite");
            return;
        }
    }

    FILE* f = fopen(path, "rb");
    if (!f)
    {
        __android_log_print(ANDROID_LOG_INFO, LOGT, "intro: no se abre %s", path);
        return;
    }

    G2MvHeader h;
    if (fread(&h, 1, sizeof(h), f) != sizeof(h) || memcmp(h.magic, "G2MV", 4) != 0 || h.pixfmt != 1 || h.ver != 2)
    {
        fclose(f);
        __android_log_print(ANDROID_LOG_INFO, LOGT, "intro: header invalido");
        return;
    }
    u32 w = h.w, hh = h.h;
    if (w == 0 || hh == 0 || w > 4096 || hh > 4096 || h.frames == 0 || h.frames > 100000)
    {
        fclose(f);
        __android_log_print(ANDROID_LOG_INFO, LOGT, "intro: dimensiones malas %ux%u fr=%u", w, hh, h.frames);
        return;
    }
    u32 delayMs = 0;
    memcpy(&delayMs, &h.pad2[0], 4);
    if (delayMs == 0 || delayMs > 1000) delayMs = 66;

    const u32 nbytes = w * hh * 2;
    u8* prev = (u8*)malloc(nbytes);
    u8* cur  = (u8*)malloc(nbytes);
    u8* delta = (u8*)malloc(nbytes);
    const size_t ccap = (size_t)nbytes + nbytes / 16 + 4096;
    u8* comp = (u8*)malloc(ccap);
    if (!prev || !cur || !delta || !comp)
    {
        free(prev); free(cur); free(delta); free(comp);
        fclose(f);
        return;
    }
    memset(prev, 0, nbytes);

    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    if (inflateInit2(&zs, -MAX_WBITS) != Z_OK)
    {
        free(prev); free(cur); free(delta); free(comp);
        fclose(f);
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, LOGT, "intro: %ux%u frames=%u delay=%ums ver=%u", w, hh, h.frames, delayMs, h.ver);

    struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (u32 fi = 0; fi < h.frames; fi++)
    {
        u32 csize = 0;
        if (fread(&csize, 1, 4, f) != 4) { __android_log_print(ANDROID_LOG_WARN, LOGT, "intro: fread csize fallo fi=%u", fi); break; }
        if (fi < 3) __android_log_print(ANDROID_LOG_INFO, LOGT, "intro: fi=%u csize=%u", fi, csize);
        if (csize == 0)
        {
            memcpy(cur, prev, nbytes);
        }
        else
        {
            if (csize > ccap) { __android_log_print(ANDROID_LOG_WARN, LOGT, "intro: csize=%u>ccap fi=%u", csize, fi); break; }
            if (fread(comp, 1, csize, f) != csize) { __android_log_print(ANDROID_LOG_WARN, LOGT, "intro: fread comp corto fi=%u csize=%u", fi, csize); break; }
            zs.next_in = comp;
            zs.avail_in = csize;
            zs.next_out = delta;
            zs.avail_out = nbytes;
            if (inflate(&zs, Z_NO_FLUSH) != Z_OK || zs.total_out != nbytes)
            {
                __android_log_print(ANDROID_LOG_WARN, LOGT, "intro: inflate fallo en frame %u", fi);
                break;
            }
            inflateReset(&zs);
            for (u32 i = 0; i < nbytes; i++) cur[i] = (u8)(prev[i] + delta[i]);
        }

        {
            std::lock_guard<std::mutex> lock(gVidMutex);
            if (!gSurface || !gFramebuffer) { __android_log_print(ANDROID_LOG_WARN, LOGT, "intro: sin surface/fb fi=%u", fi); break; }
            if (w == (u32)gFBW && hh == (u32)gFBH)
            {
                memcpy(gFramebuffer, cur, nbytes);
            }
            else
            {
                for (u32 y = 0; y < hh && y < (u32)gFBH; y++)
                {
                    u32 sy = (y * gFBH) / hh;
                    const u16* srow = (const u16*)cur + sy * w;
                    u16* drow = gFramebuffer + y * gFBStride;
                    for (u32 x = 0; x < w && x < (u32)gFBW; x++)
                    {
                        u32 sx = (x * gFBW) / w;
                        drow[x] = srow[sx];
                    }
                }
            }
            vid_copy_to_surface(gSurface);
        }

        {
            DIDEVICEOBJECTDATA ev[16];
            int n = G2_InputGetEvents(ev, 16, 0);
            int skip = 0;
            for (int i = 0; i < n; i++)
            {
                if (ev[i].dwData & 0x80) { skip = 1; break; }
            }
            if (skip)
            {
                __android_log_print(ANDROID_LOG_INFO, LOGT, "intro: skip por tecla en frame %u", fi);
                break;
            }
        }

        struct timespec req = {0, (long)(delayMs * 1000000)};
        struct timespec rem;
        while (nanosleep(&req, &rem) != 0 && (rem.tv_sec > 0 || rem.tv_nsec > 0)) req = rem;

        u8* t = prev; prev = cur; cur = t;

        if (fi % 400 == 0 || fi + 1 == h.frames)
        {
            struct timespec tn;
            clock_gettime(CLOCK_MONOTONIC, &tn);
            long elapsed = (long)((tn.tv_sec - t0.tv_sec) * 1000 + (tn.tv_nsec - t0.tv_nsec) / 1000000);
            __android_log_print(ANDROID_LOG_INFO, LOGT, "intro: frame %u llegado (t=%ldms)", fi + 1, elapsed);
        }
    }

    inflateEnd(&zs);
    free(prev);
    free(cur);
    free(delta);
    free(comp);
    fclose(f);
    {
        struct timespec tn;
        clock_gettime(CLOCK_MONOTONIC, &tn);
        long elapsed = (long)((tn.tv_sec - t0.tv_sec) * 1000 + (tn.tv_nsec - t0.tv_nsec) / 1000000);
        __android_log_print(ANDROID_LOG_INFO, LOGT, "intro: fin (t=%ldms)", elapsed);
    }
}

int __stdcall WinMain_5E53F0(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nShowCmd;

    gHInstance_708220 = hInstance;

    if (CoInitialize(0) < 0)
    {
        return 0;
    }

    GetGTA2Version_5E5D60(&gGTA2VersionMajor_708280, &gGTA2VersionMajor_708284);


    G2Platform_SetDebugString("mapname", "bil.gmp");
    G2Platform_SetDebugString("stylename", "bil.sty");
    G2Platform_SetDebugString("scriptname", "bil.scr");
    G2Platform_SetDebugDword("skip_frontend", 0);
    G2Platform_SetDebugDword("skip_audio", 0);
    __android_log_print(ANDROID_LOG_INFO, LOGT, "frontend habilitado (skip_frontend=0)");

    gDebug_67D52C.Init_4ABBD0();
    bSkip_audio_67D6BE = false;
    __android_log_print(ANDROID_LOG_INFO, LOGT, "skip_audio forzado a 0");
    gLucid_hamilton_67E8E0.sub_4C53D0();
    bDoFrontEnd_626B68 = bSkip_frontend_67D53B == 0;
    __android_log_print(ANDROID_LOG_INFO, LOGT, "skip_frontend leido=%d  bDoFrontEnd=%d",
                        (int)bSkip_frontend_67D53B, (int)bDoFrontEnd_626B68);
    bDoFrontEnd_626B68 = true;

    getcwd(gWorkingDir_707F64, 256);


    Input::DirectInputCreate_4986D0(gHInstance_708220);



    extern int dword_61A9E4[12];

    static const int s_defControls[12] = {
        DIK_UP, DIK_DOWN, DIK_LEFT, DIK_RIGHT, DIK_LCONTROL, DIK_RETURN,
        DIK_SPACE, DIK_Y, DIK_X, DIK_TAB, DIK_LMENU, DIK_RSHIFT};
    memcpy(dword_61A9E4, s_defControls, sizeof(s_defControls));
    G2Platform_DeleteRegistrySubtree("SOFTWARE/DMA Design Ltd/GTA2/Control");
    gBurgerKing_1_67B990->input_devices_init_498C40(gHInstance_708220);
    __android_log_print(ANDROID_LOG_INFO, LOGT, "input_devices_init OK (kb fake=%d)", (int)(gKeyboardDevice_67B5C0 != 0));


    gVidSys_7071D0 = pVid_Init_SYS(gHInstance_708220, 0);
    if (gVidSys_7071D0)
    {
        pVid_SetDevice(gVidSys_7071D0, 0);
        pVid_SetMode(gVidSys_7071D0, 0, 0);
        pVid_ClearScreen(gVidSys_7071D0, 0, 0, 0, 0, 0,
                         gVidSys_7071D0->field_48_rect_right, gVidSys_7071D0->field_4C_rect_bottom);
        pVid_FlipBuffers(gVidSys_7071D0);
    }

    j_gbh_init_5D7CA0();
    Init_keybrd_jolly_and_sound_4DA440();





    if (gRegistry_6FF968.Get_Screen_Setting_5870D0("do_play_movie", 0))
    {
        G2_PlayIntroMovie("data/Movie/intro.mv");
    }

    s32 state = 0;

    u8 bQuit = 0;

    while (1)
    {
        bQuit = 0;

        if (bDoFrontEnd_626B68)
        {
            Frontend::create_4ACFA0();
            gFrontend_67DC84->sub_4B3170(state);
        }
        else
        {


            gBurgerKing_67F8B0.field_75344_bInputEnabled = 1;
            InitializeGame_4DA4D0();
        }

        UpdateWinXY_5D8E70();
        sub_5D9690();

        while (1)
        {
            if (!bQuit)
            {
                if (bDoFrontEnd_626B68)
                {
                    s32 t = gFrontend_67DC84->sub_4AEDB0();

                    if (t == Quit_1)
                    {
                        bQuit = Quit_1;
                        Frontend::destroy_4AD070();
                    }
                    else if (t == Start_Game_3)
                    {
                        Frontend::destroy_4AD070();
                        bDoFrontEnd_626B68 = false;
                        break;
                    }
                    else if (t == Start_Replay_4)
                    {
                        Frontend::destroy_4AD070();
                        bDoFrontEnd_626B68 = false;
                        bReplayMode_6F5B71 = true;
                        break;
                    }
                    else
                    {

                    }
                }
                else
                {
                    bQuit = sub_4DA850();
                }
            }


            struct timespec ts = {0, 1000000};
            nanosleep(&ts, 0);

            if (!bDoFrontEnd_626B68 && bQuit)
            {
                break;
            }
        }

        if (bQuit)
        {
            if (bSkip_frontend_67D53B)
            {
                break;
            }




            bool bCloseGame = false;
            if (gGame_0x40_67E008)
            {
                switch (gGame_0x40_67E008->field_2C_game_exit_type)
                {
                    case GameExitType::CloseGameImmediately_1:
                        bCloseGame = true;
                        break;

                    case GameExitType::PlayerQuit_2:
                        gLucid_hamilton_67E8E0.sub_4C5A10(gGame_0x40_67E008->field_38_orf1);
                        gJolly_poitras_0x2BC0_6FEAC0->sub_56BB10(gGame_0x40_67E008->field_38_orf1);
                        gJolly_poitras_0x2BC0_6FEAC0->sub_56C010();
                        state = gLucid_hamilton_67E8E0.sub_4C59A0() != 0 ? 6 : 11;
                        break;

                    case GameExitType::GameOverRIP_3:
                        gLucid_hamilton_67E8E0.sub_4C5A10(gGame_0x40_67E008->field_38_orf1);
                        gJolly_poitras_0x2BC0_6FEAC0->sub_56BB10(gGame_0x40_67E008->field_38_orf1);
                        gJolly_poitras_0x2BC0_6FEAC0->sub_56C010();
                        state = gLucid_hamilton_67E8E0.sub_4C59A0() != 0 ? 6 : 2;
                        break;

                    case GameExitType::AreaCompleted_4:
                        gLucid_hamilton_67E8E0.sub_4C5A10(gGame_0x40_67E008->field_38_orf1);
                        gJolly_poitras_0x2BC0_6FEAC0->sub_56BB10(gGame_0x40_67E008->field_38_orf1);
                        gJolly_poitras_0x2BC0_6FEAC0->sub_56C010();
                        state = gLucid_hamilton_67E8E0.sub_4C59A0() != 0 ? 6 : 3;
                        break;

                    case GameExitType::MultiplayerExit_5:
                        state = 7;
                        break;

                    default:
                        break;
                }
            }

            CleanUpInputAndOthers_4DA700();
            bDoFrontEnd_626B68 = 1;
            bQuit = 0;
            if (!bCloseGame)
            {
                continue;
            }
            break;
        }
    }

    CoUninitialize();
    return 0;
}

void __stdcall Init_keybrd_jolly_and_sound_4DA440()
{
    keybrd_0x204::create_4D5F50();
    jolly_poitras_0x2BC0::create_56C2C0();

    if (!bSkip_audio_67D6BE)
    {
        gRoot_sound_66B038.sub_40EF80();
        gRoot_sound_66B038.SetCDVol_40F0F0(gRegistry_6FF968.Set_Sound_Setting_586AE0("CDVol", 127));
        gRoot_sound_66B038.SetSfxVol_40F0B0(gRegistry_6FF968.Set_Sound_Setting_586AE0("SFXVol", 127));
        gRoot_sound_66B038.Set3DSound_40F160(gRegistry_6FF968.Get_Sound_Settting_586A70("do_3d_sound"));
        gRegistry_6FF968.Clear_Or_Delete_Sound_Setting_586BF0("do_3d_sound", gRoot_sound_66B038.Get3DSound_40F180());
    }
}