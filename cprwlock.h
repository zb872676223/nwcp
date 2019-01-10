
#ifndef _CPRWLOCK_H
#define _CPRWLOCK_H

#include "cpdltype.h"

#ifdef __cplusplus
extern "C" {
#endif


extern int CPDLAPI cprwlock_create(cprwlock_t* rwl);
extern int CPDLAPI cprwlock_close(cprwlock_t* rwl);

extern int CPDLAPI cprwlock_wlock(cprwlock_t rwl, const int timeout);
extern int CPDLAPI cprwlock_trywlock(cprwlock_t rwl);
extern int CPDLAPI cprwlock_wunlock(cprwlock_t rwl);
extern int CPDLAPI cprwlock_rlock(cprwlock_t rwl, const int timeout);
extern int CPDLAPI cprwlock_tryrlock(cprwlock_t rwl);
extern int CPDLAPI cprwlock_runlock(cprwlock_t rwl);


extern int CPDLAPI cpprwlock_create(const char* name, cpprwlock_t* rwl);
extern int CPDLAPI cpprwlock_open(const char* name, cpprwlock_t* rwl);
extern int CPDLAPI cpprwlock_close(cpprwlock_t* rwl);

extern int CPDLAPI cpprwlock_wlock(cpprwlock_t rwl, const int timeout);
extern int CPDLAPI cpprwlock_trywlock(cpprwlock_t rwl);
extern int CPDLAPI cpprwlock_wunlock(cpprwlock_t rwl);
extern int CPDLAPI cpprwlock_rlock(cpprwlock_t rwl, const int timeout);
extern int CPDLAPI cpprwlock_tryrlock(cpprwlock_t rwl);
extern int CPDLAPI cpprwlock_runlock(cpprwlock_t rwl);


#ifdef __cplusplus
}
#endif

#endif  /* not _CPRWLOCK_H */

