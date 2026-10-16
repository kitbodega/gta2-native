#pragma once

#ifndef _TCHAR_DEFINED
#define _TCHAR_DEFINED
#ifdef UNICODE
typedef wchar_t _TCHAR;
#else
typedef char _TCHAR;
#endif
#endif

#ifndef _T
#ifdef UNICODE
#define _T(x) L##x
#define _TEXT(x) L##x
#else
#define _T(x) x
#define _TEXT(x) x
#endif
#endif

#ifndef TCHAR
#ifdef UNICODE
#define TCHAR wchar_t
#else
#define TCHAR char
#endif
#endif

#ifndef _tcslen
#ifdef UNICODE
#define _tcslen wcslen
#define _tcsncpy wcsncpy
#define _tcscmp wcscmp
#define _tcscpy wcscpy
#define _tcsstr wcsstr
#define _tcsncmp wcsncmp
#define _tprintf wprintf
#define _stprintf swprintf
#define _sntprintf _snwprintf
#define _tcsrchr wcsrchr
#define _tcschr wcschr
#else
#define _tcslen strlen
#define _tcsncpy strncpy
#define _tcscmp strcmp
#define _tcscpy strcpy
#define _tcsstr strstr
#define _tcsncmp strncmp
#define _tprintf printf
#define _stprintf sprintf
#define _sntprintf snprintf
#define _tcsrchr strrchr
#define _tcschr strchr
#endif
#endif