/*
 *  rwkaudio.h
 *  rwk
 *
 *  Created on 12/05/2013.
 *
 */

#ifndef RWK_rwk_audio_internal_h
#define RWK_rwk_audio_internal_h

#include "g721.h"
#include "platform.h"
#include "pak.h"

#include <xmmintrin.h>
typedef union {
    __m128 mVec128;
    float v[4];
} rwkgraphics_vector;
typedef union {
    __m128 mVec128[4];
    float m[4][4];
} rwkgraphics_matrix;

#define NUM_BUF_SAMPLES 700
#define BUF_SIZE NUM_BUF_SAMPLES*2

extern struct rwkaudio_user_prefs {
    float sfx_vol;
    float music_vol;
} USER_AUDIO_PREFS;

struct rwkaudio_voiceplayer_context;

/* Callback type to fill buffer with PCM samples of decompressed audio */
typedef size_t(*rwkaudio_pcm_callback)(struct rwkaudio_voiceplayer_context* ctx,
                                       s16* target_buffer, size_t buffer_size);

/* Callback type to post-process audio and perform low-latency upmixing */
typedef void(*rwkaudio_tap_callback)(struct rwkaudio_voiceplayer_context* ctx,
unsigned channel_count, s16** channel_buffers,
unsigned sample_count);

struct AGSC_subclip;


/* Platform-specific voiceplayer structs */
#if __APPLE__
#include <AudioToolbox/AudioQueue.h>
typedef struct apple_aqs_voice {
    unsigned sample_rate;
    AudioQueueRef aq;
    AudioQueueBufferRef aq_bufs[3];
    rwkaudio_pcm_callback pcm_cb;
    struct rwkaudio_voiceplayer_context* voiceplayer;
    bool started;
} apple_aqs_voice;

#elif _WIN32
#if !RWKAUDIO_MICROSOFT_XAUDIO2
typedef struct XAUDIO2_BUFFER
{
    UINT32 Flags;                       // Either 0 or XAUDIO2_END_OF_STREAM.
    UINT32 AudioBytes;                  // Size of the audio data buffer in bytes.
    const BYTE* pAudioData;             // Pointer to the audio data buffer.
    UINT32 PlayBegin;                   // First sample in this buffer to be played.
    UINT32 PlayLength;                  // Length of the region to be played in samples,
    //  or 0 to play the whole buffer.
    UINT32 LoopBegin;                   // First sample of the region to be looped.
    UINT32 LoopLength;                  // Length of the desired loop region in samples,
    //  or 0 to loop the entire buffer.
    UINT32 LoopCount;                   // Number of times to repeat the loop region,
    //  or XAUDIO2_LOOP_INFINITE to loop forever.
    void* pContext;                     // Context value to be passed back in callbacks.
} XAUDIO2_BUFFER;
#endif
typedef struct {
    unsigned sample_rate;
#if RWKAUDIO_MICROSOFT_XAUDIO2
    class IXAudio2SourceVoice* voice;
    class rwkaudio_xaudio2_callback* voice_callback;
#else
    void* voice;
    void* voice_callback;
#endif
    XAUDIO2_BUFFER voice_bufs[3];
    rwkaudio_pcm_callback pcm_cb;
    struct rwkaudio_voiceplayer_context* voiceplayer;
} microsoft_xaudio2_voice;

#else
/* ALSA as fallback */
#include <alsa/asoundlib.h>
#include <samplerate.h>
typedef struct alsa_voice {
    unsigned sample_rate, target_sample_rate;
    float ref_pitch;
    float target_src_pitch;
    RWK_THREAD_T thread;
    RWK_MUTEX_T started_mutex;
    RWK_COND_T started_cond;
    snd_pcm_t* pcm;
    SRC_STATE* src;
    SRC_DATA src_d;
    s16* buffer;
    float* f_buffer_in;
    float* f_buffer_out;
    rwkaudio_pcm_callback pcm_cb;
    struct rwkaudio_voiceplayer_context* voiceplayer;
    bool started, needs_start, needs_sample_rate_reset, needs_src_reset, needs_destroy;
} alsa_voice;

#endif

struct AGSC_subclip {
    u16 myId;
    u16 padding1;
    u32 startFrameOffset;
    u32 padding2;
    u16 padding3;
    u16 sampleRate;
    u32 sampleCount;
    u32 loopStartSample;
    u32 loopLengthSamples;
    u32 offsetToCoeffs;
};

struct AGSC_link {
    u16 tbl1Key;
    void* tbl1Data;
    u16 tbl2Key;
    u16 clipKey;
};

struct AGSC_subclip_meta {
    s16 prev, prev_prev;
    s16 loop_prev, loop_prev_prev;
    s16 coefs[16];
};

