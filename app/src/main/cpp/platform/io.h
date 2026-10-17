#pragma once

#include <stdlib.h>

struct _finddata_t
{
    unsigned int attrib;
    long time_create;
    long time_access;
    long time_write;
    unsigned long size;
    char name[260];
};

#define _A_NORMAL 0x00
#define _A_RDONLY 0x01
#define _A_HIDDEN 0x02
#define _A_SYSTEM 0x04
#define _A_SUBDIR 0x10
#define _A_ARCH 0x20

#ifdef __cplusplus
extern "C" {
#endif

long _findfirst(const char* pattern, struct _finddata_t* fd);
int _findnext(long handle, struct _finddata_t* fd);
int _findclose(long handle);

char* _itoa(int value, char* buffer, int radix);
wchar_t* _itow(int value, wchar_t* buffer, int radix);

#ifdef __cplusplus
}
#endif