#pragma once
// Capa de compatibilidad Win32 -> Android para el port nativo de GTA2.
// NO es emulacion: implementa con APIs nativas las pocas primitivas
// que el motor (gta2_re) necesita de windows.h.
#include "wintypes.hpp"

#ifndef _Return_type_success_
#define _Return_type_success_(x)
#endif
#ifndef _Check_return_
#define _Check_return_
#endif
#ifndef _Success_
#define _Success_(x)
#endif
#ifndef _Ret_maybenull_
#define _Ret_maybenull_
#endif
#ifndef _Post_ptr_invalid_
#define _Post_ptr_invalid_
#endif
#ifndef _In_
#define _In_
#endif
#ifndef _Out_
#define _Out_
#endif
#ifndef _Inout_
#define _Inout_
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---- alias _strnicmp/_stricmp (MSVC) a POSIX ---- */
#include <strings.h>
#define _strnicmp strncasecmp
#define _stricmp  strcasecmp
#define _strcmpi  strcasecmp

/* ---- registry emulado ---- */
long RegOpenKeyExA(HKEY hKey, const char* lpSubKey, DWORD ulOptions, DWORD samDesired, PHKEY phkResult);
long RegOpenKeyA(HKEY hKey, const char* lpSubKey, PHKEY phkResult);
long RegCreateKeyExA(HKEY hKey, const char* lpSubKey, DWORD Reserved, char* lpClass,
                     DWORD dwOptions, DWORD samDesired, void* lpSecurityAttributes,
                     PHKEY phkResult, DWORD* lpdwDisposition);
long RegQueryValueExA(HKEY hKey, const char* lpValueName, DWORD* lpReserved, DWORD* lpType,
                      unsigned char* lpData, DWORD* lpcbData);
long RegSetValueExA(HKEY hKey, const char* lpValueName, DWORD Reserved, DWORD dwType,
                    const unsigned char* lpData, DWORD cbData);
long RegDeleteValueA(HKEY hKey, const char* lpValueName);
long RegDeleteKeyA(HKEY hKey, const char* lpSubKey);
long RegCloseKey(HKEY hKey);
long RegOpenKeyW(HKEY hKey, const wchar_t* lpSubKey);

/* ---- LastError / debug ---- */
unsigned long Win32_GetLastError(void);
void Win32_SetLastError(unsigned long e);
#define GetLastError() Win32_GetLastError()
#define SetLastError(e) Win32_SetLastError((e))
void OutputDebugStringA(const char* s);
#define OutputDebugStringA(s) OutputDebugStringA(s)

/* ---- tiempo / proceso ---- */
unsigned long timeGetTime_impl(void);
#define timeGetTime() timeGetTime_impl()
#define GetTickCount() timeGetTime_impl()
unsigned long GetCurrentThreadId(void);
void* GetCurrentProcess(void);

void Windows_Sleep(unsigned long ms);
#define Sleep(ms) Windows_Sleep((ms))

BOOL GetVersionExA(void* lpOSVer);
BOOL GetVersionExW(void* lpOSP);

int GetSystemMetrics(int nIndex);

/* ---- mutex / handles ---- */
void* OpenMutexA(unsigned long dwDesiredAccess, BOOL bInheritHandle, const char* lpName);
void* CreateMutexA(void* lpAttr, BOOL bInitialOwner, const char* lpName);
BOOL ReleaseMutex(void* hMutex);
BOOL CloseHandle(void* hObject);

/* ---- modulos ---- */
void* GetModuleHandleA(const char* lpModuleName);
#define GetModuleHandle GetModuleHandleA
void* LoadLibraryA(const char* lpLibFileName);
void* LoadLibraryW(const wchar_t* lpLibFileName);
BOOL FreeLibrary(void* hLibModule);
void* (GetProcAddress)(void* hModule, const char* lpProcName);
unsigned long GetModuleFileNameA(void* hModule, char* lpFilename, unsigned long nSize);

/* ---- directorios ---- */
unsigned long GetCurrentDirectoryA(unsigned long nBufferLength, char* lpBuffer);
BOOL SetCurrentDirectoryA(const char* lpPathName);
unsigned long GetFileAttributesA(const char* lpFileName);
unsigned int GetDriveTypeA(const char* lpRootPathName);

/* ---- version info (GTA2.EXE) ---- */
unsigned long GetFileVersionInfoSizeA(const char* lpFilename, unsigned long* lpdwHandle);
BOOL GetFileVersionInfoA(const char* lpFilename, unsigned long dwHandle, unsigned long dwLen, void* lpData);
BOOL VerQueryValueA(const void* pBlock, const char* lpSubBlock, void** lplpBuffer, unsigned int* puLen);

