/*
 *  pak.c
 *  rwk
 *
 *  Created on 09/29/13.
 *
 */

#include <stdio.h>
#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include "pak.h"
#include "platform.h"

#define DECOMPRESS_BUF_SIZE 65536

void pak_lookup_init(struct pak_lookup* ctx, struct image_file* image) {
    ctx->image = image;
    ctx->pak_count = 0;
}
void pak_lookup_destroy(struct pak_lookup* ctx) {
    int i;
    for (i=0 ; i<ctx->pak_count ; ++i)
        free(ctx->pak_arr[i].entries);
}

/* PAK header */
struct pak_original_header {
    u32 magic;
    u32 padding;
};

struct pak_original_name_entry {
    char type[4];
    u32 uid;
    u32 name_length;
    char name[];
};

struct pak_original_object_entry {
    u32 compressed;
    char type[4];
    u32 uid;
    u32 length;
    u32 offset;
};

int pak_lookup_add_pak_file(struct pak_lookup* ctx, const char* pak_path) {
    int i,j;
    
    struct pak_file* pak = &ctx->pak_arr[ctx->pak_count];
    pak->parent = ctx;
    pak->rt_files.file_a = fopen(pak_path, "rb");
    pak->rt_files.file_b = fopen(pak_path, "rb");
    FILE* file = pak->rt_files.file_a;
    
    /* File Length */
    fseek(file, 0, SEEK_END);
    long total_length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    /* Read first 1MiB of PAK */
    size_t read_size = 1024*1024;
    if (total_length < read_size)
        read_size = total_length;
    
    u8* pak_data = malloc(read_size);
    fread(pak_data, 1, read_size, file);
    
    /* PAK Header */
    struct pak_original_header* pak_head = (struct pak_original_header*)pak_data;
    if (swap_u32(pak_head->magic) != 0x00030005) {
        printf("Invalid PAK Magic\n");
        free(pak_data);
        return -1;
    }
    
    /* Begin cursor */
    u8* pak_cur = pak_data + 8;
    
    /* Traverse name table to object table */
    u32 name_count = *(u32*)pak_cur;
    name_count = swap_u32(name_count);
    pak_cur += 4;
    
    void* name_start = pak_cur;
    for (i=0 ; i<name_count ; ++i) {
        struct pak_original_name_entry* name_ent = (struct pak_original_name_entry*)pak_cur;
        u32 name_len = swap_u32(name_ent->name_length);
        pak_cur += 12 + name_len;
    }
    
    /* Iterate object table */
    u32 object_count = *(u32*)pak_cur;
    object_count = swap_u32(object_count);
    pak_cur += 4;
    
    pak->entry_count = object_count;
    pak->entries = malloc(object_count * sizeof(struct pak_entry));
    
    for (i=0 ; i<object_count ; ++i) {
        struct pak_entry* target = &pak->entries[i];
        target->parent = pak;
        struct pak_original_object_entry* obj_ent = (struct pak_original_object_entry*)pak_cur;
        
        /* Skip if this entry is already added (lots of duplicates to strip) */
        u8 already_added = 0;
        for (j=0 ; j<i ; ++j) {
            struct pak_entry* orig_test = &pak->entries[j];
            if (orig_test->uid == obj_ent->uid) {
                already_added = 1;
                break;
            }
        }
        if (already_added) {
            --object_count;
            --i;
            pak_cur += 20;
            continue;
        }
        //printf("Added %s - %08X\n", pak_node->name, swap_u32(obj_ent->uid));
        
        /* See if there's a corresponding name entry */
        memset(target->name, 0, 64);
        u8* name_cur = name_start;
        for (j=0 ; j<name_count ; ++j) {
            struct pak_original_name_entry* name_ent = (struct pak_original_name_entry*)name_cur;
            u32 name_len = swap_u32(name_ent->name_length);
            if (name_ent->uid == obj_ent->uid) {
                strncpy(target->name, name_ent->name, name_len);
                break;
            }
            name_cur += 12 + name_len;
        }
        
        target->compressed = swap_u32(obj_ent->compressed);
        memcpy(target->type, obj_ent->type, 4);
        target->uid = obj_ent->uid;
        target->offset = swap_u32(obj_ent->offset);
        target->length = swap_u32(obj_ent->length);
        target->shared_entry = NULL;
        
        pak_cur += 20;
    }
    
    //printf("ORIG %u NEW %u\n", pak->entry_count, object_count);
    pak->entry_count = object_count;
    
    free(pak_data);
    ++ctx->pak_count;
    
    return 0;
    
}


struct pak_entry* pak_lookup_get_entry(struct pak_lookup* ctx, u32 uid) {
    int i,j;
    
    struct pak_entry* best_entry = NULL;
    for (i=0 ; i<ctx->pak_count ; ++i) {
        struct pak_file* pak = &ctx->pak_arr[i];
        for (j=0 ; j<pak->entry_count ; ++j) {
            struct pak_entry* entry = &pak->entries[j];
            if (entry->uid == uid) {
                if (best_entry && entry->name[0])
                    best_entry = entry;
                else if (!best_entry)
                    best_entry = entry;
                break;
            }
        }
    }
    
    return best_entry;
    
}

struct pak_entry* pak_lookup_get_name(struct pak_lookup* ctx, const char* name) {
    int i,j;
    
    for (i=0 ; i<ctx->pak_count ; ++i) {
        struct pak_file* pak = &ctx->pak_arr[i];
        for (j=0 ; j<pak->entry_count ; ++j) {
            struct pak_entry* entry = &pak->entries[j];
            if (!strcmp(entry->name, name))
                return entry;
        }
    }
    
    return NULL;
}


size_t pak_lookup_get_data(struct pak_entry* entry, u8** data_out) {
    
    FILE* pak_file = entry->parent->rt_files.file_a;
    
    size_t offset = entry->offset;
    
    if (entry->compressed) {
        
        /* Decompress data using zlib */
        u32 decomp_len = 0;
        fseek(pak_file, offset, SEEK_SET);
        fread(&decomp_len, 1, 4, pak_file);
        decomp_len = swap_u32(decomp_len);
        
        unsigned char comp_buf[DECOMPRESS_BUF_SIZE];
        void* decomp_data = malloc(decomp_len);
        
        z_stream z_ctx;
        memset(&z_ctx, 0, sizeof(z_stream));
        z_ctx.avail_out = decomp_len;
        z_ctx.next_out = decomp_data;
        inflateInit(&z_ctx);
        
        size_t comp_offset = offset + 4;
        fseek(pak_file, comp_offset, SEEK_SET);
        while (z_ctx.avail_out) {
            size_t bytes_read = fread(comp_buf, 1, DECOMPRESS_BUF_SIZE, pak_file);
            comp_offset += bytes_read;
            z_ctx.avail_in = (uInt)bytes_read;
            z_ctx.next_in = comp_buf;
            inflate(&z_ctx, Z_FINISH);
        }
        
        inflateEnd(&z_ctx);
        
        *data_out = decomp_data;
        return decomp_len;
        
        
    } else {
        
        void* read_data = malloc(entry->length);
        fseek(pak_file, offset, SEEK_SET);
        fread(read_data, 1, entry->length, pak_file);
        *data_out = read_data;
        return entry->length;
        
    }
    
}


