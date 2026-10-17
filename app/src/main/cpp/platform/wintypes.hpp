#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint8_t   BYTE;
typedef uint16_t  WORD;
typedef uint32_t  DWORD;
typedef uint64_t  ULONGLONG;
typedef int32_t   LONG;
typedef int64_t   LONGLONG;
typedef uint32_t  ULONG32;
typedef long      ULONG;
typedef DWORD*    LPDWORD;
typedef BYTE*     LPBYTE;
typedef char      CHAR;
typedef wchar_t   WCHAR;
typedef float     FLOAT;
typedef char*     LPSTR;
typedef const char*   LPCSTR;
typedef wchar_t*  LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef void*     LPVOID;
typedef const void*   LPCVOID;
typedef int       BOOL;
typedef unsigned int  UINT;
typedef signed int    INT;
typedef intptr_t  INT_PTR;
typedef uintptr_t UINT_PTR;
typedef uintptr_t ULONG_PTR;
typedef uintptr_t DWORD_PTR;
typedef uintptr_t SIZE_T;
typedef intptr_t  SSIZE_T;
typedef intptr_t  LRESULT;
typedef intptr_t  WPARAM;
typedef intptr_t  LPARAM;

typedef struct HWND__*      HWND;
typedef struct HINSTANCE__* HINSTANCE;
typedef struct HMODULE__*   HMODULE;
typedef struct HKEY__*      HKEY;
typedef struct HDC__*       HDC;
typedef struct HICON__*     HICON;
typedef struct HCURSOR__*   HCURSOR;
typedef struct HBRUSH__*    HBRUSH;
typedef struct HMENU__*     HMENU;
typedef struct HBITMAP__*   HBITMAP;
typedef struct HGDIOBJ__*   HGDIOBJ;
typedef struct HEVENT_MONITOR__* HMONITOR;
typedef void*               HANDLE;

typedef struct IUnknown IUnknown;
typedef struct IDirectDraw IDirectDraw;
typedef struct IDirectDrawSurface IDirectDrawSurface;
typedef struct IDirectDraw2 IDirectDraw2;
typedef struct IDirectDrawSurface2 IDirectDrawSurface2;
typedef struct IDirectDrawSurface4 IDirectDrawSurface4;
typedef struct IDirectDrawClipper IDirectDrawClipper;
typedef struct IDirectInputA IDirectInputA;
typedef struct IDirectInputDeviceA IDirectInputDeviceA;

typedef long HRESULT;
typedef HRESULT(*FARPROC)(void);

#define TRUE  1
#define FALSE 0
#ifndef NULL
#define NULL  0
#endif

#define MAX_PATH 260
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

#ifndef __stdcall
#define __stdcall
#endif
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(x)
#endif

#ifndef FAR
#define FAR
#endif
#ifndef NEAR
#define NEAR
#endif
#ifndef PASCAL
#define PASCAL __stdcall
#endif
#define WINAPI __stdcall
#define CALLBACK __stdcall

#define S_OK            ((HRESULT)0L)
#define S_FALSE         ((HRESULT)1L)
#define E_FAIL          ((HRESULT)0x80004005L)
#define E_NOINTERFACE   ((HRESULT)0x80004002L)
#define E_OUTOFMEMORY   ((HRESULT)0x8007000EL)
#define E_INVALIDARG    ((HRESULT)0x80070057L)
#define E_NOTIMPL       ((HRESULT)0x80004001L)

#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr)    (((HRESULT)(hr)) < 0)

typedef struct _RECT { LONG left, top, right, bottom; } RECT;
typedef RECT* LPRECT;
typedef struct tagPOINT { LONG x, y; } POINT;
typedef struct tagSIZE { LONG cx, cy; } SIZE;

typedef struct _FILETIME { DWORD dwLowDateTime, dwHighDateTime; } FILETIME;
typedef struct _SYSTEMTIME { WORD wYear,wMonth,wDayOfWeek,wDay,wHour,wMinute,wSecond,wMilliseconds; } SYSTEMTIME;

typedef struct _SECURITY_ATTRIBUTES { DWORD nLength; void* lpSecurityDescriptor; BOOL bInheritHandle; } SECURITY_ATTRIBUTES;

typedef struct _STARTUPINFOA { DWORD cb; char* lpReserved; char* lpDesktop; char* lpTitle; DWORD dwX,dwY,dwXSize,dwYSize,dwFlags,dwFillAttribute; WORD wShowWindow,wReserved2; BYTE* cbReserved2; char* lpReserved3; HANDLE hStdInput, hStdOutput, hStdError; } STARTUPINFOA;
typedef struct _STARTUPINFOW { DWORD cb; wchar_t* lpReserved; wchar_t* lpDesktop; wchar_t* lpTitle; DWORD dwX,dwY,dwXSize,dwYSize,dwFlags,dwFillAttribute; WORD wShowWindow,wReserved2; BYTE* cbReserved2; wchar_t* lpReserved3; HANDLE hStdInput, hStdOutput, hStdError; } STARTUPINFOW;

typedef struct _OSVERSIONINFOA { DWORD dwOSVersionInfoSize; DWORD dwMajorVersion,dwMinorVersion,dwBuildNumber,dwPlatformId; CHAR szCSDVersion[128]; } OSVERSIONINFOA;
typedef struct _OSVERSIONINFOW { DWORD dwOSVersionInfoSize; DWORD dwMajorVersion,dwMinorVersion,dwBuildNumber,dwPlatformId; WCHAR szCSDVersion[128]; } OSVERSIONINFOW;

