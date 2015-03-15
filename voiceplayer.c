/*
 *  rwkaudio_voiceplayer.c
 *  rwk
 *
 *  Created on 12/05/2013.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "rwkaudio.h"
#include "logging.h"
#include "platform.h"

/* Buffer size constants */
const unsigned RWKAUDIO_NUM_BUF_SAMPLES = NUM_BUF_SAMPLES;
const unsigned RWKAUDIO_PACKETBUF_COUNT = (NUM_BUF_SAMPLES/14);
const unsigned RWKAUDIO_PACKETBUF_SIZE = (NUM_BUF_SAMPLES/14)*8;


#pragma mark Surround Upmixers

/* Common upmixing routines */
static unsigned audio_upmix_mono(rwkaudio_voiceplayer_context* ctx,
                                 s16* target_buffer, s16* buffer,
                                 unsigned sample_count, float master_gain) {
    unsigned i;
    
    for (i=0 ; i<sample_count ; ++i) {
        *target_buffer++ = *buffer;
        buffer++;
    }
    
    return sample_count;
    
}
static void audio_upmix_stereo(rwkaudio_voiceplayer_context* ctx,
                               s16* target_buffer, s16* left_buffer, s16* right_buffer,
                               unsigned sample_count, float master_gain) {
    unsigned i;
    
    /* Surround Matrix */
    for (i=0 ; i<sample_count ; ++i) {
        *target_buffer++ = *left_buffer;
        *target_buffer++ = *right_buffer;
        left_buffer++;
        right_buffer++;
    }
    
}



#pragma mark ADPCM Decoding

/* Reference: http://sourceforge.net/p/vgmstream/code/HEAD/tree/src/coding/ngc_dsp_decoder.c */

/* Signed nibbles come up a lot */
static const s32 nibble_to_int[16] = {0,1,2,3,4,5,6,7,-8,-7,-6,-5,-4,-3,-2,-1};

static INLINE s16 samp_clamp(s32 val) {
    if (val < -32768) val = -32768;
    if (val > 32767) val = 32767;
    return val;
}

#define adpcm_decompress_packet(exponent, factor1, factor2, prev, \
prev_prev, samp_idx, samp_limit, buf_in, buf_out, fill_samp, max_samp, mult) \
unsigned samp;\
for (samp=samp_idx ; samp<samp_limit && fill_samp<max_samp ; ++samp) {\
    s32 sample_data = (samp&1)?\
    nibble_to_int[(*(u8*)(buf_in+1+samp/2))&0xf]:\
    nibble_to_int[((*(u8*)(buf_in+1+samp/2))>>4)&0xf];\
    sample_data *= exponent;\
    sample_data <<= 11;\
    sample_data += 1024;\
    sample_data +=\
    factor1 * prev +\
    factor2 * prev_prev;\
    sample_data >>= 11;\
    buf_out[fill_samp++] = samp_clamp(sample_data);\
    prev_prev = prev;\
    prev = sample_data;\
}

#if RWK_PLAYER

#pragma mark THP Format

struct thp_audioframe_header {
    u32 channel_size;
    u32 num_samples;
    s16 left_coefs[16];
    s16 right_coefs[16];
    s16 left_prev, left_prev_prev;
    s16 right_prev, right_prev_prev;
};

static void swap_thp_audioframe_header(struct thp_audioframe_header* header) {
    header->channel_size = swap_u32(header->channel_size);
    header->num_samples = swap_u32(header->num_samples);
    unsigned i;
    for (i=0 ; i<16 ; ++i) {
        header->left_coefs[i] = swap_s16(header->left_coefs[i]);
        header->right_coefs[i] = swap_s16(header->right_coefs[i]);
    }
    header->left_prev = swap_s16(header->left_prev);
    header->left_prev_prev = swap_s16(header->left_prev_prev);
    header->right_prev = swap_s16(header->right_prev);
    header->right_prev_prev = swap_s16(header->right_prev_prev);
}

static void thp_decompress_audioframe(u8* thp_frame_data, s16* buf_out,
                                      unsigned chan_index) {
    
    /* Already byte-swapped */
    struct thp_audioframe_header* audioheader = (struct thp_audioframe_header*)thp_frame_data;
    
    unsigned i;
    u8* left = thp_frame_data + sizeof(struct thp_audioframe_header);
    u8* right = left + audioheader->channel_size;
    
    s16 local_left_prev = audioheader->left_prev;
    s16 local_left_prev_prev = audioheader->left_prev_prev;
    s16 local_right_prev = audioheader->right_prev;
    s16 local_right_prev_prev = audioheader->right_prev_prev;
    
    unsigned packet_count = audioheader->channel_size / 8;
    if (!packet_count && audioheader->num_samples)
        packet_count = audioheader->num_samples / 14;
    
    unsigned left_filled_samples = 0;
    unsigned right_filled_samples = 0;
    
    if (chan_index == 0)
        for (i=0 ; i<packet_count ; ++i, left+=8) {
            u8 index = (*(u8*)left >> 4) & 0xf;
            u32 scale = 1 << (*(u8*)left & 0xf);
            s16 factor1 = audioheader->left_coefs[2*index];
            s16 factor2 = audioheader->left_coefs[2*index+1];
            adpcm_decompress_packet(scale, factor1, factor2, local_left_prev,
                                    local_left_prev_prev, 0, 14, left, buf_out,
                                    left_filled_samples, audioheader->num_samples, 1.0);
        }
    
    else if (chan_index == 1)
        for (i=0 ; i<packet_count ; ++i, right+=8) {
            u8 index = (*(u8*)right >> 4) & 0xf;
            u32 scale = 1 << (*(u8*)right & 0xf);
            s16 factor1 = audioheader->right_coefs[2*index];
            s16 factor2 = audioheader->right_coefs[2*index+1];
            adpcm_decompress_packet(scale, factor1, factor2, local_right_prev,
                                    local_right_prev_prev, 0, 14, right, buf_out,
                                    right_filled_samples, audioheader->num_samples, 1.0);
        }
    
}

