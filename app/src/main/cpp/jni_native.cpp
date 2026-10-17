#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <pthread.h>
#include <atomic>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "wintypes.hpp"
#include "winmain.hpp"

void G2Platform_SetGameDataDir(const char* dir);
void G2Platform_SetSurface(void* anw);
extern "C" void G2_InputKey(int scan, int down);
extern "C" bool G2Platform_InVehicle(void);

#define LOGT "GTA2-ndk"

static std::atomic<bool> g_running{false};
static std::atomic<bool> g_stop{false};

static void* engineThread(void*)
{
    __android_log_print(ANDROID_LOG_INFO, LOGT, "engine thread start (WinMain_5E53F0)");
    g_running = true;
    int rc = WinMain_5E53F0((HINSTANCE)1, 0, (LPSTR)"", 1);
    __android_log_print(ANDROID_LOG_INFO, LOGT, "engine thread exit rc=%d", rc);
    g_running = false;
    return 0;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_bodega_gta2_MainActivity_nativeLoadGame(JNIEnv* env, jobject, jstring path)
{
    const char* p = env->GetStringUTFChars(path, 0);
    __android_log_print(ANDROID_LOG_INFO, LOGT, "nativeLoadGame: %s", p ? p : "(null)");
    if (p)
    {
        char dir[1024];
        snprintf(dir, sizeof(dir), "%s", p);
        char* slash = strrchr(dir, '/');
        if (slash)
        {
            *slash = 0;
            G2Platform_SetGameDataDir(dir);
            if (chdir(dir) == 0)
            {
                __android_log_print(ANDROID_LOG_INFO, LOGT, "chdir OK -> %s", dir);
                env->ReleaseStringUTFChars(path, p);
                return JNI_TRUE;
            }
            __android_log_print(ANDROID_LOG_WARN, LOGT, "chdir failed: %s", dir);
        }
        env->ReleaseStringUTFChars(path, p);
    }
    return JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_bodega_gta2_MainActivity_nativeStart(JNIEnv*, jobject)
{
    bool expected = false;
    if (!g_running.compare_exchange_strong(expected, true)) return JNI_TRUE;
    g_stop = false;
    pthread_t tid;
    if (pthread_create(&tid, 0, &engineThread, 0) != 0)
    {
        g_running = false;
        __android_log_print(ANDROID_LOG_ERROR, LOGT, "pthread_create FAILED");
        return JNI_FALSE;
    }
    pthread_detach(tid);
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_bodega_gta2_MainActivity_nativeStop(JNIEnv*, jobject)
{
    __android_log_print(ANDROID_LOG_INFO, LOGT, "stop requested");
    g_stop = true;
}

extern "C" JNIEXPORT void JNICALL
Java_com_bodega_gta2_MainActivity_nativeSetSurface(JNIEnv* env, jobject, jobject surface)
{
    if (surface)
    {
        ANativeWindow* win = ANativeWindow_fromSurface(env, surface);
        __android_log_print(ANDROID_LOG_INFO, LOGT, "nativeSetSurface: %p", (void*)win);
        G2Platform_SetSurface(win);
    }
    else
    {
        __android_log_print(ANDROID_LOG_INFO, LOGT, "nativeSetSurface: null (surface destroy)");
        G2Platform_SetSurface(0);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_bodega_gta2_MainActivity_nativeKey(JNIEnv*, jobject, jint scan, jint down)
{
    G2_InputKey((int)scan, (int)down);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_bodega_gta2_MainActivity_nativeInVehicle(JNIEnv*, jobject)
{
    return G2Platform_InVehicle() ? JNI_TRUE : JNI_FALSE;
}