typedef struct _VS_FIXEDFILEINFO {
    DWORD dwSignature; DWORD dwStrucVersion;
    DWORD dwFileVersionMS, dwFileVersionLS;
    DWORD dwProductVersionMS, dwProductVersionLS;
    DWORD dwFileFlagsMask, dwFileFlags;
    DWORD dwFileOS, dwFileType, dwFileSubtype;
    DWORD dwFileDateMS, dwFileDateLS;
} VS_FIXEDFILEINFO;

typedef struct tagMSG { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; POINT pt; } MSG;
typedef struct tagWNDCLASSA {
    UINT style;
    LRESULT(*lpfnWndProc)(HWND, UINT, WPARAM, LPARAM);
    int cbClsExtra; int cbWndExtra;
    HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor; HBRUSH hbrBackground;
    LPCSTR lpszMenuName; LPCSTR lpszClassName;
} WNDCLASSA;
typedef struct tagWINDOWPOS { HWND hwnd; HWND hwndInsertAfter; int x,y,cx,cy; UINT flags; } WINDOWPOS;

typedef struct _GUID { unsigned long Data1; unsigned short Data2, Data3; unsigned char Data4[8]; } GUID;

#define ERROR_SUCCESS 0
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_ACCESS_DENIED 5
#define ERROR_NOT_READY 21
#define ERROR_INVALID_PARAMETER 87

#define REG_NONE 0
#define REG_SZ 1
#define REG_EXPAND_SZ 2
#define REG_BINARY 3
#define REG_DWORD 4
#define REG_MULTI_SZ 7
#define KEY_QUERY_VALUE 0x0001
#define KEY_SET_VALUE 0x0002
#define KEY_CREATE_SUB_KEY 0x0004
#define KEY_ALL_ACCESS (0x0001|0x0002|0x0004|0x0008|0x0010|0x10000|0x20000)

#define HKEY_CLASSES_ROOT   ((HKEY)(intptr_t)0x80000000L)
#define HKEY_CURRENT_USER   ((HKEY)(intptr_t)0x80000001L)
#define HKEY_LOCAL_MACHINE  ((HKEY)(intptr_t)0x80000002L)
#define HKEY_USERS          ((HKEY)(intptr_t)0x80000003L)
typedef HKEY* PHKEY;

#define DRIVE_UNKNOWN 0
#define DRIVE_REMOVABLE 2
#define DRIVE_FIXED 3
#define DRIVE_CDROM 5

#define GWL_STYLE (-16)
#define WS_OVERLAPPEDWINDOW 0x00CF0000u
#define SW_SHOW 5
#define SW_SHOWNORMAL 1
#define SW_SHOWMINNOACTIVE 7
#define SWP_NOSIZE 0x0001
#define SWP_NOZORDER 0x0004
#define SWP_NOMOVE 0x0002
#define SWP_NOACTIVATE 0x0010
#define WM_QUIT 0x0012
#define WM_ACTIVATE 0x0006
#define WM_SETFOCUS 0x0007
#define WM_KILLFOCUS 0x0008
#define WM_SIZE 0x0005
#define WM_WINDOWPOSCHANGED 0x0047
#define WM_WINDOWPOSCHANGING 0x0046
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSCOMMAND 0x0112
#define WM_DESTROY 0x0002
#define WM_SIZING 0x0214
#define WA_ACTIVE 1
#define WA_CLICKACTIVE 2
#define WA_INACTIVE 0
#define VK_RETURN 0x0D
#define VK_MENU 0x12
#define VK_F10 0x79
#define VK_LMENU 0xA4
#define SC_MONITORPOWER 0xF170
#define SC_SCREENSAVE 0xF140
#define SC_MAXIMIZE 0xF030
#define SC_MINIMIZE 0xF020
#define BITSPIXEL 12
#define GENERIC_READ 0x80000000
#define GENERIC_WRITE 0x40000000
#define CREATE_ALWAYS 2
#define CREATE_NEW 1
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define FILE_SHARE_READ 0x1
#define FILE_SHARE_WRITE 0x2
#define FILE_ATTRIBUTE_NORMAL 0x80
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2
#define CS_VREDRAW 0x1
#define CS_HREDRAW 0x2
#define MB_OK 0
#define MUTEX_ALL_ACCESS 0x1F0001
#define VER_PLATFORM_WIN32_NT 2
#define VER_PLATFORM_WIN32_WINDOWS 1
#define DISCL_EXCLUSIVE 0x1
#define DISCL_FOREGROUND 0x2
#define DDSCAPS_BACKBUFFER 0x4
#define DDSD_CAPS 0x1
#define DDSCL_EXCLUSIVE 0x1
#define DDSCL_FULLSCREEN 0x2
#define DDSCL_NORMAL 0x8
#define DIEDFL_ATTACHEDONLY 0x1
#define DIGDD_PEEK 0x2
#define INFINITE 0xFFFFFFFFu

typedef struct IDirectDraw*          LPDIRECTDRAW;
typedef struct IDirectDrawSurface*   LPDIRECTDRAWSURFACE;
typedef struct IDirectDraw2*         LPDIRECTDRAW2;
typedef struct IDirectDrawSurface2*  LPDIRECTDRAWSURFACE2;
typedef struct IDirectDrawSurface4*  LPDIRECTDRAWSURFACE4;
typedef struct IDirectDrawClipper*   LPDIRECTDRAWCLIPPER;
typedef struct IDirectInputA*        LPDIRECTINPUTA;
typedef struct IDirectInputDeviceA*  LPDIRECTINPUTDEVICEA;
typedef struct DIDEVICEOBJECTDATA DIDEVICEOBJECTDATA;
typedef struct DIDATAFORMAT DIDATAFORMAT;

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

#define GUID_SysKeyboard __g_guid_sys_keyboard
#define GUID_SysMouse    __g_guid_sys_mouse

extern const GUID __g_guid_sys_keyboard;
extern const GUID __g_guid_sys_mouse;