/* THP audio callback */
static size_t thp_audio_callback(rwkaudio_voiceplayer_context* ctx,
                                 s16* target_buffer, size_t buffer_size) {
    size_t orig_buf_size = buffer_size;
    
    /* Loop through frames, feeding platform API */
    while (1) {
        
        /* Extract audio data from next frame */
        void* thp_frame_data = rwkgraphics_thp_get_audioframe(ctx->thp.thp_ctx, ctx->thp.next_frame++);
        if (!thp_frame_data && !ctx->loop) {
            /* Non looping audio ran out of samples; give platform no further samples */
            return orig_buf_size-buffer_size;
        } else if (!thp_frame_data && ctx->loop) {
            /* Looping audio ran out of samples; give platform first frame's samples */
            thp_frame_data = rwkgraphics_thp_get_audioframe(ctx->thp.thp_ctx, 0);
            ctx->thp.next_frame = 1;
        }
            
        struct thp_audioframe_header* audioheader = thp_frame_data;
        swap_thp_audioframe_header(audioheader);
        
#if DEBUG
        if (audioheader->num_samples > RWKAUDIO_NUM_BUF_SAMPLES) {
            rwk_log(RWK_ERROR, "audio buffer overflow", "THP frame has %u samples;"
                    "this build of RWK only supports %u samples", audioheader->num_samples,
                    RWKAUDIO_NUM_BUF_SAMPLES);
            return 0;
        }
#endif
        
        /* See if we need another platform buffer */
        unsigned frame_sample_count = audioheader->num_samples*RWKAUDIO_SPEAKERCHANNEL_COUNT;
        unsigned frame_size = frame_sample_count*4;
        if (frame_size > buffer_size) {
            --ctx->thp.next_frame;
            break;
        }
        
        /* Decompress and upmix */
        if (ctx->thp.chan_count == 1) {
            thp_decompress_audioframe(thp_frame_data, ctx->left_decompress_buf, 0);
            audio_upmix_mono(ctx, target_buffer, ctx->left_decompress_buf, audioheader->num_samples, USER_AUDIO_PREFS.music_vol);
        } else if (ctx->thp.chan_count == 2) {
            thp_decompress_audioframe(thp_frame_data, ctx->left_decompress_buf, 0);
            thp_decompress_audioframe(thp_frame_data, ctx->right_decompress_buf, 1);
            audio_upmix_stereo(ctx, target_buffer, ctx->left_decompress_buf,
                               ctx->right_decompress_buf, audioheader->num_samples,
                               USER_AUDIO_PREFS.music_vol);
        }
        
        target_buffer += frame_sample_count;
        buffer_size -= frame_size;
        
    }
    
    
    return orig_buf_size - buffer_size;
    
}
#endif



#pragma mark AGSC Format

/* AGSC audio callback - no loop */
size_t agsc_audio_callback_no_loop(rwkaudio_voiceplayer_context* ctx,
                                   s16* target_buffer, size_t buffer_size) {
    unsigned i;
    
    s32 local_prev = ctx->agsc.prev;
    s32 local_prev_prev = ctx->agsc.prev_prev;
    unsigned local_next_sample_in_packet = ctx->agsc.next_sample_in_packet;
    
    /* Determine sample count to fill */
    int sample_count = RWKAUDIO_NUM_BUF_SAMPLES;
    int remaining_stream_samples = ctx->agsc.sample_count -
    (ctx->agsc.next_packet * 14 + ctx->agsc.next_sample_in_packet);
    
    if (sample_count > remaining_stream_samples)
        sample_count = remaining_stream_samples;
    
    unsigned sample_limit = 14;
    unsigned filled_samples = 0;
    
    /* Decompress samples */
    for (i=ctx->agsc.next_packet ; i<ctx->agsc.packet_count ; ++i) {
        if (i == ctx->agsc.packet_count-1)
            sample_limit = ctx->agsc.sample_count % 14;
        if (!sample_limit)
            sample_limit = 14;
        u8* left = &ctx->agsc.compress_buf[8*i];
        u8 index = (*left >> 4) & 0xf;
        u32 scale = 1 << (*left & 0xf);
        s16 factor1 = ctx->agsc.coefs[2*index];
        s16 factor2 = ctx->agsc.coefs[2*index+1];
        adpcm_decompress_packet(scale, factor1, factor2, local_prev,
                                local_prev_prev, local_next_sample_in_packet, sample_limit,
                                left, ctx->left_decompress_buf, filled_samples, sample_count, 0.9);
        if (filled_samples >= sample_count) {
            ctx->agsc.next_packet = i;
            ctx->agsc.next_sample_in_packet = samp;
            if (samp == 14) {
                ctx->agsc.next_packet = i+1;
                ctx->agsc.next_sample_in_packet = 0;
            }
            break;
        }
        local_next_sample_in_packet = 0;
    }
    
    
    ctx->agsc.prev = local_prev;
    ctx->agsc.prev_prev = local_prev_prev;
    
    
    /* Upmix */
    sample_count = audio_upmix_mono(ctx, target_buffer, ctx->left_decompress_buf, sample_count,
                                    USER_AUDIO_PREFS.sfx_vol);
    return sample_count * 2;
    
}

