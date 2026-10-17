#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GlesVertex
{
    float x, y;
    float s, t;
    float w;
} GlesVertex;

void* gles_create(void* anw);

void gles_destroy(void* ctx);

int gles_get_size(void* ctx, int* out_w, int* out_h);

void gles_set_clear_color(void* ctx, float r, float g, float b);

void gles_begin_frame(void* ctx);
int gles_frame_seq();

void gles_clear_game_rect(void* ctx, int left, int top, int right, int bottom);

unsigned gles_upload_texture(void* ctx, int w, int h, const void* rgba);

int gles_upload_texture_into(void* ctx, unsigned tex, int w, int h, const void* rgba);

void gles_delete_texture(void* ctx, unsigned tex);

void gles_draw_poly(void* ctx, unsigned tex, const GlesVertex* v, int nverts);

void gles_draw_poly_shadow(void* ctx, unsigned tex, const GlesVertex* v, int nverts);

void gles_end_frame(void* ctx);

#ifdef __cplusplus
}
#endif