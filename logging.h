/*
 *  rwkplayer_logging.h
 *  rwk
 *
 *  Created on 11/23/2013.
 *
 */

#ifndef RWK_rwk_logging_h
#define RWK_rwk_logging_h


enum LOG_TYPE {
    RWK_NONE       = 0,
    RWK_ERROR      = 1,
    RWK_WARN       = 1<<1,
    RWK_INFO       = 1<<2,
    RWK_PYTHON_EXC = 1<<3
};

#if __GNUC__
#include <sys/cdefs.h>
#ifndef __printflike
#define __printflike(n,m) __attribute__((format(printf,n,m)))
#endif
void rwk_log(enum LOG_TYPE type, const char* subject, const char* msgf, ...) __printflike(3, 4);
#else
void rwk_log(enum LOG_TYPE type, const char* subject, const char* msgf, ...);
#endif

#endif