/* AGSC audio callback - loop */
static size_t agsc_audio_callback_loop(rwkaudio_voiceplayer_context* ctx,
                                       s16* target_buffer, size_t buffer_size) {
    unsigned i;
    
    s32 local_prev = ctx->agsc.prev;
    s32 local_prev_prev = ctx->agsc.prev_prev;
    unsigned local_next_sample_in_packet = ctx->agsc.next_sample_in_packet;
    
    /* Determine sample count to fill */
    unsigned sample_count = RWKAUDIO_NUM_BUF_SAMPLES;
    
    unsigned sample_limit = 14;
    unsigned filled_samples = 0;
    
    /* Decompress samples */
    while (filled_samples < sample_count) {
        
        for (i=ctx->agsc.next_packet ; i<ctx->agsc.packet_count ; ++i) {
            if (i == ctx->agsc.packet_count-1)
                sample_limit = ctx->agsc.loop_end_sample % 14;
            if (!sample_limit)
                sample_limit = 14;
            u8* left = &ctx->agsc.compress_buf[8*i];
            u8 index = (*left >> 4) & 0xf;
            u32 scale = 1 << (*left & 0xf);
            s16 factor1 = ctx->agsc.coefs[2*index];
            s16 factor2 = ctx->agsc.coefs[2*index+1];
            adpcm_decompress_packet(scale, factor1, factor2, local_prev,
                                    local_prev_prev, local_next_sample_in_packet, sample_limit,
                                    left, ctx->left_decompress_buf, filled_samples, sample_count, 0.9);
            if (filled_samples >= sample_count) {
                ctx->agsc.next_packet = i;
                ctx->agsc.next_sample_in_packet = samp;
                if (samp == 14) {
                    ctx->agsc.next_packet = i+1;
                    ctx->agsc.next_sample_in_packet = 0;
                    if (i+1 >= ctx->agsc.packet_count) {
                        ctx->agsc.next_packet = ctx->agsc.loop_start_sample/14;
                        ctx->agsc.next_sample_in_packet = ctx->agsc.loop_start_sample%14;
                        local_prev = ctx->agsc.loop_prev;
                        local_prev_prev = ctx->agsc.loop_prev_prev;
                    }
                }
                break;
            }
            local_next_sample_in_packet = 0;
        }
        if (filled_samples >= sample_count)
            break;
        
        /* Loop to 'loop_start' */
        ctx->agsc.next_packet = ctx->agsc.loop_start_sample/14;
        sample_limit = 14;

        local_prev = ctx->agsc.loop_prev;
        local_prev_prev = ctx->agsc.loop_prev_prev;
        local_next_sample_in_packet = ctx->agsc.loop_start_sample%14;
        for (i=ctx->agsc.next_packet ; i<ctx->agsc.packet_count ; ++i) {
            if (i == ctx->agsc.packet_count-1)
                sample_limit = ctx->agsc.loop_end_sample % 14;
            if (!sample_limit)
                sample_limit = 14;
            u8* left = &ctx->agsc.compress_buf[8*i];
            u8 index = (*left >> 4) & 0xf;
            u32 scale = 1 << (*left & 0xf);
            s16 factor1 = ctx->agsc.coefs[2*index];
            s16 factor2 = ctx->agsc.coefs[2*index+1];
            adpcm_decompress_packet(scale, factor1, factor2, local_prev,
                                    local_prev_prev, local_next_sample_in_packet, sample_limit,
                                    left, ctx->left_decompress_buf, filled_samples, sample_count, 1.0);
            if (filled_samples >= sample_count) {
                ctx->agsc.next_packet = i;
                ctx->agsc.next_sample_in_packet = samp;
                if (samp == 14) {
                    ctx->agsc.next_packet = i+1;
                    ctx->agsc.next_sample_in_packet = 0;
                    if (i+1 >= ctx->agsc.packet_count) {
                        ctx->agsc.next_packet = ctx->agsc.loop_start_sample/14;
                        ctx->agsc.next_sample_in_packet = ctx->agsc.loop_start_sample%14;
                        local_prev = ctx->agsc.loop_prev;
                        local_prev_prev = ctx->agsc.loop_prev_prev;
                    }
                }
                break;
            }
            local_next_sample_in_packet = 0;
        }
            
    }

    ctx->agsc.prev = local_prev;
    ctx->agsc.prev_prev = local_prev_prev;
    
    
    /* Upmix */
    sample_count = audio_upmix_mono(ctx, target_buffer, ctx->left_decompress_buf, sample_count,
                                    USER_AUDIO_PREFS.sfx_vol);
    
    
    return sample_count * 2;
    
}

