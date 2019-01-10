#include "../cpdltype.h"
#ifndef WORDS_BIGENDIAN
int CPDLAPI is_big_endian()
{
    static int cpone = 1;
    return (*(char *)(&cpone)==0);
}
#endif
