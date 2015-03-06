//
//  main.c
//  agsc_extract
//
//  Created by Jack Andersen on 03/06/2015.
//  Copyright (c) 2015 Jack Andersen. All rights reserved.
//

#include <stdio.h>
#include <sys/stat.h>
#include "pak.h"
#include "logging.h"

struct pak_lookup PAK_LOOKUP = {0};
void rwkaudio_extract_agscs(struct pak_file* audiogrp);

int main(int argc, const char** argv) {
    if (argc < 2) {
        printf("Usage: agsc_extract <AudioGrp.pak>\n");
        return 0;
    }
    struct stat pak_st;
    if (stat(argv[1], &pak_st)) {
        rwk_log(RWK_ERROR, "file err", "unable to open PAK file");
        return -1;
    }
    if (pak_lookup_add_pak_file(&PAK_LOOKUP, argv[1]) < 0) {
        rwk_log(RWK_ERROR, "file err", "unable to parse PAK file");
        return -1;
    }
    rwkaudio_extract_agscs(&PAK_LOOKUP.pak_arr[0]);
    return 0;
}