/* Dynamic callback selection */
size_t agsc_audio_callback(rwkaudio_voiceplayer_context* ctx,
                           s16* target_buffer, size_t buffer_size) {
    if (ctx->agsc.loop_end_sample)
        return agsc_audio_callback_loop(ctx, target_buffer, buffer_size);
    else
        return agsc_audio_callback_no_loop(ctx, target_buffer, buffer_size);
}


#pragma mark DSP Format

struct dsp_header {
    u32 sample_count;
    u32 nibble_count;
    u32 sample_rate;
    u16 loop_flag;
    u16 format;
    u32 loop_start_offset;
    u32 loop_end_offset;
    u32 ca;
    s16 coef[16];
    u16 gain;
    u16 initial_ps;
    s16 initial_hist1;
    s16 initial_hist2;
    u16 loop_ps;
    s16 loop_hist1;
    s16 loop_hist2;
};

static void swap_dsp_header(struct dsp_header* header) {
    unsigned i;
    header->sample_count = swap_u32(header->sample_count);
    header->nibble_count = swap_u32(header->nibble_count);
    header->sample_rate = swap_u32(header->sample_rate);
    header->loop_flag = swap_u16(header->loop_flag);
    header->format = swap_u16(header->format);
    header->loop_start_offset = swap_u32(header->loop_start_offset);
    header->loop_end_offset = swap_u32(header->loop_end_offset);
    header->ca = swap_u32(header->ca);
    for (i=0 ; i<16 ; ++i)
        header->coef[i] = swap_s16(header->coef[i]);
    header->gain = swap_u16(header->gain);
    header->initial_ps = swap_u16(header->initial_ps);
    header->initial_hist1 = swap_s16(header->initial_hist1);
    header->initial_hist2 = swap_s16(header->initial_hist2);
    header->loop_ps = swap_u16(header->loop_ps);
    header->loop_hist1 = swap_s16(header->loop_hist1);
    header->loop_hist2 = swap_s16(header->loop_hist2);
}


/* DSP audio callback - no loop */
static size_t dsp_audio_callback_no_loop(rwkaudio_voiceplayer_context* ctx,
                                         s16* target_buffer, size_t buffer_size) {
    unsigned i;
    
    s16 local_left_prev = ctx->dsp.left_prev;
    s16 local_left_prev_prev = ctx->dsp.left_prev_prev;
    s16 local_right_prev = ctx->dsp.right_prev;
    s16 local_right_prev_prev = ctx->dsp.right_prev_prev;
    
    /* Determine packet and sample count */
    unsigned packet_count = (ctx->dsp.packet_count - ctx->dsp.next_packet);
    if (packet_count > RWKAUDIO_PACKETBUF_COUNT)
        packet_count = RWKAUDIO_PACKETBUF_COUNT;
    unsigned sample_count = packet_count * 14;
    if (!packet_count) {
        //rwkaudio_voiceplayer_stop(ctx);
        ctx->oneshot_done = true;
        return 0;
    }
    
    unsigned left_filled_samples = 0;
    unsigned right_filled_samples = 0;

    /* Decompress samples from left channel */
    fread(ctx->dsp.compress_buf, 8, packet_count, ctx->dsp.l_file);
    for (i=0 ; i<packet_count ; ++i) {
        u8* left = ctx->dsp.compress_buf + 8*i;
        u8 index = (*left >> 4) & 0xf;
        u32 scale = 1 << (*left & 0xf);
        s16 factor1 = ctx->dsp.left_coefs[2*index];
        s16 factor2 = ctx->dsp.left_coefs[2*index+1];
        adpcm_decompress_packet(scale, factor1, factor2, local_left_prev,
                                local_left_prev_prev, 0, 14, left, ctx->left_decompress_buf,
                                left_filled_samples, sample_count, 1.0);
    }
    ctx->dsp.left_prev = local_left_prev;
    ctx->dsp.left_prev_prev = local_left_prev_prev;
    
    /* Decompress samples from right channel (if present) */
    if (ctx->dsp.r_file) {
        fread(ctx->dsp.compress_buf, 8, packet_count, ctx->dsp.r_file);
        for (i=0 ; i<packet_count ; ++i) {
            u8* right = ctx->dsp.compress_buf + 8*i;
            u8 index = (*right >> 4) & 0xf;
            u32 scale = 1 << (*right & 0xf);
            s16 factor1 = ctx->dsp.right_coefs[2*index];
            s16 factor2 = ctx->dsp.right_coefs[2*index+1];
            adpcm_decompress_packet(scale, factor1, factor2, local_right_prev,
                                    local_right_prev_prev, 0, 14, right, ctx->right_decompress_buf,
                                    right_filled_samples, sample_count, 1.0);
        }
        ctx->dsp.right_prev = local_right_prev;
        ctx->dsp.right_prev_prev = local_right_prev_prev;
    }
    
    /* Upmix */
    if (!ctx->dsp.r_file) {
        audio_upmix_mono(ctx, target_buffer, ctx->left_decompress_buf, sample_count,
                         USER_AUDIO_PREFS.music_vol);
    } else {
        audio_upmix_stereo(ctx, target_buffer, ctx->left_decompress_buf,
                           ctx->right_decompress_buf, sample_count,
                           USER_AUDIO_PREFS.music_vol);
    }
    
    ctx->dsp.next_packet += packet_count;
    
    
    return sample_count * 2;
    
}

