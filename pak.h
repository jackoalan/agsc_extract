/*
 *  pak.h
 *  rwk
 *
 *  Created on 09/29/13.
 *
 */

#ifndef RWK_PAKLookup_h
#define RWK_PAKLookup_h

#include <stdint.h>
#include "platform.h"

struct pak_file;
struct pak_lookup;

struct pak_entry {
    struct pak_file* parent;
    char name[64];
    union {
        char type[4];
        u32 type_num;
    };
    u32 compressed;
    u32 uid;
    size_t offset;
    size_t length;
    
    /* If set, this entry occurs in multiple PAK files */
    struct pak_entry* shared_entry;
    
};

struct pak_file {
    struct pak_lookup* parent;
    union {
        struct tree* pak_node;
        struct {
            FILE* file_a;
            FILE* file_b;
        } rt_files;
    };
    u32 entry_count;
    struct pak_entry* entries;
};

struct pak_lookup {
    struct image_file* image;
    int pak_count;
    struct pak_file pak_arr[32];
};

void pak_lookup_init(struct pak_lookup* ctx, struct image_file* image);
void pak_lookup_destroy(struct pak_lookup* ctx);

int pak_lookup_add_pak_file(struct pak_lookup* ctx, const char* pak_path);

struct pak_entry* pak_lookup_get_entry(struct pak_lookup* ctx, u32 uid);
struct pak_entry* pak_lookup_get_name(struct pak_lookup* ctx, const char* name);
size_t pak_lookup_get_data(struct pak_entry* entry, u8** data_out);
size_t pak_lookup_get_length(struct pak_entry* entry);


#endif
