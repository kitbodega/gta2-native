
#include "windows.h"
#include "io.h"
#include <android/log.h>
#ifndef ANDROID_LOG_DEBUG
#define ANDROID_LOG_DEBUG 3
#endif
#include <types.hpp>
#include <Dinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <errno.h>

static unsigned long g_lastError = 0;
static char g_storageDir[512] = "./gta2_data";
static char g_gameDataDir[1024] = "";

void G2Platform_SetStorageDir(const char* dir) { snprintf(g_storageDir, sizeof(g_storageDir), "%s", dir); }
const char* G2Platform_GetStorageDir() { return g_storageDir; }
void G2Platform_SetGameDataDir(const char* dir) { snprintf(g_gameDataDir, sizeof(g_gameDataDir), "%s", dir); }
const char* G2Platform_GetGameDataDir() { return g_gameDataDir; }

#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <android/log.h>

extern "C" FILE* fopen(const char* name, const char* mode);

static FILE* G2_RealFopen(const char* name, const char* mode)
{
    static FILE* (*s_realFopen)(const char*, const char*) = 0;
    if (!s_realFopen)
    {
        s_realFopen = (FILE* (*)(const char*, const char*))dlsym(RTLD_NEXT, "fopen");
        if (!s_realFopen)
        {
            s_realFopen = (FILE* (*)(const char*, const char*))dlsym(RTLD_DEFAULT, "fopen");
        }
    }
    return s_realFopen ? s_realFopen(name, mode) : 0;
}

static void G2_FixSlashes(char* out, size_t cap, const char* name)
{
    size_t i = 0;
    for (; i < cap - 1 && name[i]; i++)
    {
        out[i] = (name[i] == '\\') ? '/' : name[i];
    }
    out[i] = 0;
}