/* DSP audio callback - loop */
static size_t dsp_audio_callback_loop(rwkaudio_voiceplayer_context* ctx,
                                      s16* target_buffer, size_t buffer_size) {
    unsigned i;
    
    s16 local_left_prev = ctx->dsp.left_prev;
    s16 local_left_prev_prev = ctx->dsp.left_prev_prev;
    s16 local_right_prev = ctx->dsp.right_prev;
    s16 local_right_prev_prev = ctx->dsp.right_prev_prev;
    
    /* Determine packet and sample count */
    unsigned packet_count = RWKAUDIO_PACKETBUF_COUNT;
    unsigned rem_packet_count = (ctx->dsp.packet_count - ctx->dsp.next_packet);
    if (rem_packet_count > packet_count)
        rem_packet_count = packet_count;
    unsigned loop_packet_count = packet_count - rem_packet_count;
    unsigned sample_count = packet_count * 14;
    
    unsigned left_filled_samples = 0;
    unsigned right_filled_samples = 0;

    /* Decompress samples from left channel */
    fread(ctx->dsp.compress_buf, 8, rem_packet_count, ctx->dsp.l_file);
    for (i=0 ; i<rem_packet_count ; ++i) {
        u8* left = &ctx->dsp.compress_buf[8*i];
        u8 index = (*left >> 4) & 0xf;
        u32 scale = 1 << (*left & 0xf);
        s16 factor1 = ctx->dsp.left_coefs[2*index];
        s16 factor2 = ctx->dsp.left_coefs[2*index+1];
        adpcm_decompress_packet(scale, factor1, factor2, local_left_prev,
                                local_left_prev_prev, 0, 14, left, ctx->left_decompress_buf,
                                left_filled_samples, sample_count, 1.0);
    }
    if (loop_packet_count) {
        local_left_prev = ctx->dsp.left_loop_prev;
        local_left_prev_prev = ctx->dsp.left_loop_prev_prev;
        long offset = 0x60 + ctx->dsp.loop_off;
        fseek(ctx->dsp.l_file, offset, SEEK_SET);
        fread(&ctx->dsp.compress_buf[8*i], 8, loop_packet_count, ctx->dsp.l_file);
        for ( ; i<packet_count ; ++i) {
            u8* left = &ctx->dsp.compress_buf[8*i];
            u8 index = (*left >> 4) & 0xf;
            u32 scale = 1 << (*left & 0xf);
            s16 factor1 = ctx->dsp.left_coefs[2*index];
            s16 factor2 = ctx->dsp.left_coefs[2*index+1];
            adpcm_decompress_packet(scale, factor1, factor2, local_left_prev,
                                    local_left_prev_prev, 0, 14, left, ctx->left_decompress_buf,
                                    left_filled_samples, sample_count, 1.0);
        }
    }
    ctx->dsp.left_prev = local_left_prev;
    ctx->dsp.left_prev_prev = local_left_prev_prev;
    
    /* Decompress samples from right channel (if present) */
    if (ctx->dsp.r_file) {
        fread(ctx->dsp.compress_buf, 8, rem_packet_count, ctx->dsp.r_file);
        for (i=0 ; i<rem_packet_count ; ++i) {
            u8* right = &ctx->dsp.compress_buf[8*i];
            u8 index = (*right >> 4) & 0xf;
            u32 scale = 1 << (*right & 0xf);
            s16 factor1 = ctx->dsp.right_coefs[2*index];
            s16 factor2 = ctx->dsp.right_coefs[2*index+1];
            adpcm_decompress_packet(scale, factor1, factor2, local_right_prev,
                                    local_right_prev_prev, 0, 14, right, ctx->right_decompress_buf,
                                    right_filled_samples, sample_count, 1.0);
        }
        if (loop_packet_count) {
            local_right_prev = ctx->dsp.right_loop_prev;
            local_right_prev_prev = ctx->dsp.right_loop_prev_prev;
            long offset = 0x60 + ctx->dsp.loop_off;
            fseek(ctx->dsp.r_file, offset, SEEK_SET);
            fread(&ctx->dsp.compress_buf[8*i], 8, loop_packet_count, ctx->dsp.r_file);
            for ( ; i<packet_count ; ++i) {
                u8* right = &ctx->dsp.compress_buf[8*i];
                u8 index = (*right >> 4) & 0xf;
                u32 scale = 1 << (*right & 0xf);
                s16 factor1 = ctx->dsp.right_coefs[2*index];
                s16 factor2 = ctx->dsp.right_coefs[2*index+1];
                adpcm_decompress_packet(scale, factor1, factor2, local_right_prev,
                                        local_right_prev_prev, 0, 14, right, ctx->right_decompress_buf,
                                        right_filled_samples, sample_count, 1.0);
            }
        }
        ctx->dsp.right_prev = local_right_prev;
        ctx->dsp.right_prev_prev = local_right_prev_prev;
    }
    
    /* Upmix */
    if (!ctx->dsp.r_file) {
        audio_upmix_mono(ctx, target_buffer, ctx->left_decompress_buf, sample_count,
                         USER_AUDIO_PREFS.music_vol);
    } else {
        audio_upmix_stereo(ctx, target_buffer, ctx->left_decompress_buf,
                           ctx->right_decompress_buf, sample_count,
                           USER_AUDIO_PREFS.music_vol);
    }
    
    ctx->dsp.next_packet += packet_count;
    if (loop_packet_count)
        ctx->dsp.next_packet = loop_packet_count;
    
    
    return sample_count * 2;
    
}




