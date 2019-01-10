/****************************************************************************
** $Id: cpmemory.c 63 2013-03-27 17:22:25Z cuterhei $
**
** Copyright(c) 2010, cuterhei@gmail.com
** http://www.cp-power.com
**************************************************************************/
#include "cpmemory.h"

void cpcopy(void *dst, void *src, int half_len)
{
#ifdef SP_BIGENDIAN
    __rotate(dst, src, half_len);
#else
    memcpy(dst, src, half_len * 2);
#endif
}