static int G2_KeyEqualCI(const char* a, const char* b)
{
    while (*a && *b)
    {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return 0;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static int G2_ResolvePathCI(const char* path, char* out, size_t cap)
{


    char dir[1024] = "";
    const char* p = path;
    size_t o = 0;
    int isAbs = (path[0] == '/');
    while (*p)
    {
        while (*p == '/')
        {
            p++;
        }
        if (!*p)
        {
            break;
        }
        const char* comp = p;
        while (*p && *p != '/')
        {
            p++;
        }
        char compBuf[801];
        size_t clen = (size_t)(p - comp);
        if (clen > 800)
        {
            clen = 800;
        }
        memcpy(compBuf, comp, clen);
        compBuf[clen] = 0;

        if (*dir || (isAbs && o == 0))
        {
            if (o + 1 >= cap)
            {
                break;
            }
            out[o++] = '/';
        }

        char probe[1100];
        if (*dir)
        {
            snprintf(probe, sizeof(probe), "%s/%s", dir, compBuf);
        }
        else if (isAbs)
        {
            snprintf(probe, sizeof(probe), "/%s", compBuf);
        }
        else
        {
            snprintf(probe, sizeof(probe), "%s", compBuf);
        }

        int exact = 0;
        {
            struct stat st;
            if (stat(probe, &st) == 0)
            {
                exact = 1;
            }
        }

        const char* chosen = compBuf;
        if (!exact)
        {
            DIR* d = opendir(*dir ? dir : (isAbs ? "/" : "."));
            if (d)
            {
                struct dirent* e;
                while ((e = readdir(d)))
                {
                    if (G2_KeyEqualCI(e->d_name, compBuf))
                    {
                        chosen = e->d_name;
                        break;
                    }
                }
                closedir(d);
            }
        }

        size_t elen = strlen(chosen);
        if (elen > 800)
        {
            elen = 800;
        }
        if (o + elen >= cap)
        {
            elen = cap - 1 - o;
        }
        memcpy(out + o, chosen, elen);
        o += elen;

        char next[1024];
        if (*dir)
        {
            snprintf(next, sizeof(next), "%s/%s", dir, chosen);
        }
        else if (isAbs)
        {
            snprintf(next, sizeof(next), "/%s", chosen);
        }
        else
        {
            snprintf(next, sizeof(next), "%s", chosen);
        }
        snprintf(dir, sizeof(dir), "%s", next);
    }
    out[o] = 0;
    return o > 0;
}

extern "C" FILE* fopen(const char* name, const char* mode)
{
    char fixed[1024];
    G2_FixSlashes(fixed, sizeof(fixed), name);

    if (fixed[1] == ':' && G2_KeyEqualCI(fixed + 2, "GTAUDIO"))
    {
        const char* rest = fixed + 3;
        while (*rest == '/') rest++;
        const char* dataDir = G2Platform_GetGameDataDir();
        char mapped[1024];
        if (dataDir && dataDir[0])
        {
            snprintf(mapped, sizeof(mapped), "%s/data/GTAudio/%s", dataDir, rest);
        }
        else
        {
            snprintf(mapped, sizeof(mapped), "data/GTAudio/%s", rest);
        }
        snprintf(fixed, sizeof(fixed), "%s", mapped);
        __android_log_print(ANDROID_LOG_INFO, "GTA2-port", "fopen drive-remap -> %s", fixed);
    }
    FILE* f = G2_RealFopen(fixed, mode);
    if (f)
    {
        long sz = 0;
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        __android_log_print(ANDROID_LOG_INFO, "GTA2-port", "fopen OK [%s] size=%ld mode=%s", fixed, sz, mode);
        return f;
    }

    const char* dataDir = G2Platform_GetGameDataDir();
    if (dataDir && dataDir[0] && fixed[0] != '/')
    {
        char alt[1024];
        snprintf(alt, sizeof(alt), "%s/%s", dataDir, fixed);
        f = G2_RealFopen(alt, mode);
        if (f)
        {
            return f;
        }
    }

    {
        char resolved[1200];
        memset(resolved, 0, sizeof(resolved));
        if (G2_ResolvePathCI(fixed, resolved, sizeof(resolved)) && resolved[0])
        {
            f = G2_RealFopen(resolved, mode);
            if (f)
            {
                return f;
            }
            if (dataDir && dataDir[0] && resolved[0] != '/')
            {
                char altResolved[1200];
                snprintf(altResolved, sizeof(altResolved), "%s/%s", dataDir, resolved);
                f = G2_RealFopen(altResolved, mode);
                if (f)
                {
                    return f;
                }
            }
        }
    }
    return 0;
}

unsigned long Win32_GetLastError(void) { return g_lastError; }
void Win32_SetLastError(unsigned long e) { g_lastError = e; }
void OutputDebugStringA(const char* s) { if (s) fprintf(stderr, "[dbg] %s\n", s); }

unsigned long timeGetTime_impl(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}
unsigned long GetCurrentThreadId(void) { return (unsigned long)(uintptr_t)pthread_self(); }
void* GetCurrentProcess(void) { return (void*)(uintptr_t)1; }

void Windows_Sleep(unsigned long ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, 0);
}

BOOL GetVersionExA(void* lpOSVer)
{
    OSVERSIONINFOA* o = (OSVERSIONINFOA*)lpOSVer;
    o->dwMajorVersion = 6; o->dwMinorVersion = 0; o->dwBuildNumber = 9200; o->dwPlatformId = VER_PLATFORM_WIN32_NT;
    return TRUE;
}
BOOL GetVersionExW(void* lpOSVer)
{
    OSVERSIONINFOW* o = (OSVERSIONINFOW*)lpOSVer;
    o->dwMajorVersion = 6; o->dwMinorVersion = 0; o->dwBuildNumber = 9200; o->dwPlatformId = VER_PLATFORM_WIN32_NT;
    return TRUE;
}
int GetSystemMetrics(int nIndex) { (void)nIndex; return 800; }

static pthread_mutex_t g_mutexCompat = PTHREAD_MUTEX_INITIALIZER;
void* CreateMutexA(void* lpAttr, BOOL bInitialOwner, const char* lpName)
{ (void)lpAttr; (void)bInitialOwner; (void)lpName; return (void*)&g_mutexCompat; }
void* OpenMutexA(unsigned long dwDesiredAccess, BOOL bInheritHandle, const char* lpName)
{ (void)dwDesiredAccess; (void)bInheritHandle; (void)lpName; return (void*)&g_mutexCompat; }
BOOL ReleaseMutex(void* hMutex) { (void)hMutex; return TRUE; }
BOOL CloseHandle(void* hObject) { (void)hObject; return TRUE; }

void* GetModuleHandleA(const char* lpModuleName) { (void)lpModuleName; return (void*)(uintptr_t)0x400000; }
void* LoadLibraryA(const char* lpLibFileName) { (void)lpLibFileName; return 0; }
void* LoadLibraryW(const wchar_t* lpLibFileName) { (void)lpLibFileName; return 0; }
BOOL FreeLibrary(void* hLibModule) { (void)hLibModule; return TRUE; }
void* (GetProcAddress)(void* hModule, const char* lpProcName) { (void)hModule; (void)lpProcName; return 0; }
unsigned long GetModuleFileNameA(void* hModule, char* lpFilename, unsigned long nSize)
{ (void)hModule; snprintf(lpFilename, nSize, "%s", g_storageDir); return (unsigned long)strlen(lpFilename); }

unsigned long GetCurrentDirectoryA(unsigned long nBufferLength, char* lpBuffer)
{ snprintf(lpBuffer, nBufferLength, "%s", g_storageDir); return (unsigned long)strlen(lpBuffer); }
BOOL SetCurrentDirectoryA(const char* lpPathName)
{ (void)lpPathName; return TRUE; }
unsigned long GetFileAttributesA(const char* lpFileName)
{
    struct stat st;
    if (!lpFileName || stat(lpFileName, &st) != 0) { Win32_SetLastError(ERROR_FILE_NOT_FOUND); return 0xFFFFFFFFu; }
    return FILE_ATTRIBUTE_NORMAL;
}
unsigned int GetDriveTypeA(const char* lpRootPathName)
{
    struct stat st;
    if (lpRootPathName && stat(lpRootPathName, &st) == 0)
    {
        if (S_ISDIR(st.st_mode)) return DRIVE_FIXED;
        return DRIVE_CDROM;
    }
    return DRIVE_UNKNOWN;
}

unsigned long GetFileVersionInfoSizeA(const char* lpFilename, unsigned long* lpdwHandle)
{ (void)lpFilename; if (lpdwHandle) *lpdwHandle = 0; return 0; }
BOOL GetFileVersionInfoA(const char* lpFilename, unsigned long dwHandle, unsigned long dwLen, void* lpData)
{ (void)lpFilename; (void)dwHandle; (void)dwLen; (void)lpData; return FALSE; }
BOOL VerQueryValueA(const void* pBlock, const char* lpSubBlock, void** lplpBuffer, unsigned int* puLen)
{ (void)pBlock; (void)lpSubBlock; if (lplpBuffer) *lplpBuffer = 0; if (puLen) *puLen = 0; return FALSE; }

void* CreateWindowExA(unsigned long dwExStyle, const char* lpClassName, const char* lpWindowName,
                      unsigned long dwStyle, int x, int y, int nWidth, int nHeight,
                      void* hWndParent, void* hMenu, void* hInstance, void* lpParam)
{
    (void)dwExStyle; (void)lpClassName; (void)lpWindowName; (void)dwStyle;
    (void)x; (void)y; (void)nWidth; (void)nHeight;
    (void)hWndParent; (void)hMenu; (void)hInstance; (void)lpParam;
    return (void*)(uintptr_t)0x1;
}
unsigned short RegisterClassA(const void* lpWndClass) { (void)lpWndClass; return 1; }
BOOL ShowWindow(void* hWnd, int nCmdShow) { (void)hWnd; (void)nCmdShow; return TRUE; }
BOOL UpdateWindow(void* hWnd) { (void)hWnd; return TRUE; }
BOOL GetClientRect(void* hWnd, void* lpRect)
{ (void)hWnd; RECT* r = (RECT*)lpRect; r->left=0; r->top=0; r->right=800; r->bottom=600; return TRUE; }
BOOL GetWindowRect(void* hWnd, void* lpRect)
{ (void)hWnd; RECT* r = (RECT*)lpRect; r->left=0; r->top=0; r->right=800; r->bottom=600; return TRUE; }
BOOL SetWindowPos(void* hWnd, void* hWndInsertAfter, int x, int y, int cx, int cy, unsigned int uFlags)
{ (void)hWnd; (void)hWndInsertAfter; (void)x; (void)y; (void)cx; (void)cy; (void)uFlags; return TRUE; }
long SetWindowLongA(void* hWnd, int nIndex, long dwNewLong) { (void)hWnd; (void)nIndex; return dwNewLong; }
long GetWindowLongA(void* hWnd, int nIndex) { (void)hWnd; (void)nIndex; return 0; }
void* LoadIconA(void* hInstance, const char* lpIconName) { (void)hInstance; (void)lpIconName; return 0; }
void* LoadCursorA(void* hInstance, const char* lpCursorName) { (void)hInstance; (void)lpCursorName; return 0; }
void* GetDC(void* hWnd) { (void)hWnd; return (void*)(uintptr_t)0x1; }
int ReleaseDC(void* hWnd, void* hDC) { (void)hWnd; (void)hDC; return 1; }
int GetDeviceCaps(void* hdc, int nIndex) { (void)hdc; return (nIndex == BITSPIXEL) ? 16 : 32; }
BOOL DestroyWindow(void* hWnd) { (void)hWnd; return TRUE; }
BOOL IsWindow(void* hWnd) { (void)hWnd; return TRUE; }

int MessageBoxA(void* hWnd, const char* lpText, const char* lpCaption, unsigned int uType)
{
    (void)hWnd; (void)lpCaption; (void)uType;
    fprintf(stderr, "[msgbox] %s\n", lpText ? lpText : "");
    return 1;
}

BOOL PeekMessageA(void* lpMsg, void* hWnd, unsigned int wMsgFilterMin, unsigned int wMsgFilterMax, unsigned int wRemoveMsg)
{ (void)hWnd; (void)wMsgFilterMin; (void)wMsgFilterMax; (void)wRemoveMsg; (void)lpMsg; return FALSE; }
BOOL TranslateMessage(const void* lpMsg) { (void)lpMsg; return FALSE; }
long DispatchMessageA(const void* lpMsg) { (void)lpMsg; return 0; }
BOOL PostMessageA(void* hWnd, unsigned int Msg, void* wParam, void* lParam)
{ (void)hWnd; (void)Msg; (void)wParam; (void)lParam; return TRUE; }
void PostQuitMessage(int nExitCode) { exit(nExitCode); }
long DefWindowProcA(void* hWnd, unsigned int Msg, void* wParam, void* lParam)
{ (void)hWnd; (void)Msg; (void)wParam; (void)lParam; return 0; }
int ShowCursor(BOOL bShow) { (void)bShow; return 0; }

long CoInitialize(void* pvReserved) { (void)pvReserved; return 0; }
void CoUninitialize(void) {}
long CoCreateInstance(const void* rclsid, void* pUnkOuter, unsigned long dwClsContext,
                      const void* riid, void** ppv)
{ (void)rclsid; (void)pUnkOuter; (void)dwClsContext; (void)riid; if (ppv) *ppv = 0; return (long)0x80004001L; }

extern "C" {
HRESULT DirectInputCreateA(void* hinst, DWORD dwVersion, LPDIRECTINPUTA* lplpDirectInput, void* pUnkOuter)
{ (void)hinst; (void)dwVersion; (void)pUnkOuter; if (lplpDirectInput) *lplpDirectInput = GetFakeDirectInput(); return S_OK; }
HRESULT DirectInput8Create(void* hinst, DWORD dwVersion, const GUID& rguid, void** ppvOut, void* pUnkOuter)
{ (void)hinst; (void)dwVersion; (void)rguid; (void)pUnkOuter; if (ppvOut) *ppvOut = 0; return 0x80004001L;  }
}

static IDirectInputDeviceA g_fakeInputDevice;
static IDirectInputA g_fakeDirectInput;
IDirectInputDeviceA* GetFakeInputDevice() { return &g_fakeInputDevice; }
IDirectInputA* GetFakeDirectInput() { return &g_fakeDirectInput; }

#define G2_KB_EVENTS 512

static pthread_mutex_t g_kbLock = PTHREAD_MUTEX_INITIALIZER;
static unsigned char g_kbState[256];
static DIDEVICEOBJECTDATA g_kbEvents[G2_KB_EVENTS];
static int g_kbHead = 0;
static int g_kbTail = 0;
static int g_kbCount = 0;
static unsigned long g_kbSeq = 0;

void G2_InputKey(int scan, int down)
{
    if (scan < 0 || scan >= 256)
    {
        return;
    }

    __android_log_print(ANDROID_LOG_ERROR, "GTA2-port", "G2_InputKey scan=%d(%02X) down=%d", scan, scan, down);

    pthread_mutex_lock(&g_kbLock);
    if (down)
    {
        g_kbState[scan] |= 0x80;
    }
    else
    {
        g_kbState[scan] &= (unsigned char)~0x80;
    }
    if (g_kbCount < G2_KB_EVENTS)
    {
        g_kbEvents[g_kbTail].dwOfs = (unsigned long)scan;
        g_kbEvents[g_kbTail].dwData = down ? 0x80 : 0;
        g_kbEvents[g_kbTail].dwTimeStamp = 0;
        g_kbEvents[g_kbTail].dwSequence = g_kbSeq++;
        g_kbEvents[g_kbTail].uAppData = 0;
        g_kbTail = (g_kbTail + 1) % G2_KB_EVENTS;
        g_kbCount++;
    }
    pthread_mutex_unlock(&g_kbLock);
}

void G2_InputGetState256(unsigned char* out)
{
    if (!out)
    {
        return;
    }
    pthread_mutex_lock(&g_kbLock);
    memcpy(out, g_kbState, 256);
    pthread_mutex_unlock(&g_kbLock);
}

int G2_InputGetEvents(void* rgdod, unsigned int cap, int bPeek)
{
    pthread_mutex_lock(&g_kbLock);
    unsigned int n = 0;
    if (rgdod)
    {
        DIDEVICEOBJECTDATA* out = (DIDEVICEOBJECTDATA*)rgdod;
        while (n < cap && g_kbCount > 0)
        {
            out[n] = g_kbEvents[g_kbHead];
            if (!bPeek)
            {
                g_kbHead = (g_kbHead + 1) % G2_KB_EVENTS;
                g_kbCount--;
            }
            n++;
        }
    }
    else
    {
        n = (unsigned int)g_kbCount;
    }
    pthread_mutex_unlock(&g_kbLock);
    return (int)n;
}

void G2_InputClear()
{
    pthread_mutex_lock(&g_kbLock);
    memset(g_kbState, 0, sizeof(g_kbState));
    g_kbHead = g_kbTail = g_kbCount = 0;
    pthread_mutex_unlock(&g_kbLock);
}

void* GetCommandLineA(void) { static char dummy[8] = "gta2"; return dummy; }
BOOL ShellExecuteA(void* hwnd, const char* lpOperation, const char* lpFile, const char* lpParameters,
                   const char* lpDirectory, int nShowCmd)
{ (void)hwnd; (void)lpOperation; (void)lpFile; (void)lpParameters; (void)lpDirectory; (void)nShowCmd; return FALSE; }
void* InterlockedExchange(void* t, long vl) { long* p = (long*)t; long old = *p; *p = vl; return (void*)(intptr_t)old; }
void* LocalAlloc(unsigned int uFlags, unsigned long uBytes) { (void)uFlags; return malloc(uBytes); }
void* LocalFree(void* hMem) { free(hMem); return 0; }

void* FindFirstFileA(const char* lpFileName, void* lpFindFileData)
{ (void)lpFileName; (void)lpFindFileData; Win32_SetLastError(ERROR_FILE_NOT_FOUND); return INVALID_HANDLE_VALUE; }
BOOL FindNextFileA(void* hFindFile, void* lpFindFileData)
{ (void)hFindFile; (void)lpFindFileData; return FALSE; }
BOOL FindClose(void* hFindFile) { (void)hFindFile; return TRUE; }

void* CreateFileA(const char* lpFileName, unsigned long dwDesiredAccess, unsigned long dwShareMode,
                  void* lpSecAttrs, unsigned long dwCreationDisposition, unsigned long dwFlagsAndAttributes,
                  void* hTemplateFile)
{
    (void)lpSecAttrs; (void)hTemplateFile; (void)dwShareMode; (void)dwFlagsAndAttributes;
    if (!lpFileName) return INVALID_HANDLE_VALUE;
    const char* m = (dwCreationDisposition == CREATE_ALWAYS) ? "wb" : "rb";
    FILE* f = fopen(lpFileName, m);
    (void)dwDesiredAccess;
    if (!f) { Win32_SetLastError(errno); return INVALID_HANDLE_VALUE; }
    return (void*)f;
}
BOOL ReadFile(void* hFile, void* lpBuffer, unsigned long nNumberOfBytesToRead,
              unsigned long* lpNumberOfBytesRead, void* lpOverlapped)
{ (void)lpOverlapped; size_t n = fread(lpBuffer, 1, nNumberOfBytesToRead, (FILE*)hFile); if (lpNumberOfBytesRead) *lpNumberOfBytesRead = (unsigned long)n; return n == nNumberOfBytesToRead; }
BOOL WriteFile(void* hFile, const void* lpBuffer, unsigned long nNumberOfBytesToWrite,
               unsigned long* lpNumberOfBytesWritten, void* lpOverlapped)
{ (void)lpOverlapped; size_t n = fwrite(lpBuffer, 1, nNumberOfBytesToWrite, (FILE*)hFile); if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = (unsigned long)n; return n == nNumberOfBytesToWrite; }
unsigned long SetFilePointer(void* hFile, long lDistanceToMove, long* lpDistanceToMoveHigh, unsigned long dwMoveMethod)
{ (void)lpDistanceToMoveHigh; fseek((FILE*)hFile, lDistanceToMove, (int)dwMoveMethod); return (unsigned long)ftell((FILE*)hFile); }
unsigned long GetFileSize(void* hFile, unsigned long* lpFileSizeHigh)
{ (void)lpFileSizeHigh; long c = ftell((FILE*)hFile); fseek((FILE*)hFile, 0, SEEK_END); long e = ftell((FILE*)hFile); fseek((FILE*)hFile, c, SEEK_SET); return (unsigned long)(e >= 0 ? e : 0); }

#include <stdarg.h>

int GTA2_swprintf(wchar_t* buf, const wchar_t* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    wchar_t* out = buf;
    const wchar_t* f = fmt;

    while (*f)
    {
        if (*f != L'%')
        {
            *out++ = *f++;
            continue;
        }

        f++;


        int leftAlign = 0, zeroPad = 0;
        int plusSign = 0, spaceSign = 0, altForm = 0;
        int width = 0;
        int done = 0;

        while (!done && *f)
        {
            switch (*f)
            {
                case L'-': leftAlign = 1; f++; break;
                case L'0': zeroPad = 1; f++; break;
                case L'+': plusSign = 1; f++; break;
                case L' ': spaceSign = 1; f++; break;
                case L'#': altForm = 1; f++; break;
                default: done = 1; break;
            }
        }


        while (*f >= L'0' && *f <= L'9')
        {
            width = width * 10 + (*f - L'0');
            f++;
        }


        if (*f == L'l' || *f == L'h')
        {
            f++;
        }

        wchar_t tmp[80];
        int tmpLen = 0;

        switch (*f)
        {
            case L'd':
            case L'i':
            {
                long long v = va_arg(args, s32);
                if (v < 0) { tmp[tmpLen++] = L'-'; v = -v; }
                else if (plusSign) { tmp[tmpLen++] = L'+'; }
                else if (spaceSign) { tmp[tmpLen++] = L' '; }
                int digits = 0;
                if (v == 0) { digits = 1; }
                long long t = v;
                while (t) { t /= 10; digits++; }
                int zeros = (zeroPad && !leftAlign) ? width - tmpLen - digits : 0;
                if (zeros < 0) zeros = 0;

                for (int i = 0; i < zeros && !leftAlign; i++) tmp[tmpLen++] = L'0';
                static const wchar_t* dz = L"0123456789";
                wchar_t dig[32]; int dn = 0;
                if (v == 0) dig[dn++] = L'0';
                while (v) { dig[dn++] = dz[v % 10]; v /= 10; }
                for (int i = dn - 1; i >= 0; i--) tmp[tmpLen++] = dig[i];
                break;
            }
            case L'u':
            {
                unsigned long long v = va_arg(args, u32);
                int digits = (v == 0) ? 1 : 0;
                unsigned long long t = v;
                while (t) { t /= 10; digits++; }
                int zeros = (zeroPad && !leftAlign) ? width - tmpLen - digits : 0;
                if (zeros < 0) zeros = 0;
                for (int i = 0; i < zeros && !leftAlign; i++) tmp[tmpLen++] = L'0';
                static const wchar_t* dz = L"0123456789";
                wchar_t dig[32]; int dn = 0;
                if (v == 0) dig[dn++] = L'0';
                while (v) { dig[dn++] = dz[v % 10]; v /= 10; }
                for (int i = dn - 1; i >= 0; i--) tmp[tmpLen++] = dig[i];
                break;
            }
            case L'x':
            case L'X':
            {
                unsigned long long v = va_arg(args, u32);
                static const wchar_t* lx = L"0123456789abcdef";
                static const wchar_t* ux = L"0123456789ABCDEF";
                const wchar_t* dz = (*f == L'x') ? lx : ux;
                int digits = (v == 0) ? 1 : 0;
                unsigned long long t = v;
                while (t) { t /= 16; digits++; }
                if (altForm) { tmp[tmpLen++] = L'0'; tmp[tmpLen++] = (*f == L'x') ? L'x' : L'X'; }
                int zeros = (zeroPad && !leftAlign) ? width - tmpLen - digits : 0;
                if (zeros < 0) zeros = 0;
                for (int i = 0; i < zeros && !leftAlign; i++) tmp[tmpLen++] = L'0';
                wchar_t dig[32]; int dn = 0;
                if (v == 0) dig[dn++] = L'0';
                while (v) { dig[dn++] = dz[v % 16]; v /= 16; }
                for (int i = dn - 1; i >= 0; i--) tmp[tmpLen++] = dig[i];
                break;
            }
            case L'c':
                tmp[tmpLen++] = (wchar_t)va_arg(args, int);
                break;
            case L's':
            {
                const wchar_t* s = va_arg(args, const wchar_t*);
                if (!s) s = L"(null)";
                while (*s && tmpLen < (int)sizeof(tmp) - 1) tmp[tmpLen++] = *s++;
                break;
            }
            case L'p':
            {
                void* p = va_arg(args, void*);
                tmp[tmpLen++] = L'0'; tmp[tmpLen++] = L'x';
                unsigned long long v = (unsigned long long)(uintptr_t)p;
                static const wchar_t* dz = L"0123456789abcdef";
                wchar_t dig[32]; int dn = 0;
                if (v == 0) dig[dn++] = L'0';
                while (v) { dig[dn++] = dz[v % 16]; v /= 16; }
                for (int i = dn - 1; i >= 0; i--) tmp[tmpLen++] = dig[i];
                break;
            }
            case L'%':
                tmp[tmpLen++] = L'%';
                break;
            case L'\0':
                break;
            default:
                tmp[tmpLen++] = L'%';
                break;
        }
        f++;


        for (int i = 0; i < width - tmpLen && !leftAlign; i++) *out++ = L' ';
        for (int i = 0; i < tmpLen; i++) *out++ = tmp[i];
        for (int i = 0; i < width - tmpLen && leftAlign; i++) *out++ = L' ';
    }

    *out = L'\0';
    va_end(args);
    return (int)(out - buf);
}

typedef struct _g2_findstate
{
    DIR* dir;
    char pattern[260];
    int issued;
} _g2_findstate;

static int _g2_fnmatch_pattern(const char* pat, const char* name)
{
    const char* p = pat;
    const char* n = name;
    while (*p)
    {
        if (*p == '*')
        {
            p++;
            if (!*p) return 1;
            for (const char* s = n; s; s++)
            {
                if (_g2_fnmatch_pattern(p, s)) return 1;
                if (!*s) break;
            }
            return 0;
        }
        else if (*p == '?')
        {
            if (!*n) return 0;
            p++; n++;
        }
        else
        {
            if (*p != *n) return 0;
            p++; n++;
        }
    }
    return *n == 0;
}

static int _g2_fnmatch_entry(const char* pat, const char* name)
{
    if (name[0] == '.') return 0;
    return _g2_fnmatch_pattern(pat, name);
}

long _findfirst(const char* pattern, struct _finddata_t* fd)
{

    char dirPath[260] = ".";
    const char* slash = strrchr(pattern, '/');
    const char* bslash = strrchr(pattern, '\\');
    const char* namePattern = pattern;
    if (slash && (!bslash || slash > bslash)) { namePattern = slash + 1; }
    else if (bslash) { namePattern = bslash + 1; }
    if (namePattern != pattern)
    {
        size_t len = namePattern - pattern - 1;
        if (len >= sizeof(dirPath)) len = sizeof(dirPath) - 1;
        memcpy(dirPath, pattern, len);
        dirPath[len] = 0;
    }

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) __android_log_print(ANDROID_LOG_DEBUG, "GTA2-find", "pattern='%s' dir='%s' cwd='%s' dataDir='%s'", pattern, dirPath, cwd, G2Platform_GetGameDataDir());
    else __android_log_print(ANDROID_LOG_DEBUG, "GTA2-find", "pattern='%s' dir='%s' (cwd unknown, errno=%d)", pattern, dirPath, errno);

    _g2_findstate* st = (_g2_findstate*)calloc(1, sizeof(_g2_findstate));
    if (!st) return -1;
    snprintf(st->pattern, sizeof(st->pattern), "%s", namePattern);
    st->dir = opendir(dirPath);
    if (!st->dir)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "GTA2-find", "opendir failed: dir='%s' errno=%d", dirPath, errno);

        char alt[1024];
        snprintf(alt, sizeof(alt), "%s/%s", G2Platform_GetGameDataDir(), dirPath);
        char altResolved[1024];
        if (G2_ResolvePathCI(alt, altResolved, sizeof(altResolved)) && altResolved[0])
        {
            strncpy(alt, altResolved, sizeof(alt) - 1);
            alt[sizeof(alt) - 1] = 0;
        }
        st->dir = opendir(alt[0] ? alt : dirPath);
        if (!st->dir)
        {
            __android_log_print(ANDROID_LOG_DEBUG, "GTA2-find", "opendir fallback failed: alt='%s' errno=%d", alt, errno);
            free(st);
            return -1;
        }
    }
    long handle = (long)(intptr_t)st;
    if (_findnext(handle, fd) != 0)
    {
        _findclose(handle);
        return -1;
    }
    return handle;
}

