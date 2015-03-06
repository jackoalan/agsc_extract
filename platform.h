/*
 *  platform.h
 *  rwk
 *
 *  Created on 01/03/2014.
 *
 */

#ifndef rwk_platform_h
#define rwk_platform_h


#define _USE_MATH_DEFINES 1
#include <math.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif

#ifndef M_PI_2
#define M_PI_2 1.5707963267948966192E0
#endif

/* Platform Core APIs */
#if _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include "dirent.h"
#include <stdbool.h>
#define MAXPATHLEN 1024
#define SIGWINCH 999
#include <io.h>
#include <fcntl.h>
#include "winsupport.h"

#define MIN min
#define MAX max

#define INLINE __inline
#define rwksys_open _open
#define rwksys_close _close
#define rwksys_dup2 _dup2
#define rwksys_unlink _unlink
#define rwksys_putenv _putenv

void rwkwin_print_err(DWORD err);


/* Windows Thread */
#define RWK_THREAD_T HANDLE
#define rwk_thread_create(thread, func, ctx) 0;thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(func), ctx, 0, NULL)
#define rwk_thread_create_ss(thread, func, ctx, size) 0;thread = CreateThread(NULL, size, (LPTHREAD_START_ROUTINE)(func), ctx, 0, NULL)
#define rwk_thread_join(thread) WaitForSingleObject(thread, INFINITE)
#define rwk_thread_kill(thread, sig) TerminateThread(thread, sig)
#define rwk_thread_setname(name)


#else
#include <dirent.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

#ifdef __linux__
# include "linuxsupport.h"
#endif

#define INLINE inline
#define rwksys_open open
#define rwksys_close close
#define rwksys_dup2 dup2
#define rwksys_unlink unlink
#define rwksys_putenv putenv


/* Pthread Thread */
#include <pthread.h>
#define RWK_THREAD_T pthread_t
#define rwk_thread_create(thread, func, ctx) pthread_create(&(thread), NULL, func, ctx)
static inline int _rwk_thread_create_ss(pthread_t* thread, void*(*func)(void*),
                                        void* ctx, size_t stack_sz) {
    pthread_attr_t stack_size_attr;
    pthread_attr_init(&stack_size_attr);
    pthread_attr_setstacksize(&stack_size_attr, stack_sz);
    int ret = pthread_create(thread, &stack_size_attr, func, ctx);
    pthread_attr_destroy(&stack_size_attr);
    return ret;
}
#define rwk_thread_create_ss(thread, func, ctx, size) _rwk_thread_create_ss(&(thread), func, ctx, size)
#define rwk_thread_join(thread) pthread_join(thread, NULL)
#define rwk_thread_kill(thread, sig) pthread_kill(thread, sig)
#ifdef __APPLE__
#define rwk_thread_setname(name) pthread_setname_np(name)
#else
#define rwk_thread_setname(name)
#endif

#endif



/* Platform Mutex Type */
#if defined(_WIN32)
/* Windows Critical Section */
#define RWK_MUTEX_T CRITICAL_SECTION
#define rwk_mutex_create(mutex) 0;InitializeCriticalSection(&(mutex))
#define rwk_mutex_destroy(mutex) DeleteCriticalSection(&(mutex))
#define rwk_mutex_lock(mutex) EnterCriticalSection(&(mutex))
#define rwk_mutex_trylock(mutex) (TryEnterCriticalSection(&(mutex)) == 0)
#define rwk_mutex_unlock(mutex) LeaveCriticalSection(&(mutex))

#else
/* Pthread Mutex */
#define RWK_MUTEX_T pthread_mutex_t
#define rwk_mutex_create(mutex) pthread_mutex_init(&(mutex), NULL)
#define rwk_mutex_destroy(mutex) pthread_mutex_destroy(&(mutex))
#define rwk_mutex_lock(mutex) pthread_mutex_lock(&(mutex))
#define rwk_mutex_trylock(mutex) (pthread_mutex_trylock(&(mutex)) == EBUSY)
#define rwk_mutex_unlock(mutex) pthread_mutex_unlock(&(mutex))

#endif



/* Platform Cond Type */
#if defined(_WIN32)
/* Windows Condition Variable */
#define RWK_COND_T CONDITION_VARIABLE
#define rwk_cond_create(cond) 0;InitializeConditionVariable(&(cond))
#define rwk_cond_destroy(cond)
#define rwk_cond_wait(cond, mutex) SleepConditionVariableCS(&(cond), &(mutex), INFINITE)
#define rwk_cond_signal(cond) WakeConditionVariable(&(cond))

