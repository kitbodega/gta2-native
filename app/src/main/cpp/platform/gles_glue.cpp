

#include "gles_glue.hpp"

#include <android/log.h>
#include <android/native_window.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <string.h>
#include <stdlib.h>
#include <vector>

#define LOGT "GTA2-gles"

#define GLES_MAX_VERTS 6144

static std::vector<GLuint> gles_delq;

struct GlesCtx
{
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int winW;
    int winH;

    unsigned program;
    GLint uViewLoc;
    GLint uTexLoc;
    GLint uTintLoc;
    GLint aPosLoc;
    GLint aSTWLoc;
    GLuint vbo;

    float clearR;
    float clearG;
    float clearB;


    struct BVtx
    {
        float x, y, s, t, w;
        unsigned tex;
    } verts[GLES_MAX_VERTS];
    int vertCount;
    unsigned curTex;
};

static void gles_compute_view(int winW, int winH, float* ux, float* oz, float* uy, float* ow)
{
    const float fit = (winW / 640.0f < winH / 480.0f) ? (winW / 640.0f) : (winH / 480.0f);
    const float dw = 640.0f * fit;
    const float dh = 480.0f * fit;
    const float ox = (winW - dw) * 0.5f;
    const float oy = (winH - dh) * 0.5f;
    *ux = 2.0f * fit / (float)winW;
    *uy = -2.0f * fit / (float)winH;
    *oz = 2.0f * ox / (float)winW - 1.0f;
    *ow = -(2.0f * oy / (float)winH - 1.0f);
}

static unsigned gles_compile_shader(GLenum type, const char* src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, 0);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[512];
        GLint len = 0;
        glGetShaderInfoLog(sh, sizeof(log) - 1, &len, log);
        log[len] = 0;
        __android_log_print(ANDROID_LOG_ERROR, LOGT, "shader compile: %s", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static const char* gVS =
    "attribute vec2 aPos;\n"
    "attribute vec3 aSTW;\n"
    "uniform vec4 uView;\n"
    "varying vec3 vSTW;\n"
    "void main(){\n"
    "  vSTW = vec3(aSTW.x * aSTW.z, aSTW.y * aSTW.z, aSTW.z);\n"
    "  gl_Position = vec4(aPos.x*uView.x + uView.z, aPos.y*uView.y + uView.w, 0.0, 1.0);\n"
    "}\n";

static const char* gFS =
    "precision mediump float;\n"
    "varying vec3 vSTW;\n"
    "uniform sampler2D uTex;\n"
    "uniform vec4 uTint;\n"
    "void main(){\n"
    "  vec2 uv = vSTW.xy / vSTW.z;\n"
    "  vec4 c = texture2D(uTex, uv);\n"
    "  if (c.a < 0.5) discard;\n"
    "  gl_FragColor = vec4(c.rgb * uTint.rgb, c.a * uTint.a);\n"
    "}\n";

void* gles_create(void* anw)
{
    __android_log_print(ANDROID_LOG_INFO, LOGT, "gles_create: ventana ANativeWindow");

    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY) return 0;
    if (!eglInitialize(dpy, 0, 0)) return 0;

    const EGLint cfgAttrs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_NONE
    };
    EGLConfig cfg;
    EGLint ncfg = 0;
    if (!eglChooseConfig(dpy, cfgAttrs, &cfg, 1, &ncfg) || ncfg < 1)
    {
        __android_log_print(ANDROID_LOG_ERROR, LOGT, "gles_create: sin config EGL");
        eglTerminate(dpy);
        return 0;
    }

    EGLSurface sur = eglCreateWindowSurface(dpy, cfg, (EGLNativeWindowType)anw, 0);
    if (sur == EGL_NO_SURFACE)
    {
        __android_log_print(ANDROID_LOG_ERROR, LOGT, "gles_create: eglCreateWindowSurface %d", (int)eglGetError());
        eglTerminate(dpy);
        return 0;
    }

    const EGLint ctxAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxAttrs);
    if (ctx == EGL_NO_CONTEXT)
    {
        __android_log_print(ANDROID_LOG_ERROR, LOGT, "gles_create: eglCreateContext %d", (int)eglGetError());
        eglDestroySurface(dpy, sur);
        eglTerminate(dpy);
        return 0;
    }

    if (!eglMakeCurrent(dpy, sur, sur, ctx))
    {
        __android_log_print(ANDROID_LOG_ERROR, LOGT, "gles_create: eglMakeCurrent %d", (int)eglGetError());
        eglDestroyContext(dpy, ctx);
        eglDestroySurface(dpy, sur);
        eglTerminate(dpy);
        return 0;
    }
    eglSwapInterval(dpy, 1);

    GlesCtx* gc = (GlesCtx*)calloc(1, sizeof(GlesCtx));
    if (!gc)
    {
        eglDestroyContext(dpy, ctx);
        eglDestroySurface(dpy, sur);
        eglTerminate(dpy);
        return 0;
    }
    gc->display = dpy;
    gc->surface = sur;
    gc->context = ctx;
    gc->clearR = gc->clearG = gc->clearB = 0.0f;

    int w = 0, h = 0;
    eglQuerySurface(dpy, sur, EGL_WIDTH, &w);
    eglQuerySurface(dpy, sur, EGL_HEIGHT, &h);
    gc->winW = w > 0 ? w : 1;
    gc->winH = h > 0 ? h : 1;

    __android_log_print(ANDROID_LOG_INFO, LOGT, "gles_create: ventana %dx%d", gc->winW, gc->winH);

    GLuint vs = gles_compile_shader(GL_VERTEX_SHADER, gVS);
    GLuint fs = gles_compile_shader(GL_FRAGMENT_SHADER, gFS);
    if (!vs || !fs)
    {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        gles_destroy(gc);
        return 0;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "aPos");
    glBindAttribLocation(prog, 1, "aSTW");
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[512];
        GLint len = 0;
        glGetProgramInfoLog(prog, sizeof(log) - 1, &len, log);
        log[len] = 0;
        __android_log_print(ANDROID_LOG_ERROR, LOGT, "program link: %s", log);
        glDeleteProgram(prog);
        gles_destroy(gc);
        return 0;
    }

    gc->program = prog;
    gc->aPosLoc = 0;
    gc->aSTWLoc = 1;
    gc->uViewLoc = glGetUniformLocation(prog, "uView");
    gc->uTexLoc = glGetUniformLocation(prog, "uTex");
    gc->uTintLoc = glGetUniformLocation(prog, "uTint");

    glGenBuffers(1, &gc->vbo);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glClearColor(gc->clearR, gc->clearG, gc->clearB, 1.0f);

    return gc;
}