int _findnext(long handle, struct _finddata_t* fd)
{
    _g2_findstate* st = (_g2_findstate*)(intptr_t)handle;
    if (!st || !st->dir) return -1;
struct dirent* de;
    while ((de = readdir(st->dir)) != NULL)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "GTA2-find", "  entry '%s' match('%s')", de->d_name, st->pattern);
        if (!_g2_fnmatch_entry(st->pattern, de->d_name)) continue;
        snprintf(fd->name, sizeof(fd->name), "%s", de->d_name);
        fd->size = 0;
        fd->attrib = _A_NORMAL;
        struct stat sb;
        if (stat(de->d_name, &sb) == 0)
        {
            fd->size = (unsigned long)sb.st_size;
            fd->time_write = (long)sb.st_mtime;
            if (S_ISDIR(sb.st_mode)) fd->attrib |= _A_SUBDIR;
        }
        return 0;
    }
    return -1;
}

int _findclose(long handle)
{
    _g2_findstate* st = (_g2_findstate*)(intptr_t)handle;
    if (!st) return -1;
    if (st->dir) closedir(st->dir);
    free(st);
    return 0;
}
char* _itoa(int value, char* buffer, int radix)
{
    if (!buffer) return 0;
    char tmp[32];
    if (radix == 10)
    {
        snprintf(tmp, sizeof(tmp), "%d", value);
    }
    else if (radix == 16)
    {
        snprintf(tmp, sizeof(tmp), "%x", value);
    }
    else if (radix == 8)
    {
        snprintf(tmp, sizeof(tmp), "%o", value);
    }
    else
    {
        strcpy(tmp, buffer);
    }
    strcpy(buffer, tmp);
    return buffer;
}