/* AGSC structure */
typedef struct {
    
    u8* agsc_buf;
    
    unsigned link_count;
    struct AGSC_link* link_table;
    
    unsigned subclip_count;
    struct AGSC_subclip* subclip_table;
    
    u8* coefficient_base;
    u8* packet_data;

    const char* name;
    
} rwkaudio_agsc_context;



/* Voiceplayer context */
typedef struct rwkaudio_voiceplayer_context {

    int loop;
    bool oneshot_done;
    
    enum {
        NONE,
        AGSC,
        DSP,
        RSF,
        THP
    } type;
    
    union {
        struct {
            u16 clip_id;
            rwkaudio_agsc_context* an_agsc;
            bool in_interval;
            void* cue_data;
            struct AGSC_subclip* subclip;
            s32 prev, prev_prev;
            s32 loop_prev, loop_prev_prev;
            unsigned sample_count;
            unsigned packet_count;
            unsigned next_packet;
            unsigned next_sample_in_packet;
            unsigned loop_start_sample;
            unsigned loop_end_sample;
            s16* coefs;
            u8* compress_buf;
        } agsc;
        struct {
            FILE* l_file;
            FILE* r_file;
            s16 left_prev, left_prev_prev;
            s16 right_prev, right_prev_prev;
            s16 left_init_prev, left_init_prev_prev;
            s16 right_init_prev, right_init_prev_prev;
            s16 left_loop_prev, left_loop_prev_prev;
            s16 right_loop_prev, right_loop_prev_prev;
            unsigned packet_count;
            unsigned loop_off;
            unsigned next_packet;
            s16 left_coefs[16];
            s16 right_coefs[16];
            u8* compress_buf;
        } dsp;
        struct {
            FILE* l_file;
            FILE* r_file;
            struct g72x_state l_state;
            struct g72x_state r_state;
            bool loop_computed;
            struct g72x_state l_loop_state;
            struct g72x_state r_loop_state;
            unsigned sample_count;
            int loop_sample;
            unsigned next_sample;
            u8* compress_buf;
        } rsf;
#if RWK_PLAYER
        struct {
            rwkgraphics_thp_context* thp_ctx;
            unsigned next_frame;
            unsigned chan_count;
        } thp;
#endif
    };
    
    s16* left_decompress_buf;
    s16* right_decompress_buf;
    
    void(*stop_cb)(void* ctx);
    void* stop_ctx;
    
} rwkaudio_voiceplayer_context;

/* CSNG structure */
typedef struct {
    u8* wavetable_buf;
    rwkaudio_voiceplayer_context voices[64];
} rwkaudio_csng_context;

extern const unsigned RWKAUDIO_NUM_BUF_SAMPLES;
extern const unsigned RWKAUDIO_PACKETBUF_COUNT;
extern const unsigned RWKAUDIO_PACKETBUF_SIZE;


/* Start platform's audio system and set channel mapping info */
void rwkaudio_voiceplayer_prep();


/* Triggers sample-rate set and/or platform voice init */
void rwkaudio_init_agsc_voice(rwkaudio_voiceplayer_context* voice);


/* Prep Audio system */
void rwkaudio_prep();

/* Start here - Call from Kernel Thread! */
int rwkaudio_voiceplayer_new_agsc(rwkaudio_voiceplayer_context* ctx, int loop);
int rwkaudio_voiceplayer_new_dsp(rwkaudio_voiceplayer_context* ctx, const char* dsp_l_path,
                                 const char* dsp_r_path);
#if RWK_PLAYER
int rwkaudio_voiceplayer_new_thp(rwkaudio_voiceplayer_context* ctx, rwkgraphics_thp_context* thp_ctx);
#endif
int rwkaudio_voiceplayer_new_rsf(rwkaudio_voiceplayer_context* ctx, const char* rsf_path, int loop_sample);

/* Common destroy func */
void rwkaudio_voiceplayer_destroy(rwkaudio_voiceplayer_context* ctx);

/* Load selected AGSC files from `AudioGrp.pak` */
void rwkaudio_agscindex_load(rwkaudio_agsc_context* new_agsc, struct pak_entry* agsc_entry);
void rwkaudio_agscindex_unload(rwkaudio_agsc_context* an_agsc);
#if RWK_PLAYER
int rwkaudio_agscindex_setclip(rwkaudio_voiceplayer_context* voice,
                               PyObject* agsc_set,
                               u16 clip_id);
#elif RWK_TOOLCHAIN
void rwkaudio_agscindex_setclip(rwkaudio_voiceplayer_context* voice,
                                rwkaudio_agsc_context* an_agsc,
                                u16 clip_id);
#endif

/* CSNG files */
int rwkaudio_csng_load(rwkaudio_csng_context* ctx, struct pak_entry* csng_entry, int idx);
void rwkaudio_csng_unload(rwkaudio_csng_context* ctx);


/* Offline render to wave file */
int rwkaudio_render_rsf(const char* rsf_path, const char* wave_path);


#endif
