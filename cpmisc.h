
#ifndef _CPMISC_H
#define _CPMISC_H

#include "cpdltype.h"

#ifdef __cplusplus
extern "C" {
#endif

extern double CPDLAPI cpcpuusage(double* last_idletime, double* last_systemtime);
extern double CPDLAPI cpdiskusage();

#ifdef __cplusplus
}
#endif

#endif /* not _CPMISC_H */