BOOL GetKeyboardLayoutNameA(char* pwszKLID)
{
    if (pwszKLID)
    {
        strcpy(pwszKLID, "00000409");
    }
    return 1;
}

wchar_t* _itow(int value, wchar_t* buffer, int radix)
{
    if (!buffer) return 0;
    char tmp[32];
    if (radix == 10)
    {
        snprintf(tmp, sizeof(tmp), "%d", value);
    }
    else if (radix == 16)
    {
        snprintf(tmp, sizeof(tmp), "%x", value);
    }
    else
    {
        snprintf(tmp, sizeof(tmp), "%o", value);
    }
    for (int i = 0; tmp[i]; ++i)
    {
        buffer[i] = (wchar_t)tmp[i];
    }
    buffer[strlen(tmp)] = 0;
    return buffer;
}

extern "C" {

size_t wcslen(const wchar_t* s)
{
    size_t n = 0;
    while (s && s[n])
    {
        n++;
    }
    return n;
}

wchar_t* wcscpy(wchar_t* dst, const wchar_t* src)
{
    size_t i = 0;
    do
    {
        dst[i] = src[i];
    } while (src[i++]);
    return dst;
}

wchar_t* wcsncpy(wchar_t* dst, const wchar_t* src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i]; i++)
    {
        dst[i] = src[i];
    }
    for (; i < n; i++)
    {
        dst[i] = 0;
    }
    return dst;
}