#pragma mark RSF Format

/* RSF audio callback - no loop */
static size_t rsf_audio_callback_no_loop(rwkaudio_voiceplayer_context* ctx,
                                         s16* target_buffer, size_t buffer_size) {
    unsigned i;
    
    /* Determine packet and sample count */
    unsigned sample_count = RWKAUDIO_NUM_BUF_SAMPLES;
    unsigned rem_sample_count = (ctx->rsf.sample_count - ctx->rsf.next_sample);
    if (rem_sample_count > sample_count)
        rem_sample_count = sample_count;
    if (!rem_sample_count) {
        //rwkaudio_voiceplayer_stop(ctx);
        ctx->oneshot_done = true;
        return 0;
    }
    
    /* Decompress samples from left channel */
    fread(ctx->rsf.compress_buf, 1, rem_sample_count/2, ctx->rsf.l_file);
    for (i=0 ; i<rem_sample_count ; ++i) {
        u8 audio_byte = ctx->rsf.compress_buf[i/2];
        ctx->left_decompress_buf[i] =
        g721_decoder((i&1)?(audio_byte>>4):(audio_byte), &ctx->rsf.l_state);
    }
    
    /* Decompress samples from right channel */
    fread(ctx->rsf.compress_buf, 1, rem_sample_count/2, ctx->rsf.r_file);
    for (i=0 ; i<rem_sample_count ; ++i) {
        u8 audio_byte = ctx->rsf.compress_buf[i/2];
        ctx->right_decompress_buf[i] =
        g721_decoder((i&1)?(audio_byte>>4):(audio_byte), &ctx->rsf.r_state);
    }
    
    /* Upmix */
    audio_upmix_stereo(ctx, target_buffer, ctx->left_decompress_buf,
                       ctx->right_decompress_buf, rem_sample_count,
                       USER_AUDIO_PREFS.music_vol);
    
    ctx->rsf.next_sample += rem_sample_count;
    
    
    return rem_sample_count * 2;
    
}

/* RSF audio callback - loop */
static size_t rsf_audio_callback_loop(rwkaudio_voiceplayer_context* ctx,
                                      s16* target_buffer, size_t buffer_size) {
    int i;
    
    /* Determine packet and sample count */
    unsigned sample_count = RWKAUDIO_NUM_BUF_SAMPLES;
    unsigned rem_sample_count = (ctx->rsf.sample_count - ctx->rsf.next_sample);
    unsigned loop_sample_count = 0;
    if (rem_sample_count > sample_count)
        rem_sample_count = sample_count;
    else
        loop_sample_count = sample_count - rem_sample_count;
    
    /* Store loop state if performing first play and loop sample will occur in this pass */
    int loop_store_sample = -1;
    if (!ctx->rsf.loop_computed &&
        ctx->rsf.next_sample + sample_count > ctx->rsf.loop_sample) {
        loop_store_sample = ctx->rsf.loop_sample - ctx->rsf.next_sample;
        ctx->rsf.loop_computed = true;
    }
    
    /* Decompress samples from left channel */
    fread(ctx->rsf.compress_buf, 1, rem_sample_count/2, ctx->rsf.l_file);
    for (i=0 ; i<rem_sample_count ; ++i) {
        if (i == loop_store_sample)
            ctx->rsf.l_loop_state = ctx->rsf.l_state;
        u8 audio_byte = ctx->rsf.compress_buf[i/2];
        ctx->left_decompress_buf[i] =
        g721_decoder((i&1)?(audio_byte>>4):(audio_byte), &ctx->rsf.l_state);
    }
    if (loop_sample_count) {
        fseek(ctx->rsf.l_file, ctx->rsf.loop_sample/2, SEEK_SET);
        ctx->rsf.l_state = ctx->rsf.l_loop_state;
        fread(&ctx->rsf.compress_buf[i/2], 1, loop_sample_count/2, ctx->rsf.l_file);
        for ( ; i<sample_count ; ++i) {
            u8 audio_byte = ctx->rsf.compress_buf[i/2];
            ctx->left_decompress_buf[i] =
            g721_decoder((i&1)?(audio_byte>>4):(audio_byte), &ctx->rsf.l_state);
        }
    }
    
    /* Decompress samples from right channel */
    fread(ctx->rsf.compress_buf, 1, rem_sample_count/2, ctx->rsf.r_file);
    for (i=0 ; i<rem_sample_count ; ++i) {
        if (i == loop_store_sample)
            ctx->rsf.r_loop_state = ctx->rsf.r_state;
        u8 audio_byte = ctx->rsf.compress_buf[i/2];
        ctx->right_decompress_buf[i] =
        g721_decoder((i&1)?(audio_byte>>4):(audio_byte), &ctx->rsf.r_state);
    }
    if (loop_sample_count) {
        fseek(ctx->rsf.r_file, ctx->rsf.sample_count/2 + ctx->rsf.loop_sample/2, SEEK_SET);
        ctx->rsf.r_state = ctx->rsf.r_loop_state;
        fread(&ctx->rsf.compress_buf[i/2], 1, loop_sample_count/2, ctx->rsf.r_file);
        for ( ; i<sample_count ; ++i) {
            u8 audio_byte = ctx->rsf.compress_buf[i/2];
            ctx->right_decompress_buf[i] =
            g721_decoder((i&1)?(audio_byte>>4):(audio_byte), &ctx->rsf.r_state);
        }
    }
    
    /* Upmix */
    audio_upmix_stereo(ctx, target_buffer, ctx->left_decompress_buf,
                       ctx->right_decompress_buf, sample_count,
                       USER_AUDIO_PREFS.music_vol);
    
    ctx->rsf.next_sample += sample_count;
    if (loop_sample_count)
        ctx->rsf.next_sample = ctx->rsf.loop_sample + loop_sample_count;
    
    
    return sample_count * 2;
    
}




