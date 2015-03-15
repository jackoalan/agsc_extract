/*
 *  rwkaudio_agscindex.c
 *  rwk
 *
 *  Created on 12/17/2013.
 *
 */

#include <stdio.h>
#include "rwkaudio.h"
#include "logging.h"
#include "platform.h"

#if RWK_PLAYER
#include "../rwkdata/rwkdata.h"
#endif


struct AGSC_t2_index {
    u16 gameKey;
    u16 table1Key;
    u16 un1;
    u16 un2;
    u16 un3;
};



static void swap_subclip_meta(struct AGSC_subclip_meta* meta) {
    meta->prev = swap_s16(meta->prev);
    meta->prev_prev = swap_s16(meta->prev_prev);
    meta->loop_prev = swap_s16(meta->loop_prev);
    meta->loop_prev_prev = swap_s16(meta->loop_prev_prev);
    unsigned i;
    for (i=0 ; i<16 ; ++i)
        meta->coefs[i] = swap_s16(meta->coefs[i]);
}

static void swap_t2_index(struct AGSC_t2_index* t2) {
    t2->gameKey = swap_u16(t2->gameKey);
    t2->table1Key = swap_u16(t2->table1Key);
    t2->un1 = swap_u16(t2->un1);
    t2->un2 = swap_u16(t2->un2);
    t2->un3 = swap_u16(t2->un3);
}

static void swap_subclip(struct AGSC_subclip* sc) {
    sc->myId = swap_u16(sc->myId);
    sc->startFrameOffset = swap_u32(sc->startFrameOffset);
    sc->sampleRate = swap_u16(sc->sampleRate);
    sc->sampleCount = swap_u32(sc->sampleCount);
    sc->loopStartSample = swap_u32(sc->loopStartSample);
    sc->loopLengthSamples = swap_u32(sc->loopLengthSamples);
    sc->offsetToCoeffs = swap_u32(sc->offsetToCoeffs);
}

static u16 resolve_clip_key_from_t1_key(u16 t1Key, u8* t1, u32 t1Len, u8** t1dataout) {
    
    /* First entry of table 1 (gives total length to count down) */
    u32 tableCur = 0;
    u32 secLen = swap_u32(*(u32*)(t1+tableCur));
    s32 tableRem = t1Len;
    tableCur += secLen;
    tableRem -= secLen;
    
    /* Start parsing remaining sections */
    while(tableRem > 0)
    {
        secLen = swap_u32(*(u32*)(t1+tableCur));
        if(secLen == 0xffffffff) /* Done with table */
            break;
        
        /* My own key */
        u16 selfKey = swap_u16(*(u16*)(t1+tableCur+4));
        
        /* Search for buffer key */
        u32 bufferKeyOff = 8;
        u8 bufferKeyTest = *(u8*)(t1+tableCur+bufferKeyOff+3);
        while(bufferKeyTest != 0x10)
        {
            bufferKeyOff += 8;
            bufferKeyTest = *(u8*)(t1+tableCur+bufferKeyOff+3);
        }
        
        u16 bufferKey = swap_u16(*(u16*)(t1+tableCur+bufferKeyOff+1));
        
        if(selfKey == t1Key) {
            *t1dataout = (t1+tableCur+8);
            return bufferKey;
        }
        
        
        tableCur += secLen;
        tableRem -= secLen;
    }
    
    *t1dataout = NULL;
    return 0;
}