int wcscmp(const wchar_t* a, const wchar_t* b)
{
    while (*a && *b && *a == *b)
    {
        a++;
        b++;
    }
    return (int)(*a - *b);
}

int wcsncmp(const wchar_t* a, const wchar_t* b, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
    {
        if (a[i] != b[i])
        {
            return (int)(a[i] - b[i]);
        }
        if (!a[i])
        {
            return 0;
        }
    }
    return 0;
}

wchar_t* wcscat(wchar_t* dst, const wchar_t* src)
{
    size_t i = 0;
    size_t j = 0;
    while (dst[i])
        i++;
    do
    {
        dst[i + j] = src[j];
    } while (src[j++]);
    return dst;
}

wchar_t* wcschr(wchar_t* s, wchar_t c)
{
    while (*s)
    {
        if (*s == c)
        {
            return s;
        }
        s++;
    }
    return c ? 0 : s;
}

wchar_t* wcsrchr(wchar_t* s, wchar_t c)
{
    wchar_t* last = 0;
    do
    {
        if (*s == c)
        {
            last = s;
        }
    } while (*s++);
    return last;
}

wchar_t* wcsstr(wchar_t* haystack, const wchar_t* needle)
{
    if (!*needle)
    {
        return haystack;
    }
    for (; *haystack; haystack++)
    {
        const wchar_t* h = haystack;
        const wchar_t* n = needle;
        while (*h && *n && *h == *n)
        {
            h++;
            n++;
        }
        if (!*n)
        {
            return haystack;
        }
    }
    return 0;
}

}
