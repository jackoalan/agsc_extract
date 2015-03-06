/*
 *  rwkaudio_wave_file.c
 *  rwk
 *
 *  Created on 03/16/2014.
 *
 */

#include <stdio.h>
#include <sys/stat.h>
#include "rwkaudio.h"
#include "logging.h"
#include "platform.h"

struct rwkaudio_user_prefs USER_AUDIO_PREFS = {1.0, 1.0};

/* Quick way to make a PCM .wav file of any audio
 * buffer supported by 'rwkaudio' (except THP audio). Don't use
 * looping callbacks! */

struct SampleLoop {
    u32 dwIdentifier;
    u32 dwType;
    u32 dwStart;
    u32 dwEnd;
    u32 dwFraction;
    u32 dwPlayCount;
};

struct SamplerChunk {
    union {
        char idname[4];
        u32 fourcc;
    };
    u32 chunkSize;
    
    u32 dwManufacturer;
    u32 dwProduct;
    u32 dwSamplePeriod;
    u32 dwMIDIUnityNote;
    u32 dwMIDIPitchFraction;
    u32 dwSMPTEFormat;
    u32 dwSMPTEOffset;
    u32 cSampleLoops;
    u32 cbSamplerData;
    struct SampleLoop Loop;
};

void rwkaudio_rendervoice(unsigned channel_count, unsigned sample_rate, rwkaudio_pcm_callback pcm_cb,
                          rwkaudio_voiceplayer_context* voiceplayer, const char* wave_path) {
    
    unsigned sample_count = 0;
    bool loops = false;
    if (voiceplayer->type == RSF)
        sample_count = voiceplayer->rsf.sample_count;
    else if (voiceplayer->type == DSP)
        sample_count = voiceplayer->dsp.packet_count * 14;
    else if (voiceplayer->type == AGSC) {
        sample_count = voiceplayer->agsc.sample_count;
        loops = voiceplayer->agsc.loop_end_sample;
    } else {
        rwk_log(RWK_ERROR, "wave file error", "unable to create '%s'; unsupported input format", wave_path);
        return;
    }
    
    /* Wave header */
    FILE* wavef = fopen(wave_path, "wb");
    fwrite("RIFF", 1, 4, wavef);
    u32 data_size = sample_count * channel_count * 2;
    u32 chunk_size = 36 + data_size;
    if (loops && voiceplayer->type == AGSC)
        chunk_size += 68; /* smpl chunk */
    fwrite(&chunk_size, 1, 4, wavef);
    fwrite("WAVE", 1, 4, wavef);
    
    fwrite("fmt ", 1, 4, wavef);
    u32 sixteen = 16;
    fwrite(&sixteen, 1, 4, wavef);
    u16 audio_fmt = 1;
    fwrite(&audio_fmt, 1, 2, wavef);
    u16 ch_count = channel_count;
    fwrite(&ch_count, 1, 2, wavef);
    u32 samp_rate = sample_rate;
    fwrite(&samp_rate, 1, 4, wavef);
    u16 block_align = channel_count * 2;
    u32 byte_rate = samp_rate * block_align;
    fwrite(&byte_rate, 1, 4, wavef);
    fwrite(&block_align, 1, 2, wavef);
    u16 bps = 16;
    fwrite(&bps, 1, 2, wavef);
    
    fwrite("data", 1, 4, wavef);
    fwrite(&data_size, 1, 4, wavef);
    
    s16 buf_out[NUM_BUF_SAMPLES*2];
    size_t written_size = 0;
    size_t total_samples = 0;
    while (true) {
        written_size = pcm_cb(voiceplayer, buf_out, BUF_SIZE * channel_count);
        if (written_size)
            fwrite(buf_out, 1, written_size, wavef);
        total_samples += (written_size / 2);
        if (total_samples >= sample_count) {
            break;
        }
    }
    
    if (loops && voiceplayer->type == AGSC) {
    
        /* Build smpl chunk */
        struct SamplerChunk smpl = {
            "smpl",
            60,
            0,
            0,
            1000000000 / sample_rate,
            60, /* Middle C */
            0,
            0,
            0,
            1,
            0,
            {
                0x00020000,
                0,
                voiceplayer->agsc.loop_start_sample * 2,
                voiceplayer->agsc.loop_end_sample * 2,
                0,
                0
            }
        };
        
        fwrite(&smpl, 1, sizeof(smpl), wavef);
        
    }
    
    fclose(wavef);
    
}

