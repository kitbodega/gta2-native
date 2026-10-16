#include <android/log.h>
#include <dlfcn.h>
#include <cstring>
#include <cstdint>
#include <cstdio>

extern "C" char* __real_strcpy(char* dest, const char* src);

char gLastSrcBuf[64];
void* gLastCaller = 0;
size_t gLastLen = 0;
size_t gLastSiz = 0;
bool gLastOverflow = false;

extern "C" char* __wrap___strcpy_chk(char* dest, const char* src, size_t siz)
{
    size_t srcLen = ::strlen(src);
    bool overflow = srcLen >= siz;

    void* caller = __builtin_return_address(0);
    Dl_info info;
    const char* fname = "?";
    if (dladdr(caller, &info) && info.dli_fbase && info.dli_sname)
    {
        fname = info.dli_sname;
    }
    char srcBuf[64];
    strncpy(srcBuf, src, 63);
    srcBuf[63] = 0;

    if (overflow || siz <= 8)
    {
        __android_log_print(
            ANDROID_LOG_ERROR,
            "GTA2-port",
            "STRCPYCHK %s len=%zu size=%zu dest=%p src='%s' caller=%p %s",
            overflow ? "OVERFLOW" : "small",
            srcLen,
            siz,
            (void*)dest,
            srcBuf,
            caller,
            fname);
    }

    if (overflow)
    {
        gLastOverflow = true;
        gLastLen = srcLen;
        gLastSiz = siz;
        gLastCaller = caller;
        memcpy(gLastSrcBuf, srcBuf, 64);
    }

    return __real_strcpy(dest, src);
}

extern "C" char* __wrap_strcpy(char* dest, const char* src)
{
    return __real_strcpy(dest, src);
}