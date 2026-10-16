#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Vertice de la api GLES del port: posicion en coordenadas de juego
// (640x480 logico) + UV con perspectiva pre-horneada.
// La UV se pasa como (s, t, w) siendo uv_final = (s/w, t/w): el shader
// interpola el numerador y el denominador (lineales) y divide por w, con lo
// que se reproduce en el GPU la homografia g*(x,y)+1 del rasterizador CPU.
typedef struct GlesVertex
{
    float x, y; // coordenadas de juego (pixels de pantalla logica)
    float s, t; // (a*x+b*y+c)*invW+0.5*invW  y  (d*x+e*y+f)*invH+0.5*invH
    float w;    // denominador de perspectiva en el vertice (1.0 = affine)
} GlesVertex;

// Crea el contexto EGL sobre la ventana (estricamente en el hilo que
// dibuja/presenta). Devuelve un puntero opaco (o 0 si falla).
void* gles_create(void* anw);

void gles_destroy(void* ctx);

// Tamano de la superficie EGL en pixels.
int gles_get_size(void* ctx, int* out_w, int* out_h);

// Color de borrado (se aplica en gles_begin_frame y en clear_game_rect).
void gles_set_clear_color(void* ctx, float r, float g, float b);

// Comienzo de scene: viewport completo, borra toda la ventana (incluye el
// letterbox), mapeo logica 640x480 -> NDC con aspect 4:3 centrado.
void gles_begin_frame(void* ctx);
int gles_frame_seq();

// Borra un rect en coordenadas de juego (scissor).
void gles_clear_game_rect(void* ctx, int left, int top, int right, int bottom);

// Sube una textura RGBA8 (w*h*4 tight rows). Devuelve el id GL (0=error).
unsigned gles_upload_texture(void* ctx, int w, int h, const void* rgba);

// Sube datos a un glid EXISTENTE (mismo id, sin delete+gen). Devuelve 1/0.
int gles_upload_texture_into(void* ctx, unsigned tex, int w, int h, const void* rgba);

// Borrado diferido: la eliminacion real ocurre en gles_end_frame tras el swap.
void gles_delete_texture(void* ctx, unsigned tex);

// Dibuja un poligono texturizado: nverts=3 (triangulo) o 4 (quad, 2 tris).
// Las UV son pixel-center-biased y ya normalizadas por el tamano de textura.
void gles_draw_poly(void* ctx, unsigned tex, const GlesVertex* v, int nverts);

// Igual que gles_draw_poly pero en modo "sombra": silueta negra con alpha
// 0xA0 (~63%) sobre el contenido ya dibujado (blend SRC_ALPHA 1-SRC_ALPHA).
// Aisla el blend y el tinte: flush propio antes y despues, uTint=(0,0,0,0.63).
void gles_draw_poly_shadow(void* ctx, unsigned tex, const GlesVertex* v, int nverts);

// Flush del lote y swap de buffers.
void gles_end_frame(void* ctx);

#ifdef __cplusplus
}
#endif