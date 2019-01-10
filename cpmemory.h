#ifndef _CPMEMORY_H
#define _CPMEMORY_H

#include "cpdltype.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WORDS_BIGENDIAN
extern int CPDLAPI is_big_endian();
//#   define CP_BIGENDIAN is_big_endian()
#elif WORDS_BIGENDIAN
#   define CP_BIGENDIAN 1
#else
#   define CP_BIGENDIAN 0
#endif

inline void cprotate(void* dst, void* src, int half_len)
{
    char* p1 = (char*)src;
    char* p2 = (char*)dst + (half_len * 2 - 1);
    for (int i = 0; i < half_len * 2; i++)
        *p2-- = *p1++;
}

inline void cprotate2(void* dst, void* src)
{
    cprotate(dst, src, 1);
}

inline void cprotate4(void* dst, void* src)
{
    cprotate(dst, src, 2);
}

inline void cprotate8(void* dst, void* src)
{
    cprotate(dst, src, 4);
}

/* 拷贝数据,如果本机为高字节在前,则颠倒字节序拷贝 */
//extern void CPDLAPI cpcopy(void *dst, void *src, int half_len);
/*#ifdef CP_BIGENDIAN
#define cpcopy(dst, src, half_len)  __rotate(dst, src, half_len)
#else
#define cpcopy(dst, src, half_len) memcpy(dst, src, half_len * 2)
#endif*/
inline void cpcopy(void* dst, void* src, int half_len)
{
#ifdef CP_BIGENDIAN
#if CP_BIGENDIAN
    cprotate(dst, src, half_len);
#else
    memcpy(dst, src, half_len * 2);
#endif
#else
    if (is_big_endian())
    {
        cprotate(dst, src, half_len);
    }
    else
    {
        memcpy(dst, src, half_len * 2);
    }
#endif
}

#define cpcopy2(dst, src) cpcopy(dst, src, 1)

#define cpcopy2_allplus(dst, src) \
do { cpcopy2(dst, src); dst = (char *)dst + 2; src = (char *)src + 2; } while(0)

#define cpcopy2_dstplus(dst, src) \
do { cpcopy2(dst, src); dst = (char *)dst + 2; } while(0)

#define cpcopy2_srcplus(dst, src) \
do { cpcopy2(dst, src); src = (char *)src + 2; } while(0)

#define cpcopy4(dst, src) cpcopy(dst, src, 2)

#define cpcopy4_allplus(dst, src) \
do { cpcopy4(dst, src); dst = (char *)dst + 4; src = (char *)src + 4; } while(0)

#define cpcopy4_dstplus(dst, src) \
do { cpcopy4(dst, src); dst = (char *)dst + 4; } while(0)

#define cpcopy4_srcplus(dst, src) \
do { cpcopy4(dst, src); src = (char *)src + 4; } while(0)

#define cpcopy8(dst, src) cpcopy(dst, src, 4)

#define cpcopy8_allplus(dst, src) \
do { cpcopy8(dst, src); dst = (char *)dst + 8; src = (char *)src + 8; } while(0)

#define cpcopy8_dstplus(dst, src) \
do { cpcopy8(dst, src); dst = (char *)dst + 8; } while(0)

#define cpcopy8_srcplus(dst, src) \
do { cpcopy8(dst, src); src = (char *)src + 8; } while(0)

#define cpcopy16(dst, src) cpcopy(dst, src, 8)

#define cpcopy16_allplus(dst, src) \
do { cpcopy16(dst, src); dst = (char *)dst + 16; src = (char *)src + 16; } while(0)

#define cpcopy16_dstplus(dst, src) \
do { cpcopy16(dst, src); dst = (char *)dst + 16; } while(0)

#define cpcopy16_srcplus(dst, src) \
do { cpcopy16(dst, src); src = (char *)src + 16; } while(0)

#define cpmemcpy_allplus(dst, src, len) \
    do { memcpy(dst, src, len); dst = (char *)dst + len; \
    src = (char *)src + len; } while(0)

#define cpmemcpy_dstplus(dst, src, len) \
    do { memcpy(dst, src, len); dst = (char *)dst + len; } while(0)

#define cpmemcpy_srcplus(dst, src, len) \
    do { memcpy(dst, src, len); src = (char *)src + len; } while(0)

#ifdef __cplusplus
}
#endif

#endif
