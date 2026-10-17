#pragma once

#include "wintypes.hpp"
#include "windows.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t   S32;
typedef uint32_t  U32;
typedef int16_t   S16;
typedef uint16_t  U16;
typedef int8_t    S8;
typedef uint8_t   U8;
typedef float     F32;

typedef void* HDIGDRIVER;
typedef void* HSAMPLE;
typedef void* HSTREAM;
typedef U32   HPROVIDER;
typedef U32   HPROENUM;
typedef struct h3DPOBJECT { U32 junk; } h3DPOBJECT;
typedef h3DPOBJECT* H3DPOBJECT;
typedef H3DPOBJECT H3DSAMPLE;

typedef S32 M3DRESULT;
#define M3D_NOERR      0
#define M3D_ERR_NODRIVER 2

#define WAVE_FORMAT_PCM 1

#define SMP_FREE  0x0001
#define SMP_DONE  0x0002
#define SMP_PLAYING 0x0004
#define SMP_STOPPED 0x0008

typedef struct tagWAVEFORMAT {
    U16 wFormatTag;
    U16 nChannels;
    U32 nSamplesPerSec;
    U32 nAvgBytesPerSec;
    U16 nBlockAlign;
    U16 wBitsPerSample;
} WAVEFORMAT;

typedef struct _AILSOUNDINFO {
  S32  format;
  void* data_ptr;
  U32  data_len;
  U32  rate;
  S32  bits;
  S32  channels;
  U32  samples;
  U32  block_size;
  void* initial_ptr;
} AILSOUNDINFO;

M3DRESULT AIL_startup(void);
M3DRESULT AIL_shutdown(void);
void AIL_set_digital_master_volume(HDIGDRIVER drvr, S32 volume);
void AIL_set_preference(U32 number, S32 value);
S32 AIL_waveOutOpen(HDIGDRIVER* drvr, U32 device, S32 flags, WAVEFORMAT* wf);
void AIL_waveOutClose(HDIGDRIVER drvr);
void* AIL_mem_alloc_lock(U32 size);
void  AIL_mem_free_lock(void* mem);
S32  AIL_enumerate_3D_providers(HPROENUM* enumerator, HPROVIDER* prov, char** name);
U32  AIL_stream_status(HSTREAM stream);
HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER drvr);
void AIL_init_sample(HSAMPLE samp);
void AIL_set_sample_type(HSAMPLE samp, S32 format, S32 bits);
void AIL_release_sample_handle(HSAMPLE samp);
S32  AIL_stream_volume(HSTREAM stream);
void AIL_set_stream_volume(HSTREAM stream, S32 volume);
void AIL_delay(U32 ms);
void AIL_close_stream(HSTREAM stream);
void AIL_release_3D_sample_handle(H3DSAMPLE samp);
void AIL_set_sample_address(HSAMPLE samp, void* data, U32 len);
void AIL_set_sample_volume(HSAMPLE samp, S32 volume);
void AIL_set_sample_pan(HSAMPLE samp, S32 pan);
void AIL_set_sample_playback_rate(HSAMPLE samp, S32 rate);
void AIL_set_sample_loop_block(HSAMPLE samp, S32 loop_start, S32 loop_end);
void AIL_set_sample_loop_count(HSAMPLE samp, S32 count);
U32  AIL_sample_status(HSAMPLE samp);
void AIL_start_sample(HSAMPLE samp);
void AIL_end_sample(HSAMPLE samp);
U32  AIL_set_3D_sample_info(H3DSAMPLE samp, AILSOUNDINFO* info);
void AIL_set_3D_sample_volume(H3DSAMPLE samp, S32 volume);
void AIL_set_3D_position(H3DPOBJECT obj, F32 x, F32 y, F32 z);
void AIL_set_3D_sample_distances(H3DSAMPLE samp, F32 max_d, F32 min_d, F32 max_v, F32 min_v);
void AIL_set_3D_sample_playback_rate(H3DSAMPLE samp, S32 rate);
void AIL_set_3D_sample_loop_block(H3DSAMPLE samp, S32 loop_start, S32 loop_end);
void AIL_set_3D_sample_loop_count(H3DSAMPLE samp, S32 count);
U32  AIL_3D_sample_status(H3DSAMPLE samp);
void AIL_start_3D_sample(H3DSAMPLE samp);
void AIL_end_3D_sample(H3DSAMPLE samp);
S32  AIL_set_3D_provider_preference(HPROVIDER lib, const char* attribute, void* value);
void AIL_3D_provider_attribute(HPROVIDER lib, const char* attribute, void* value);
M3DRESULT AIL_open_3D_provider(HPROVIDER lib);
void AIL_close_3D_provider(HPROVIDER lib);
void AIL_set_stream_loop_count(HSTREAM stream, S32 count);
void AIL_start_stream(HSTREAM stream);
void AIL_set_stream_ms_position(HSTREAM stream, S32 ms_pos);
S32  AIL_stream_playback_rate(HSTREAM stream);
void AIL_stream_ms_position(HSTREAM stream, void* ms_pos, void* ms_len);
void AIL_set_stream_playback_rate(HSTREAM stream, S32 rate);
HSTREAM AIL_open_stream(HDIGDRIVER drvr, const char* filename, S32 memory_type);
S32  AIL_digital_handle_reacquire(HDIGDRIVER drvr);
void AIL_digital_handle_release(HDIGDRIVER drvr);
H3DSAMPLE AIL_allocate_3D_sample_handle(HPROVIDER lib);

#ifdef __cplusplus
}
#endif