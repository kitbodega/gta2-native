#pragma once
// Stub minimal de DirectPlay Lobby (DPLOBBY.H) para el port nativo Android.
#include <wintypes.hpp>
#include "DPLAY.H"

typedef struct IDirectPlayLobby IDirectPlayLobby;
typedef IDirectPlayLobby FAR *LPDIRECTPLAYLOBBY;
typedef struct IDirectPlayLobby2 IDirectPlayLobby2;
typedef IDirectPlayLobby2 FAR *LPDIRECTPLAYLOBBY2;

#define DPLCLOSE 1
#define DPLCHAR 2

#define DPLAPI __stdcall

#define CLSID_DirectPlayLobby { 0x2fe8ea50, 0xb2cc, 0x11d0, {0x83, 0x99, 0x0, 0xa0, 0xc9, 0xa, 0x43, 0xcb} }
#define IID_IDirectPlayLobby { 0xaf1f5300, 0x54b1, 0x11d1, {0x84, 0x7b, 0x0, 0xa0, 0xc9, 0x6e, 0x49, 0x71} }
#define IID_IDirectPlayLobby2 { 0x1dc4e5a0, 0x4a8f, 0x11d1, {0x88, 0x9, 0x0, 0xa0, 0xc9, 0x6e, 0x49, 0x81} }