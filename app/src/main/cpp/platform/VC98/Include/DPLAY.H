#pragma once
// Stub minimal de DirectPlay (DPLAY.H) para el port nativo Android.
// Solo define los tipos que NetPlay.hpp necesita compilar; las funciones
// DirectPlay reales no se enlazan (NetPlay.cpp esta excluido del tranche 1).

#include <wintypes.hpp>

typedef struct _DPNAME
{
    DWORD dwSize;
    DWORD dwFlags;
    union { LPCSTR lpszShortNameA; LPWSTR lpszShortNameW; };
    union { LPCSTR lpszLongNameA; LPWSTR lpszLongNameW; };
} DPNAME;
typedef DPNAME FAR *LPDFNAME;

typedef struct _DPSESSIONDESC2
{
    DWORD dwSize;
    DWORD dwFlags;
    GUID guidInstance;
    GUID guidApplication;
    DWORD dwMaxPlayers;
    DWORD dwCurrentPlayers;
    union { LPCSTR lpszSessionNameA; LPWSTR lpszSessionNameW; };
    union { LPCSTR lpszPasswordA; LPWSTR lpszPasswordW; };
    DWORD dwReserved1;
    DWORD dwReserved2;
    DWORD dwUser1;
    DWORD dwUser2;
    DWORD dwUser3;
    DWORD dwUser4;
} DPSESSIONDESC2;
typedef DPSESSIONDESC2 FAR *LPDPSESSIONDESC2;

typedef struct IDirectPlay3 IDirectPlay3;
typedef IDirectPlay3 FAR *LPDIRECTPLAY3;
typedef struct IDirectPlay4 IDirectPlay4;
typedef IDirectPlay4 FAR *LPDIRECTPLAY4;

#define DPENUMSESSIONS_ALL 0x00000001L
#define DPENUMSESSIONS_STOP 0x00000002L
#define DPENUMSESSIONS_PREVIEW 0x00000004L
#define DPENUMSESSIONS_AVAILABLE 0x00000008L
#define DPENUMSESSIONS_SYNC 0x00000010L
#define DPENUMSESSIONS_ASYNC 0x00000020L

#define DPESC_TIMEOUT  1
#define DPESC_ALL      2

#define DPCONNECTIONDIRECTPLAY 0x00000001L
#define DPCONNECTIONDIRECTPLAYLOBBY 0x00000002L