#pragma mark Voiceplayer Context

static void _new_context(rwkaudio_voiceplayer_context* new_ctx,
                         unsigned sample_rate, rwkaudio_pcm_callback pcm_cb,
                         int stereo) {
    
    new_ctx->oneshot_done = false;
    new_ctx->stop_cb = NULL;
    new_ctx->stop_ctx = NULL;
    
    new_ctx->left_decompress_buf = malloc(RWKAUDIO_NUM_BUF_SAMPLES*2);
    new_ctx->right_decompress_buf = NULL;
    if (stereo) {
        new_ctx->right_decompress_buf = malloc(RWKAUDIO_NUM_BUF_SAMPLES*2);
    }
    
}

#if RWK_PLAYER

/* Start here - Call from Kernel Thread! */
int rwkaudio_voiceplayer_new_thp(rwkaudio_voiceplayer_context* new_ctx, rwkgraphics_thp_context* thp_ctx) {
    
    rwkgraphics_thp_audioinfo* audioinfo = rwkgraphics_thp_get_audioinfo(thp_ctx);
    if (!audioinfo) {
        rwk_log(RWK_ERROR, "invalid THP audio", "no audio present in selected thp context to play");
        return -1;
    }
    
    /* Initialise platform voice */
    _new_context(new_ctx, audioinfo->sample_rate, thp_audio_callback,
                (audioinfo->num_chan==2)?1:0);
    new_ctx->type = THP;
    new_ctx->loop = audioinfo->loop;
    
    new_ctx->thp.next_frame = 0;
    new_ctx->thp.thp_ctx = thp_ctx;
    new_ctx->thp.chan_count = audioinfo->num_chan;
    
    return 0;
    
}

#endif

int rwkaudio_voiceplayer_new_agsc(rwkaudio_voiceplayer_context* new_ctx, int loop) {
    rwkaudio_pcm_callback my_cb = agsc_audio_callback;
    if (!loop)
        my_cb = agsc_audio_callback_no_loop;
    if (loop > 0)
        my_cb = agsc_audio_callback_loop;
    
    _new_context(new_ctx, 0, my_cb, 0);
    new_ctx->type = AGSC;
    new_ctx->loop = loop;
    new_ctx->agsc.clip_id = 0;
    new_ctx->agsc.in_interval = false;
    return 0;
    
}

int rwkaudio_voiceplayer_new_dsp(rwkaudio_voiceplayer_context* new_ctx,
                                 const char* dsp_l_path,
                                 const char* dsp_r_path) {
    
    /* Open file(s) */
    FILE* l_file = fopen(dsp_l_path, "rb");
    if (!l_file) {
        rwk_log(RWK_ERROR, "audio error", "unable to open dsp file `%s`", dsp_l_path);
        return -1;
    }
    FILE* r_file = NULL;
    if (dsp_r_path) {
        r_file = fopen(dsp_r_path, "rb");
        if (!r_file) {
            rwk_log(RWK_ERROR, "audio error", "unable to open dsp file `%s`", dsp_r_path);
            return -1;
        }
    }
    
    /* Left (or mono) file */
    struct dsp_header header;
    fread(&header, 1, sizeof(struct dsp_header), l_file);
    fseek(l_file, 0x60, SEEK_SET);
    swap_dsp_header(&header);
    
    /* Initialise platform voice */
    _new_context(new_ctx, header.sample_rate,
                 (header.loop_flag)?dsp_audio_callback_loop:dsp_audio_callback_no_loop, (dsp_r_path)?1:0);
    new_ctx->type = DSP;
    new_ctx->loop = header.loop_flag;
    
    new_ctx->dsp.left_init_prev = header.initial_hist1;
    new_ctx->dsp.left_init_prev_prev = header.initial_hist2;
    new_ctx->dsp.left_loop_prev = header.loop_hist1;
    new_ctx->dsp.left_loop_prev_prev = header.loop_hist2;
    new_ctx->dsp.left_prev = new_ctx->dsp.left_init_prev;
    new_ctx->dsp.left_prev_prev = new_ctx->dsp.left_init_prev_prev;
    new_ctx->dsp.l_file = l_file;
    new_ctx->dsp.r_file = NULL;
    new_ctx->dsp.loop_off = header.loop_start_offset / 16 * 8;
    new_ctx->dsp.next_packet = 0;
    new_ctx->dsp.packet_count = header.sample_count / 14;
    memcpy(new_ctx->dsp.left_coefs, header.coef, 32);
    new_ctx->dsp.compress_buf = malloc(RWKAUDIO_PACKETBUF_SIZE);
    
    
    /* Right file */
    if (dsp_r_path) {
        struct dsp_header header;
        fread(&header, 1, sizeof(struct dsp_header), r_file);
        fseek(r_file, 0x60, SEEK_SET);
        swap_dsp_header(&header);
        
        new_ctx->dsp.r_file = r_file;
        new_ctx->dsp.right_init_prev = header.initial_hist1;
        new_ctx->dsp.right_init_prev_prev = header.initial_hist2;
        new_ctx->dsp.right_loop_prev = header.loop_hist1;
        new_ctx->dsp.right_loop_prev_prev = header.loop_hist2;
        new_ctx->dsp.right_prev = new_ctx->dsp.right_init_prev;
        new_ctx->dsp.right_prev_prev = new_ctx->dsp.right_init_prev_prev;
        memcpy(new_ctx->dsp.right_coefs, header.coef, 32);

    }
    
    return 0;
    
}