/* Load selected AGSC file from `AudioGrp.pak` */
void rwkaudio_agscindex_load(rwkaudio_agsc_context* new_agsc, struct pak_entry* agsc_entry) {
    unsigned i;
    
    /* Special MIDI-driven wavetable; skip */
    if (!strcmp(agsc_entry->name, "test_AGSC"))
        return;
    
    /* Populate AGSC */
    pak_lookup_get_data(agsc_entry, &new_agsc->agsc_buf);
    u8* agsc_cur = new_agsc->agsc_buf;
    
    /* Determine if AGSC is MP1/MP2 */
    u32 is_mp2 = swap_u32(*(u32*)agsc_cur);
    u16 project_count = 0; /* MP2 only */
    u32 pool_len;
    u8* pool;
    u32 project_len;
    u8* project;
    u32 sample_len;
    u8* sample;
    u32 sample_dir_len;
    u8* sample_dir;

    if (is_mp2 == 0x1) {
        agsc_cur += 4;
        new_agsc->name = agsc_cur;
        agsc_cur += strlen((char*)agsc_cur)+1;
        project_count = swap_u16(*(u32*)agsc_cur);
        agsc_cur += 2;
        pool_len = swap_u32(*(u32*)agsc_cur);
        agsc_cur += 4;
        project_len = swap_u32(*(u32*)agsc_cur);
        agsc_cur += 4;
        sample_dir_len = swap_u32(*(u32*)agsc_cur);
        agsc_cur += 4;
        sample_len = swap_u32(*(u32*)agsc_cur);
        agsc_cur += 4;
        pool = agsc_cur;
        agsc_cur += pool_len;
        project = agsc_cur;
        agsc_cur += project_len;
        sample_dir = agsc_cur;
        agsc_cur += sample_dir_len;
        sample = agsc_cur;

    } else {
        agsc_cur += strlen((char*)agsc_cur)+1;
        new_agsc->name = agsc_cur;
        agsc_cur += strlen((char*)agsc_cur)+1;
        pool_len = swap_u32(*(u32*)agsc_cur);
        agsc_cur += 4;
        pool = agsc_cur;
        agsc_cur += pool_len;
        project_len = swap_u32(*(u32*)agsc_cur);
        agsc_cur += 4;
        project = agsc_cur;
        agsc_cur += project_len;
        sample_len = swap_u32(*(u32*)agsc_cur);
        agsc_cur += 4;
        sample = agsc_cur;
        agsc_cur += sample_len;
        sample_dir_len = swap_u32(*(u32*)agsc_cur);
        agsc_cur += 4;
        sample_dir = agsc_cur;
        
    }
    
    
    /* Build table for table 2 links */
    u16 table_two_count;
    u8* table_two_index = NULL;
    if (is_mp2 && project_count == 0xffff) {
        table_two_count = 0;
    } else {
        table_two_index = project + swap_u32(*(u32*)(project + 28));
        table_two_count = swap_u16(*(u16*)(table_two_index));
    }
    new_agsc->link_count = table_two_count;
    if (table_two_count)
        new_agsc->link_table = malloc(sizeof(struct AGSC_link) * table_two_count);
    else
        new_agsc->link_table = NULL;
    
    for(i=0 ; i<table_two_count ; ++i) {
        struct AGSC_t2_index* thisT2I = (struct AGSC_t2_index*)(table_two_index + 4 + 10*i);
        swap_t2_index(thisT2I);
        
        new_agsc->link_table[i].tbl1Key = thisT2I->table1Key;
        new_agsc->link_table[i].tbl2Key = thisT2I->gameKey;
        new_agsc->link_table[i].clipKey = resolve_clip_key_from_t1_key(thisT2I->table1Key,
                                                                       pool, pool_len,
                                                                       (u8**)&new_agsc->link_table[i].tbl1Data);
    }
    
    
    /* Record frame table pointer and advance (ADPCM DATA STORED HERE!) */
    new_agsc->packet_data = sample;
    
    /* Record subclip table pointer (ADPCM COEFFICIENTS STORED HERE!!) */
    new_agsc->subclip_table = (struct AGSC_subclip*)sample_dir;
    new_agsc->coefficient_base = sample_dir;
    
    /* Count subclips */
    u32 subclip_count = 0;
    u16 testJib = swap_u16(*(u16*)(sample_dir));
    while (testJib != 0xFFFF)
    {
        ++subclip_count;
        testJib = swap_u16(*(u16*)(sample_dir+(32*subclip_count)));
    }
    new_agsc->subclip_count = subclip_count;
    
    /* Make clip array and discover table 2 links */
    for(i=0 ; i<subclip_count ; ++i) {
        
        struct AGSC_subclip* thisClip = (struct AGSC_subclip*)(sample_dir+(32*i));
        swap_subclip(thisClip);
        
        struct AGSC_subclip_meta* scmeta = (struct AGSC_subclip_meta*)(sample_dir + thisClip->offsetToCoeffs);
        swap_subclip_meta(scmeta);
        
    }
    
}

void rwkaudio_agscindex_unload(rwkaudio_agsc_context* an_agsc) {
    free(an_agsc->agsc_buf);
    if (an_agsc->link_table)
        free(an_agsc->link_table);
}

static void _rwkaudio_agscindex_setclip(rwkaudio_voiceplayer_context* voice) {
    
    rwkaudio_agsc_context* an_agsc = voice->agsc.an_agsc;
    struct AGSC_subclip* subclip = voice->agsc.subclip;
    
    /* Notify platform of sample rate */
    rwkaudio_init_agsc_voice(voice);
    
    /* Populate voice */
    voice->oneshot_done = false;
    struct AGSC_subclip_meta* scmeta = (struct AGSC_subclip_meta*)(an_agsc->coefficient_base + subclip->offsetToCoeffs);
    voice->agsc.prev = 0;
    voice->agsc.prev_prev = 0;
    voice->agsc.loop_prev = scmeta->loop_prev;
    voice->agsc.loop_prev_prev = scmeta->loop_prev_prev;
    voice->agsc.coefs = scmeta->coefs;
    voice->agsc.compress_buf = an_agsc->packet_data + subclip->startFrameOffset;
    
    if (subclip->loopLengthSamples) {
        voice->agsc.loop_start_sample = subclip->loopStartSample;
        voice->agsc.loop_end_sample = subclip->loopStartSample + subclip->loopLengthSamples;
        voice->agsc.sample_count = voice->agsc.loop_end_sample;
        voice->agsc.packet_count = voice->agsc.loop_end_sample/14 + ((voice->agsc.loop_end_sample%14)?1:0);
    } else {
        voice->agsc.loop_start_sample = 0;
        voice->agsc.loop_end_sample = 0;
        voice->agsc.sample_count = subclip->sampleCount;
        voice->agsc.packet_count = subclip->sampleCount/14 + ((subclip->sampleCount%14)?1:0);
        voice->agsc.loop_prev = 0;
        voice->agsc.loop_prev_prev = 0;
    }
    
    voice->agsc.next_packet = 0;
    voice->agsc.next_sample_in_packet = 0;
    
}

