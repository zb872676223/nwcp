#ifndef _CPDLL_H
#define _CPDLL_H

#include "cpdltype.h"

#ifdef __cplusplus
extern "C" {
#endif

extern cpdll_t CPDLAPI cpdll_open(const char* filename);
extern cpdlsym_t CPDLAPI cpdll_sym(cpdll_t dll_t, const char* sym_name);
extern void CPDLAPI cpdll_close(cpdll_t dll_t);

#ifdef __cplusplus
}
#endif

#endif /* not _CPDL_H */