size_t agsc_audio_callback_no_loop(rwkaudio_voiceplayer_context* ctx,
                                   s16* target_buffer, size_t buffer_size);

void _rwkaudio_agscindex_setclip(rwkaudio_voiceplayer_context* voice);

void rwkaudio_extract_agscs(struct pak_file* audiogrp) {
    
    unsigned i,k;
    
    u8* lookup_table_buf = NULL;
    u16* lookup_table = NULL;
    unsigned lookup_table_count = 0;
    for (i=0 ; i<audiogrp->entry_count ; ++i) {
        struct pak_entry* entry = &audiogrp->entries[i];
        if (!strcmp(entry->name, "sound_lookup")) {
            pak_lookup_get_data(entry, &lookup_table_buf);
            lookup_table_count = swap_u32(*(u32*)lookup_table_buf);
            lookup_table = (u16*)(lookup_table_buf + 4);
            break;
        }
    }
    if (!lookup_table_buf)
        return;
    
    rwkaudio_voiceplayer_context vp;
    rwkaudio_voiceplayer_new_agsc(&vp, 0);
    
    mkdir("AUDIO", 0755);
    
    for (i=0 ; i<audiogrp->entry_count ; ++i) {
        struct pak_entry* entry = &audiogrp->entries[i];
        if (entry->type_num != SWAP_U32((unsigned)'AGSC'))
            continue;
        if (!strcmp(entry->name, "test_AGSC"))
            continue;
        
        char shared_path[256];
        _snprintf(shared_path, 256, "AUDIO/%s/", entry->name);
        mkdir(shared_path, 0755);
        
        rwkaudio_agsc_context agsc;
        rwkaudio_agscindex_load(&agsc, entry);
        
        printf("Extracting '%s' -- %u SDIR entries\n", entry->name, agsc.subclip_count);
        
        for (k=0 ; k<agsc.subclip_count ; ++k) {
            struct AGSC_subclip* sc = &agsc.subclip_table[k];
            
            char out_path[256];
            strlcpy(out_path, shared_path, 256);
            _snprintf(out_path + strlen(out_path), 256, "%s_%u_%04X%s.wav", entry->name, k, sc->myId, sc->loopLengthSamples?"L":"");
            
            vp.agsc.clip_id = sc->myId;
            vp.agsc.an_agsc = &agsc;
            vp.agsc.subclip = sc;
            
            /* Populate voice */
            vp.oneshot_done = false;
            struct AGSC_subclip_meta* scmeta = (struct AGSC_subclip_meta*)(agsc.coefficient_base + sc->offsetToCoeffs);
            vp.agsc.prev = 0;
            vp.agsc.prev_prev = 0;
            vp.agsc.loop_prev = scmeta->loop_prev;
            vp.agsc.loop_prev_prev = scmeta->loop_prev_prev;
            vp.agsc.coefs = scmeta->coefs;
            vp.agsc.compress_buf = agsc.packet_data + sc->startFrameOffset;
            
            if (sc->loopLengthSamples) {
                vp.agsc.loop_start_sample = sc->loopStartSample;
                vp.agsc.loop_end_sample = sc->loopStartSample + sc->loopLengthSamples;
                unsigned samp_cnt = sc->sampleCount & 0xffffff;
                vp.agsc.sample_count = samp_cnt;
                vp.agsc.packet_count = samp_cnt/14 + ((samp_cnt%14)?1:0);
            } else {
                vp.agsc.loop_start_sample = 0;
                vp.agsc.loop_end_sample = 0;
                vp.agsc.sample_count = sc->sampleCount;
                vp.agsc.packet_count = sc->sampleCount/14 + ((sc->sampleCount%14)?1:0);
                vp.agsc.loop_prev = 0;
                vp.agsc.loop_prev_prev = 0;
            }
            
            vp.agsc.next_packet = 0;
            vp.agsc.next_sample_in_packet = 0;
            
            rwkaudio_rendervoice(1, sc->sampleRate, agsc_audio_callback_no_loop, &vp, out_path);
            
        }
        
        rwkaudio_agscindex_unload(&agsc);
        
    }
    
    rwkaudio_voiceplayer_destroy(&vp);
    
    free(lookup_table_buf);
    
}
