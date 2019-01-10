
#ifndef _CPPROCESS_H
#define _CPPROCESS_H

#include "cpdltype.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int CPDLAPI cpprocess_create(char* const argv[], cppid_t* pid);
extern int CPDLAPI cpprocess_create2(const char* cmdline, cppid_t* pid);
extern int CPDLAPI cpprocess_wait(cppid_t pid, int timeout, int* exit_code);
extern int CPDLAPI cpprocess_wait_all(int timeout);
extern void CPDLAPI cpprocess_kill(cppid_t pid);
extern int CPDLAPI cpprocess_alive(cppid_t pid);

#ifdef __cplusplus
}
#endif

#endif