void gles_destroy(void* ctx)
{
    if (!ctx) return;
    gles_delq.clear();
    GlesCtx* gc = (GlesCtx*)ctx;
    if (gc->program) glDeleteProgram(gc->program);
    if (gc->vbo) glDeleteBuffers(1, &gc->vbo);
    if (gc->display != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(gc->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (gc->context != EGL_NO_CONTEXT) eglDestroyContext(gc->display, gc->context);
        if (gc->surface != EGL_NO_SURFACE) eglDestroySurface(gc->display, gc->surface);
        eglTerminate(gc->display);
    }
    free(gc);
}

int gles_get_size(void* ctx, int* out_w, int* out_h)
{
    if (!ctx) return 0;
    GlesCtx* gc = (GlesCtx*)ctx;
    int w = 0, h = 0;
    eglQuerySurface(gc->display, gc->surface, EGL_WIDTH, &w);
    eglQuerySurface(gc->display, gc->surface, EGL_HEIGHT, &h);
    if (w > 0) gc->winW = w;
    if (h > 0) gc->winH = h;
    if (out_w) *out_w = gc->winW;
    if (out_h) *out_h = gc->winH;
    return 1;
}

void gles_set_clear_color(void* ctx, float r, float g, float b)
{
    if (!ctx) return;
    GlesCtx* gc = (GlesCtx*)ctx;
    gc->clearR = r;
    gc->clearG = g;
    gc->clearB = b;
}

static volatile int s_frameSeq = 0;
int gles_frame_seq() { return s_frameSeq; }

void gles_begin_frame(void* ctx)
{
    s_frameSeq++;
    if (!ctx) return;
    GlesCtx* gc = (GlesCtx*)ctx;

    int w = 0, h = 0;
    eglQuerySurface(gc->display, gc->surface, EGL_WIDTH, &w);
    eglQuerySurface(gc->display, gc->surface, EGL_HEIGHT, &h);
    if (w > 0) gc->winW = w;
    if (h > 0) gc->winH = h;

    glViewport(0, 0, gc->winW, gc->winH);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(gc->clearR, gc->clearG, gc->clearB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    float ux, uz, uy, uw;
    gles_compute_view(gc->winW, gc->winH, &ux, &uz, &uy, &uw);
    glUseProgram(gc->program);
    glUniform4f(gc->uViewLoc, ux, uy, uz, uw);
    glUniform1i(gc->uTexLoc, 0);
    glUniform4f(gc->uTintLoc, 1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_BLEND);

    gc->vertCount = 0;
    gc->curTex = 0;
}

void gles_clear_game_rect(void* ctx, int left, int top, int right, int bottom)
{
    if (!ctx) return;
    GlesCtx* gc = (GlesCtx*)ctx;



    const float fit = (gc->winW / 640.0f < gc->winH / 480.0f) ? (gc->winW / 640.0f) : (gc->winH / 480.0f);
    const float dw = 640.0f * fit;
    const float dh = 480.0f * fit;
    const float ox = (gc->winW - dw) * 0.5f;
    const float oy = (gc->winH - dh) * 0.5f;

    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > 640) right = 640;
    if (bottom > 480) bottom = 480;
    if (right <= left || bottom <= top) return;

    const int gx0 = (int)(ox + left * fit);
    const int gx1 = (int)(ox + right * fit);
    const int gy0 = (int)(oy + top * fit);
    const int gy1 = (int)(oy + bottom * fit);

    glEnable(GL_SCISSOR_TEST);
    glScissor(gx0, gc->winH - gy1, gx1 - gx0, gy1 - gy0);
    glClearColor(gc->clearR, gc->clearG, gc->clearB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

unsigned gles_upload_texture(void* ctx, int w, int h, const void* rgba)
{
    if (!ctx || !rgba || w <= 0 || h <= 0) return 0;
    GLuint tex = 0;
    glGenTextures(1, &tex);
    if (!tex) return 0;
    if (!gles_upload_texture_into(ctx, tex, w, h, rgba))
    {
        glDeleteTextures(1, &tex);
        return 0;
    }
    return tex;
}

int gles_upload_texture_into(void* ctx, unsigned tex, int w, int h, const void* rgba)
{
    if (!ctx || !tex || !rgba || w <= 0 || h <= 0) return 0;
    glGetError();
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);



    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        __android_log_print(ANDROID_LOG_ERROR, LOGT, "TEXUPLOAD ERR %d (w=%d h=%d tex=%u)", (int)err, w, h, tex);
    glBindTexture(GL_TEXTURE_2D, 0);
    return 1;
}

void gles_delete_texture(void* ctx, unsigned tex)
{
    if (!ctx || !tex) return;
    gles_delq.push_back(tex);
}

static void gles_flush(GlesCtx* gc)
{
    if (gc->vertCount == 0) return;

    glUseProgram(gc->program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gc->curTex);
    glUniform1i(gc->uTexLoc, 0);

    glBindBuffer(GL_ARRAY_BUFFER, gc->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)gc->vertCount * sizeof(GlesCtx::BVtx), gc->verts, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(gc->aPosLoc);
    glVertexAttribPointer(gc->aPosLoc, 2, GL_FLOAT, GL_FALSE, sizeof(GlesCtx::BVtx), (const void*)0);
    glEnableVertexAttribArray(gc->aSTWLoc);
    glVertexAttribPointer(gc->aSTWLoc, 3, GL_FLOAT, GL_FALSE, sizeof(GlesCtx::BVtx), (const void*)8);

    glDrawArrays(GL_TRIANGLES, 0, gc->vertCount);
    gc->vertCount = 0;
}

static void gles_push(GlesCtx* gc, unsigned tex, const GlesVertex* v, int nverts)
{
    if (gc->curTex != tex && gc->vertCount > 0)
    {
        gles_flush(gc);
    }
    gc->curTex = tex;

    if (nverts == 4)
    {
        if (gc->vertCount + 6 > GLES_MAX_VERTS) gles_flush(gc);
        gc->curTex = tex;
        const int tri[6] = { 0, 1, 2, 0, 2, 3 };
        for (int k = 0; k < 6; k++)
        {
            const GlesVertex* p = &v[tri[k]];
            GlesCtx::BVtx& b = gc->verts[gc->vertCount++];
            b.x = p->x; b.y = p->y; b.s = p->s; b.t = p->t; b.w = p->w; b.tex = tex;
        }
    }
    else if (nverts == 3)
    {
        if (gc->vertCount + 3 > GLES_MAX_VERTS) gles_flush(gc);
        gc->curTex = tex;
        for (int k = 0; k < 3; k++)
        {
            const GlesVertex* p = &v[k];
            GlesCtx::BVtx& b = gc->verts[gc->vertCount++];
            b.x = p->x; b.y = p->y; b.s = p->s; b.t = p->t; b.w = p->w; b.tex = tex;
        }
    }
}

void gles_draw_poly(void* ctx, unsigned tex, const GlesVertex* v, int nverts)
{
    if (!ctx || !v || !tex || (nverts != 3 && nverts != 4)) return;
    gles_push((GlesCtx*)ctx, tex, v, nverts);
}

void gles_draw_poly_shadow(void* ctx, unsigned tex, const GlesVertex* v, int nverts)
{
    if (!ctx || !v || !tex || (nverts != 3 && nverts != 4)) return;
    GlesCtx* gc = (GlesCtx*)ctx;




    gles_flush(gc);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUniform4f(gc->uTintLoc, 0.0f, 0.0f, 0.0f, 160.0f / 255.0f);
    gles_push(gc, tex, v, nverts);
    gles_flush(gc);
    glUniform4f(gc->uTintLoc, 1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_BLEND);
}

void gles_end_frame(void* ctx)
{
    if (!ctx) return;
    GlesCtx* gc = (GlesCtx*)ctx;
    gles_flush(gc);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        static int sErrN = 0;
        if (sErrN++ < 60)
            __android_log_print(ANDROID_LOG_ERROR, LOGT, "FRAME ERR %d", (int)err);
    }
    if (!eglSwapBuffers(gc->display, gc->surface))
    {
        static int sSwapErr = 0;
        if (sSwapErr++ < 10)
            __android_log_print(ANDROID_LOG_ERROR, LOGT, "SWAP ERR %d", (int)eglGetError());
    }

    if (!gles_delq.empty())
    {
        glDeleteTextures((GLsizei)gles_delq.size(), gles_delq.data());
        gles_delq.clear();
    }
}