#if RWK_PLAYER

int rwkaudio_agscindex_setclip(rwkaudio_voiceplayer_context* voice,
                               PyObject* agsc_set,
                               u16 orig_clip_id) {
    unsigned i,j;
    u16 clip_id = swap_u16(RWKDATA_SOUND_LOOKUP[orig_clip_id]);
    
    if (voice->type != AGSC) {
        rwk_log(RWK_ERROR|RWK_PYTHON_EXC, "agsc error", "Attempted to load '%04X' into non-AGSC voice context", orig_clip_id);
        return -1;
    }
    
    /* Temporary index-based lookup (For discovering clips) */
    //clip_id = AGSCS[0].link_table[clip_id].tbl2Key;
    
    /* Reuse previous clip if it matches */
    if (clip_id && clip_id == voice->agsc.clip_id) {
        _rwkaudio_agscindex_setclip(voice);
        return 0;
    }
    
    /* Lookup clip */
    Py_ssize_t pos = 0;
    rwkdata_agsc_object* data_obj;
    Py_hash_t hash;
    while (_PySet_NextEntry(agsc_set, &pos, (PyObject**)&data_obj, &hash)) {
        
        rwkaudio_agsc_context* an_agsc = &data_obj->agsc;
        
        for (j=0 ; j<an_agsc->link_count ; ++j) {
            if (an_agsc->link_table[j].tbl2Key == clip_id) {
                
                /* FOUND CLIP! */
                for (i=0 ; i<an_agsc->subclip_count ; ++i) {
                    struct AGSC_subclip* subclip = &an_agsc->subclip_table[i];
                    if (subclip->myId == an_agsc->link_table[j].clipKey) {
                        
                        /* FOUND SUBCLIP!! */
                        voice->agsc.clip_id = clip_id;
                        voice->agsc.an_agsc = an_agsc;
                        voice->agsc.subclip = subclip;
                        voice->agsc.cue_data = an_agsc->link_table[j].tbl1Data;
                        
                        _rwkaudio_agscindex_setclip(voice);
                        
                        return 0;
                    }
                }
                
                rwk_log(RWK_ERROR|RWK_PYTHON_EXC, "agsc error", "unable to find audio subclip '%04X'",
                        an_agsc->link_table[j].clipKey);
                return -1;
            }
        }
        
    }
    
    /* Did not find clip */
    rwk_log(RWK_ERROR|RWK_PYTHON_EXC, "agsc error", "unable to find audio clip '%04X'", clip_id);
    return -1;
    
}

#elif RWK_TOOLCHAIN

void rwkaudio_agscindex_setclip(rwkaudio_voiceplayer_context* voice,
                                rwkaudio_agsc_context* an_agsc,
                                u16 clip_id) {
    unsigned i,j;
    
    if (voice->type != AGSC) {
        rwk_log(RWK_ERROR, "agsc error", "Attempted to load '%04X' into non-AGSC voice context", clip_id);
        return;
    }
    
    /* Temporary index-based lookup (For discovering clips) */
    //clip_id = AGSCS[0].link_table[clip_id].tbl2Key;
    
    /* Reuse previous clip if it matches */
    if (clip_id && clip_id == voice->agsc.clip_id) {
        _rwkaudio_agscindex_setclip(voice);
        return;
    }
    
    
    for (j=0 ; j<an_agsc->link_count ; ++j) {
        if (an_agsc->link_table[j].tbl2Key == clip_id) {
            
            /* FOUND CLIP! */
            for (i=0 ; i<an_agsc->subclip_count ; ++i) {
                struct AGSC_subclip* subclip = &an_agsc->subclip_table[i];
                if (subclip->myId == an_agsc->link_table[j].clipKey) {
                    
                    /* FOUND SUBCLIP!! */
                    voice->agsc.clip_id = clip_id;
                    voice->agsc.an_agsc = an_agsc;
                    voice->agsc.subclip = subclip;
                    voice->agsc.cue_data = an_agsc->link_table[j].tbl1Data;
                    
                    _rwkaudio_agscindex_setclip(voice);
                    
                    return;
                }
            }
            
            rwk_log(RWK_ERROR, "agsc error", "unable to find audio subclip '%04X'",
                    an_agsc->link_table[j].clipKey);
            return;
        }
    }
    
    /* Did not find clip */
    rwk_log(RWK_ERROR, "agsc error", "unable to find audio clip '%04X'", clip_id);
    
}

#endif

