#pragma once
// Compat DirectInput 5 -> Android para el port nativo de GTA2.
// Solo proporciona las declaraciones que las fuentes del motor necesitan
// para COMPILAR. La entrada real se sustituirá por un backend Android.
#include "wintypes.hpp"
#include <string.h>

// Backend de teclado emulado (implementado en wincompat.cpp).
#if defined(__cplusplus)
extern "C" {
#endif
void G2_InputKey(int scan, int down);
void G2_InputGetState256(unsigned char* out);
int G2_InputGetEvents(void* rgdod, unsigned int cap, int bPeek);
void G2_InputClear();
#if defined(__cplusplus)
}
#endif

#ifndef DI_OK
#define DI_OK 0
#endif
#ifndef S_FALSE_DI
#define S_FALSE_DI 1
#endif
#define DIERR_NOTACQUIRED       ((HRESULT)0x80040301L)
#define DIERR_INPUTLOST         ((HRESULT)0x80040306L)
#define DIERR_UNSUPPORTED       ((HRESULT)0x80040308L)
#define DIERR_DEVICENOTREG      ((HRESULT)0x80040309L)
#define DIERR_OBJECTNOTFOUND    ((HRESULT)0x80040302L)

#define DIENUM_STOP             1LL
#define DIENUM_CONTINUE         0LL
#define DIDEVTYPE_JOYSTICK      0x0004
#define DIDEVTYPE_DEVICE        0x0001
#define DIPH_DEVICE             0
#define DIPROP_BUFFERSIZE       _g_diprop_buffersize
#define DIDF_RELAXIS            0x00000001
#define DIDF_ABSAXIS            0x00000002

#define DISCL_BACKGROUND        0x2
#define DISCL_NONEXCLUSIVE      0x4
#define DISCL_BACKGROUND_GRAB   0x2

/* ---- Scancodes DirectInput (estandar) ---- */
#define DIK_ESCAPE      0x01
#define DIK_1           0x02
#define DIK_2           0x03
#define DIK_3           0x04
#define DIK_4           0x05
#define DIK_5           0x06
#define DIK_6           0x07
#define DIK_7           0x08
#define DIK_8           0x09
#define DIK_9           0x0A
#define DIK_0           0x0B
#define DIK_MINUS       0x0C
#define DIK_EQUALS      0x0D
#define DIK_BACK        0x0E
#define DIK_TAB         0x0F
#define DIK_Q           0x10
#define DIK_W           0x11
#define DIK_E           0x12
#define DIK_R           0x13
#define DIK_T           0x14
#define DIK_Y           0x15
#define DIK_U           0x16
#define DIK_I           0x17
#define DIK_O           0x18
#define DIK_P           0x19
#define DIK_LBRACKET    0x1A
#define DIK_RBRACKET    0x1B
#define DIK_RETURN      0x1C
#define DIK_LCONTROL    0x1D
#define DIK_A           0x1E
#define DIK_S           0x1F
#define DIK_D           0x20
#define DIK_F           0x21
#define DIK_G           0x22
#define DIK_H           0x23
#define DIK_J           0x24
#define DIK_K           0x25
#define DIK_L           0x26
#define DIK_SEMICOLON   0x27
#define DIK_APOSTROPHE  0x28
#define DIK_GRAVE       0x29
#define DIK_LSHIFT      0x2A
#define DIK_BACKSLASH   0x2B
#define DIK_Z           0x2C
#define DIK_X           0x2D
#define DIK_C           0x2E
#define DIK_V           0x2F
#define DIK_B           0x30
#define DIK_N           0x31
#define DIK_M           0x32
#define DIK_COMMA       0x33
#define DIK_PERIOD      0x34
#define DIK_SLASH       0x35
#define DIK_RSHIFT      0x36
#define DIK_MULTIPLY    0x37
#define DIK_LMENU       0x38
#define DIK_SPACE       0x39
#define DIK_CAPITAL     0x3A
#define DIK_NUMPAD0     0x52
#define DIK_NUMPAD1     0x4F
#define DIK_NUMPAD2     0x50
#define DIK_NUMPAD3     0x51
#define DIK_NUMPAD4     0x4B
#define DIK_NUMPAD5     0x4C
#define DIK_NUMPAD6     0x4D
#define DIK_NUMPAD7     0x47
#define DIK_NUMPAD8     0x48
#define DIK_NUMPAD9     0x49
#define DIK_MULTIPLY    0x37
#define DIK_ADD         0x4E
#define DIK_SUBTRACT    0x4A
#define DIK_DECIMAL     0x53
#define DIK_DIVIDE      0x35
#define DIK_F1          0x3B
#define DIK_F2          0x3C
#define DIK_F3          0x3D
#define DIK_F4          0x3E
#define DIK_F5          0x3F
#define DIK_F6          0x40
#define DIK_F7          0x41
#define DIK_F8          0x42
#define DIK_F9          0x43
#define DIK_F10         0x44
#define DIK_F11         0x57
#define DIK_F12         0x58
#define DIK_HOME        0xC7
#define DIK_UP          0xC8
#define DIK_PRIOR       0xC9
#define DIK_LEFT        0xCB
#define DIK_RIGHT       0xCD
#define DIK_NEXT        0xD1
#define DIK_DOWN        0xD0
#define DIK_DELETE      0xD3
#define DIK_INSERT      0xD2
#define DIK_BACKSPACE   0x0E