#else
/* Pthread Cond */
#define RWK_COND_T pthread_cond_t
#define rwk_cond_create(cond) pthread_cond_init(&(cond), NULL)
#define rwk_cond_destroy(cond) pthread_cond_destroy(&(cond))
#define rwk_cond_wait(cond, mutex) pthread_cond_wait(&(cond), &(mutex))
#define rwk_cond_signal(cond) pthread_cond_signal(&(cond))

#endif



/* Platform Semaphore Type */
#if defined(__APPLE__)
/* Grand Central Dispatch semaphore */
#include <dispatch/dispatch.h>
#define RWK_SEMAPHORE_T dispatch_semaphore_t
#define rwk_semaphore_create(sem, count) (sem) = dispatch_semaphore_create(count)
#define rwk_semaphore_destroy(sem, count) {\
unsigned i;\
for (i=0 ; i<(count) ; ++i)\
dispatch_semaphore_wait(sem, DISPATCH_TIME_NOW);\
for (i=0 ; i<(count) ; ++i)\
dispatch_semaphore_signal(sem);\
dispatch_release(sem);\
}
#define rwk_semaphore_empty(sem, count) {\
unsigned i;\
for (i=0 ; i<(count) ; ++i)\
dispatch_semaphore_wait(sem, DISPATCH_TIME_NOW);\
for (i=0 ; i<(count) ; ++i)\
dispatch_semaphore_signal(sem);\
}
#define rwk_semaphore_fill(sem, count) {\
unsigned i;\
for (i=0 ; i<(count) ; ++i)\
dispatch_semaphore_wait(sem, DISPATCH_TIME_NOW);\
}
#define rwk_semaphore_wait(sem) dispatch_semaphore_wait((sem), DISPATCH_TIME_FOREVER)
#define rwk_semaphore_signal(sem) dispatch_semaphore_signal(sem)

#elif defined(_WIN32)
/* Windows semaphore */
#define RWK_SEMAPHORE_T HANDLE
#define rwk_semaphore_create(sem, count) sem = CreateSemaphore(NULL, count, count, NULL)
#define rwk_semaphore_destroy(sem, count) CloseHandle((sem))
#define rwk_semaphore_empty(sem, count) { \
while (ReleaseSemaphore(sem, 1, NULL)) {}\
}
#define rwk_semaphore_wait(sem) WaitForSingleObject((sem), INFINITE)
#define rwk_semaphore_signal(sem) ReleaseSemaphore((sem), 1, NULL)

#else
/* <semaphore.h> semaphore (most UNIX-y OSes) */
#include <semaphore.h>
#define RWK_SEMAPHORE_T sem_t
#define rwk_semaphore_create(sem, count) sem_init(&(sem), 0, count)
#define rwk_semaphore_destroy(sem, count) sem_destroy(&(sem))
#define rwk_semaphore_empty(sem, count) {\
unsigned i;\
for (i=0 ; i<(count) ; ++i)\
sem_post(&(sem));\
}
#define rwk_semaphore_wait(sem) sem_wait(&(sem))
#define rwk_semaphore_signal(sem) sem_post(&(sem))

#endif



/* Platform thread-local enumeration */
enum rwk_thread_type {
    RWK_THREAD_TYPE_NONE = 0,
    RWK_THREAD_TYPE_KERNEL = 1,
    RWK_THREAD_TYPE_LOADER = 2
};
#if RWK_PLAYER
#if defined(_WIN32)
/* Windows TLS */
extern DWORD RWK_THREAD_TYPE_KEY;
#define RWK_THREAD_TYPE ((intptr_t)TlsGetValue(RWK_THREAD_TYPE_KEY))


#else
/* pthread-key */
extern pthread_key_t RWK_THREAD_TYPE_KEY;
#define RWK_THREAD_TYPE ((intptr_t)pthread_getspecific(RWK_THREAD_TYPE_KEY))

#endif

#else
#define RWK_THREAD_TYPE RWK_THREAD_TYPE_NONE

#endif


/* Fibers */
#if _WIN32
#define RWK_FIBER_T LPVOID
#define rwk_set_fiber(cur) 0
#define rwk_exit_fiber(next) SwitchToFiber(next)
#define rwk_swap_fibers(cur, next) SwitchToFiber(next)
#else
#include <setjmp.h>
#define RWK_FIBER_T jmp_buf
#define rwk_set_fiber(cur) setjmp(cur)
#define rwk_exit_fiber(next) longjmp(next, 1)
#define rwk_swap_fibers(cur, next) {if (!setjmp(cur)) longjmp(next, 1);}
#endif