int rwkaudio_voiceplayer_new_rsf(rwkaudio_voiceplayer_context* new_ctx,
                                 const char* rsf_path, int loop_sample) {
    
    /* Open file(s) */
    FILE* rsf_file = fopen(rsf_path, "rb");
    if (!rsf_file) {
        rwk_log(RWK_ERROR, "audio error", "unable to open rsf file `%s`", rsf_path);
        return -1;
    }
    
    /* Initialise platform voice */
    _new_context(new_ctx, 32000, (loop_sample > 0)?rsf_audio_callback_loop:rsf_audio_callback_no_loop, 1);
    new_ctx->type = RSF;
    new_ctx->rsf.l_file = rsf_file;
    new_ctx->rsf.r_file = fopen(rsf_path, "rb");
    new_ctx->rsf.loop_computed = false;
    new_ctx->rsf.loop_sample = loop_sample;
    new_ctx->rsf.compress_buf = malloc(RWKAUDIO_NUM_BUF_SAMPLES/2);
    
    /* Get sample count */
    fseek(rsf_file, 0, SEEK_END);
    new_ctx->rsf.sample_count = (unsigned)ftell(rsf_file);
    fseek(rsf_file, 0, SEEK_SET);
    fseek(new_ctx->rsf.r_file, new_ctx->rsf.sample_count/2, SEEK_SET);
        
    new_ctx->rsf.next_sample = 0;
    
    /* Init audio decoder state */
    g72x_init_state(&new_ctx->rsf.l_state);
    g72x_init_state(&new_ctx->rsf.r_state);
        
    
    return 0;
    
}

#if RWK_TOOLCHAIN

void rwkaudio_rendervoice(unsigned channel_count, unsigned sample_rate, rwkaudio_pcm_callback pcm_cb,
                          rwkaudio_voiceplayer_context* voiceplayer, const char* wave_path);

int rwkaudio_render_rsf(const char* rsf_path, const char* wave_path) {
    
    /* Open file(s) */
    FILE* rsf_file = fopen(rsf_path, "rb");
    if (!rsf_file) {
        rwk_log(RWK_ERROR, "audio error", "unable to open rsf file `%s`", rsf_path);
        return -1;
    }
    
    rwkaudio_voiceplayer_context new_ctx;
    
    /* Initialise platform voice */
    _new_context(&new_ctx, 32000, NULL, 1);
    new_ctx.type = RSF;
    new_ctx.rsf.l_file = rsf_file;
    new_ctx.rsf.r_file = fopen(rsf_path, "rb");
    new_ctx.rsf.compress_buf = malloc(RWKAUDIO_NUM_BUF_SAMPLES/2);
    
    /* Get sample count */
    fseek(rsf_file, 0, SEEK_END);
    new_ctx.rsf.sample_count = (unsigned)ftell(rsf_file);
    fseek(rsf_file, 0, SEEK_SET);
    fseek(new_ctx.rsf.r_file, new_ctx.rsf.sample_count/2, SEEK_SET);
    
    new_ctx.rsf.next_sample = 0;
    
    /* Init audio decoder state */
    g72x_init_state(&new_ctx.rsf.l_state);
    g72x_init_state(&new_ctx.rsf.r_state);
    
    /* Save to wave */
    rwkaudio_rendervoice(2, 32000, rsf_audio_callback_no_loop, &new_ctx, wave_path);
    
    rwkaudio_voiceplayer_destroy(&new_ctx);
    
    return 0;
    
}

#endif


/* Common destroy func */
void rwkaudio_voiceplayer_destroy(rwkaudio_voiceplayer_context* ctx) {
    
    if (ctx->type == DSP) {
        
        if (ctx->dsp.l_file)
            fclose(ctx->dsp.l_file);
        if (ctx->dsp.r_file)
            fclose(ctx->dsp.r_file);
        if (ctx->dsp.compress_buf)
            free(ctx->dsp.compress_buf);
        
    } else if (ctx->type == RSF) {
        
        if (ctx->rsf.l_file)
            fclose(ctx->rsf.l_file);
        if (ctx->rsf.r_file)
            fclose(ctx->rsf.r_file);
        free(ctx->rsf.compress_buf);
        
    } else if (ctx->type == AGSC) {
        
        ctx->agsc.clip_id = 0;
        
    }
    
    if (ctx->left_decompress_buf)
        free(ctx->left_decompress_buf);
    if (ctx->right_decompress_buf)
        free(ctx->right_decompress_buf);
    
    ctx->type = NONE;
    
}