/* ---- Guids ---- */
#define DEFINE_DINPUT_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    const GUID name = { l, (unsigned short)(w1), (unsigned short)(w2), { b1, b2, b3, b4, b5, b6, b7, b8 } }

#ifndef IID_IDirectInputDevice2A
DEFINE_DINPUT_GUID(IID_IDirectInputDevice2A, 0x5944E662, 0xAA8A, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif
#ifndef IID_IDirectInput8A
DEFINE_DINPUT_GUID(IID_IDirectInput8A, 0xBF798031, 0x483A, 0x4DA2, 0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00);
#endif
#ifndef GUID_XAxis
DEFINE_DINPUT_GUID(GUID_XAxis, 0xA36D02E0, 0xC9F3, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif
#ifndef GUID_YAxis
DEFINE_DINPUT_GUID(GUID_YAxis, 0xA36D02E1, 0xC9F3, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif
#ifndef GUID_ZAxis
DEFINE_DINPUT_GUID(GUID_ZAxis, 0xA36D02E2, 0xC9F3, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif
#ifndef GUID_Key
DEFINE_DINPUT_GUID(GUID_Key, 0x55728220, 0xD33C, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif
#ifndef GUID_RxAxis
DEFINE_DINPUT_GUID(GUID_RxAxis, 0xA36D02E3, 0xC9F3, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif
#ifndef GUID_RyAxis
DEFINE_DINPUT_GUID(GUID_RyAxis, 0xA36D02E4, 0xC9F3, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif
#ifndef GUID_RzAxis
DEFINE_DINPUT_GUID(GUID_RzAxis, 0xA36D02E5, 0xC9F3, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif
#ifndef GUID_Slider
DEFINE_DINPUT_GUID(GUID_Slider, 0xA36D02E6, 0xC9F3, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif
#ifndef GUID_POV
DEFINE_DINPUT_GUID(GUID_POV, 0xA36D02F2, 0xC9F3, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif
#ifndef GUID_SysKeyboard
DEFINE_DINPUT_GUID(GUID_SysKeyboard, 0x6F1D2B61, 0xD5A0, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
#endif
DEFINE_DINPUT_GUID(_g_diprop_buffersize, 0x00000001, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

struct DIDEVICEOBJECTDATA
{
    DWORD dwOfs;
    DWORD dwData;
    DWORD dwTimeStamp;
    DWORD dwSequence;
    DWORD uAppData;
};
typedef DIDEVICEOBJECTDATA* LPDIDEVICEOBJECTDATA;

struct DIOBJECTDATAFORMAT
{
    const GUID* pguid;
    DWORD dwOfs;
    DWORD dwType;
    DWORD dwFlags;
};
typedef DIOBJECTDATAFORMAT* LPDIOBJECTDATAFORMAT;

struct DIDATAFORMAT
{
    DWORD dwSize;
    DWORD dwObjSize;
    DWORD dwFlags;
    DWORD dwDataSize;
    DWORD dwNumObjs;
    LPDIOBJECTDATAFORMAT rgodf;
};
typedef DIDATAFORMAT* LPDIDATAFORMAT;

struct DIPROPHEADER
{
    DWORD dwSize;
    DWORD dwHeaderSize;
    DWORD dwObj;
    DWORD dwHow;
};

struct DIPROPDWORD
{
    DIPROPHEADER diph;
    DWORD dwData;
};

struct DIDEVICEINSTANCEA
{
    DWORD dwSize;
    GUID guidInstance;
    GUID guidProduct;
    DWORD dwDevType;
    CHAR tszInstanceName[MAX_PATH];
    CHAR tszProductName[MAX_PATH];
    GUID guidFFDriver;
    WORD wUsagePage;
    WORD wUsage;
};
typedef DIDEVICEINSTANCEA* LPDIDEVICEINSTANCEA;
typedef const DIDEVICEINSTANCEA* LPCDIDEVICEINSTANCEA;

typedef BOOL(*LPDIENUMDEVICESCALLBACKA)(LPCDIDEVICEINSTANCEA lpddi, LPVOID pvRef);

/* ---- Interfaces: stubs de compilacion ---- */
struct IDirectInputDeviceA
{
    inline HRESULT Release() { return S_OK; }
    inline HRESULT Unacquire() { return S_OK; }
    inline HRESULT Acquire() { return S_OK; }
    inline HRESULT QueryInterface(const GUID& riid, void** ppvObj) { return S_OK; }
    inline HRESULT GetDeviceState(DWORD cbData, LPVOID lpvData)
    {
        if (lpvData && cbData)
        {
            unsigned char* p = (unsigned char*)lpvData;
            if (cbData == 256)
            {
                G2_InputGetState256(p);
                return S_OK;
            }
            memset(p, 0, cbData);
        }
        return S_OK;
    }
    inline HRESULT GetDeviceData(DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, void* pdwInOut, DWORD dwFlags)
    {
        (void)cbObjectData;
        if (pdwInOut)
        {
            unsigned int cap = *(unsigned int*)pdwInOut;
            *(unsigned int*)pdwInOut = (unsigned int)G2_InputGetEvents(rgdod, cap, ((dwFlags & 1) != 0) ? 1 : 0);
        }
        return S_OK;
    }
    inline HRESULT SetDataFormat(const DIDATAFORMAT* lpdf) { return S_OK; }
    inline HRESULT SetCooperativeLevel(HWND hwnd, DWORD dwFlags) { return S_OK; }
    inline HRESULT SetProperty(const GUID& rguidProp, const DIPROPHEADER* pdiph) { return S_OK; }
    inline HRESULT GetProperty(const GUID& rguidProp, DIPROPHEADER* pdiph) { return S_OK; }
};
typedef IDirectInputDeviceA* LPDIRECTINPUTDEVICEA;

IDirectInputDeviceA* GetFakeInputDevice();

struct IDirectInputA
{
    inline HRESULT Release() { return S_OK; }
    inline HRESULT CreateDevice(const GUID& rguid, LPDIRECTINPUTDEVICEA* lplpDirectInputDevice, void* pUnkOuter)
    {
        if (lplpDirectInputDevice)
        {
            *lplpDirectInputDevice = GetFakeInputDevice();
        }
        return S_OK;
    }
    inline HRESULT EnumDevices(DWORD dwDevType, LPDIENUMDEVICESCALLBACKA lpCallback, LPVOID pvRef, DWORD dwFlags) { return S_OK; }
    inline HRESULT QueryInterface(const GUID& riid, void** ppvObj) { return S_OK; }
};
typedef IDirectInputA* LPDIRECTINPUTA;

IDirectInputA* GetFakeDirectInput();

#ifdef __cplusplus
extern "C" {
#endif
HRESULT DirectInputCreateA(void* hinst, DWORD dwVersion, LPDIRECTINPUTA* lplpDirectInput, void* pUnkOuter);
HRESULT DirectInput8Create(void* hinst, DWORD dwVersion, const GUID& rguid, void** ppvOut, void* pUnkOuter);
#ifdef __cplusplus
}
#endif