/* Monotonic Timing (1/120 sec tick count) */
#if _WIN32
struct timespec {
    ULONG64 tv_sec;
    long tv_nsec;
};
#define rwk_timing_120ticks() (GetTickCount64() * 120 / 1000)
static INLINE struct timespec rwk_timing_ts() {
    ULONG64 ms = GetTickCount64();
    struct timespec tsc = {ms / 1000, (ms % 1000) * 1000000};
    return tsc;
}
#define rwk_timing_float() (GetTickCount64() / 1000.0)
#elif __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
extern clock_serv_t TIMING_CCLOCK;
static inline uint64_t rwk_timing_120ticks() {
    mach_timespec_t TIMING_MTS;
    clock_get_time(TIMING_CCLOCK, &TIMING_MTS);
    return TIMING_MTS.tv_sec * 120 + TIMING_MTS.tv_nsec * 120 / 1000000000;
}
static inline struct timespec rwk_timing_ts() {
    mach_timespec_t ts;
    clock_get_time(TIMING_CCLOCK, &ts);
    struct timespec tsc = {ts.tv_sec, ts.tv_nsec};
    return tsc;
}
static inline double rwk_timing_float() {
    mach_timespec_t TIMING_MTS;
    clock_get_time(TIMING_CCLOCK, &TIMING_MTS);
    return (double)TIMING_MTS.tv_sec + (double)TIMING_MTS.tv_nsec / 1000000000.0;
}
#else
#include <time.h>
static inline uint64_t rwk_timing_120ticks() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 120 + ts.tv_nsec * 120 / 1000000000;
}
static inline struct timespec rwk_timing_ts() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}
static inline double rwk_timing_float() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}
#endif


/* GameCube SDK style integer types */
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;


#if _WIN32
#include <intrin.h>
#endif

/* Now our swappers */

#define CAST(val,type) (*((type*)(&val)))

#if !defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)
#define __LITTLE_ENDIAN__ 1
#endif

#if defined(__LITTLE_ENDIAN__) || _WIN32

/* Byte swap unsigned short */
static INLINE u16 swap_u16( u16 val )
{
#if __GNUC__
    return __builtin_bswap16(val);
#elif _WIN32
    return _byteswap_ushort(val);
#else
    return (val << 8) | (val >> 8 );
#endif
}

/* Byte swap short */
static INLINE s16 swap_s16( s16 val )
{
#if __GNUC__
    return __builtin_bswap16(val);
#elif _WIN32
    return _byteswap_ushort(val);
#else
    return (val << 8) | ((val >> 8) & 0xFF);
#endif
}

/* Byte swap unsigned int */
#define SWAP_U32(val) ((((((val) << 8) & 0xFF00FF00 ) | (((val) >> 8) & 0xFF00FF )) << 16) | (((((val) << 8) & 0xFF00FF00 ) | (((val) >> 8) & 0xFF00FF )) >> 16))
static INLINE u32 swap_u32( u32 val )
{
#if __GNUC__
    return __builtin_bswap32(val);
#elif _WIN32
    return _byteswap_ulong(val);
#else
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | (val >> 16);
#endif
}

/* Byte swap int */
static INLINE s32 swap_s32( s32 val )
{
#if __GNUC__
    return __builtin_bswap32(val);
#elif _WIN32
    return _byteswap_ulong(val);
#else
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | ((val >> 16) & 0xFFFF);
#endif
}

/* Byte swap 64 int */
static INLINE s64 swap_s64( s64 val )
{
#if __GNUC__
    return __builtin_bswap64(val);
#elif _WIN32
    return _byteswap_uint64(val);
#else
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
#endif
}

/* Byte swap unsigned 64 int */
static INLINE u64 swap_u64( u64 val )
{
#if __GNUC__
    return __builtin_bswap64(val);
#elif _WIN32
    return _byteswap_uint64(val);
#else
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
#endif
}

/* Byte swap float */
static INLINE float swap_float( float val )
{
    u32 uval = swap_u32(CAST(val, u32));
    return CAST(uval, float);
}

/* Byte swap double */
static INLINE double swap_double( double val )
{
    u64 uval = swap_u64(CAST(val, u64));
    return CAST(uval, double);
}

#elif __BIG_ENDIAN__

/* Byte swap unsigned short */
#define swap_u16(val) (val)

/* Byte swap short */
#define swap_s16(val) (val)

/* Byte swap unsigned int */
#define swap_u32(val) (val)
#define swap_U32(val) (val)

/* Byte swap int */
#define swap_s32(val) (val)

/* Byte swap 64 int */
#define swap_s64(val) (val)

/* Byte swap unsigned 64 int */
#define swap_u64(val) (val)

/* Byte swap float */
#define swap_float(val) (val)

/* Byte swap double */
#define swap_double(val) (val)

#endif


#endif
