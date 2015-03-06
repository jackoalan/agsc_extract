/*
 *  rwkplayer_logging.c
 *  rwk
 *
 *  Created on 11/23/2013.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "logging.h"

#define ERR_TEMPLATE "RWK ERROR - %s - %s\n"
#define WARN_TEMPLATE "RWK WARNING - %s - %s\n"
#define INFO_TEMPLATE "RWK INFO - %s - %s\n"


void rwk_log(enum LOG_TYPE type, const char* subject, const char* msgf, ...) {
    
    va_list va;
    va_start(va, msgf);
#if _WIN32
    char* log_buf = malloc(4096);
    vsprintf(log_buf, msgf, va);
#else
    char* log_buf = NULL;
    vasprintf(&log_buf, msgf, va);
#endif
    va_end(va);
    
    if (type & RWK_ERROR)
        fprintf(stderr, ERR_TEMPLATE, subject, log_buf);
    else if (type & RWK_WARN)
        fprintf(stderr, WARN_TEMPLATE, subject, log_buf);
    else if (type & RWK_INFO)
        fprintf(stderr, INFO_TEMPLATE, subject, log_buf);
    
    
    free(log_buf);
    
}