/* ---- ventana ---- */
void* CreateWindowExA(unsigned long dwExStyle, const char* lpClassName, const char* lpWindowName,
                      unsigned long dwStyle, int x, int y, int nWidth, int nHeight,
                      void* hWndParent, void* hMenu, void* hInstance, void* lpParam);
unsigned short RegisterClassA(const void* lpWndClass);
BOOL ShowWindow(void* hWnd, int nCmdShow);
BOOL UpdateWindow(void* hWnd);
BOOL GetClientRect(void* hWnd, void* lpRect);
BOOL GetWindowRect(void* hWnd, void* lpRect);
BOOL SetWindowPos(void* hWnd, void* hWndInsertAfter, int x, int y, int cx, int cy, unsigned int uFlags);
long SetWindowLongA(void* hWnd, int nIndex, long dwNewLong);
long GetWindowLongA(void* hWnd, int nIndex);
void* LoadIconA(void* hInstance, const char* lpIconName);
void* LoadCursorA(void* hInstance, const char* lpCursorName);
void* GetDC(void* hWnd);
int ReleaseDC(void* hWnd, void* hDC);
int GetDeviceCaps(void* hdc, int nIndex);
BOOL DestroyWindow(void* hWnd);
int MessageBoxA(void* hWnd, const char* lpText, const char* lpCaption, unsigned int uType);

/* ---- mensajes ---- */
BOOL PeekMessageA(void* lpMsg, void* hWnd, unsigned int wMsgFilterMin, unsigned int wMsgFilterMax, unsigned int wRemoveMsg);
BOOL TranslateMessage(const void* lpMsg);
long DispatchMessageA(const void* lpMsg);
BOOL PostMessageA(void* hWnd, unsigned int Msg, void* wParam, void* lParam);
void PostQuitMessage(int nExitCode);
long DefWindowProcA(void* hWnd, unsigned int Msg, void* wParam, void* lParam);
BOOL IsWindow(void* hWnd);

/* ---- cursor ---- */
int ShowCursor(BOOL bShow);

/* ---- COM stubs ---- */
long CoInitialize(void* pvReserved);
void CoUninitialize(void);
long CoCreateInstance(const void* rclsid, void* pUnkOuter, unsigned long dwClsContext,
                      const void* riid, void** ppv);

/* ---- strings / helpers ---- */
void* GetCommandLineA(void);
BOOL ShellExecuteA(void* hwnd, const char* lpOperation, const char* lpFile, const char* lpParameters,
                   const char* lpDirectory, int nShowCmd);
void* InterlockedExchange(void* t, long vl);

/* ---- local alloc ---- */
void* LocalAlloc(unsigned int uFlags, unsigned long uBytes);
void* LocalFree(void* hMem);

/* ---- find first/next (io.h) ---- */
struct WIN32_FIND_DATAA_;
void* FindFirstFileA(const char* lpFileName, void* lpFindFileData);
BOOL FindNextFileA(void* hFindFile, void* lpFindFileData);
BOOL FindClose(void* hFindFile);

/* ---- keyboard layout ---- */
#define KL_NAMELENGTH 9
BOOL GetKeyboardLayoutNameA(char* pwszKLID);

/* ---- crear archivo (app endeuda poco, pero compatible) ---- */
void* CreateFileA(const char* lpFileName, unsigned long dwDesiredAccess, unsigned long dwShareMode,
                  void* lpSecAttrs, unsigned long dwCreationDisposition, unsigned long dwFlagsAndAttributes,
                  void* hTemplateFile);
BOOL ReadFile(void* hFile, void* lpBuffer, unsigned long nNumberOfBytesToRead,
              unsigned long* lpNumberOfBytesRead, void* lpOverlapped);
BOOL WriteFile(void* hFile, const void* lpBuffer, unsigned long nNumberOfBytesToWrite,
               unsigned long* lpNumberOfBytesWritten, void* lpOverlapped);
unsigned long SetFilePointer(void* hFile, long lDistanceToMove, long* lpDistanceToMoveHigh, unsigned long dwMoveMethod);
unsigned long GetFileSize(void* hFile, unsigned long* lpFileSizeHigh);
BOOL SetFilePointerEx(void* hFile, long long llDistance, long long* lpNewFilePointer, unsigned long dwMoveMethod);
BOOL GetFileSizeEx(void* hFile, long long* lpFileSize);

#ifdef __cplusplus
}
#endif

/* ---- swprintf estilo MSVC (sin arg de count); C++ linkage para no chocar con wchar.h ---- */
#ifdef __cplusplus
int GTA2_swprintf(wchar_t* buf, const wchar_t* fmt, ...);
#endif