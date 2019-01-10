
#ifndef _CPTHREAD_H
#define _CPTHREAD_H

#include "cpdltype.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int CPDLAPI cpthread_create(cpthread_t* tid,
                                   CPTHREAD_ROUTINE start_routine, void* arg, int joinable);
extern int CPDLAPI cpthread_join(cpthread_t tid, int* value_ptr);
extern void CPDLAPI cpthread_exit(int value);
extern cpthread_id CPDLAPI cpthread_self();

extern int CPDLAPI cpthread_mutex_init(cpthread_mutex_t* mutex);
extern int CPDLAPI cpthread_mutex_destroy(cpthread_mutex_t* mutex);
extern int CPDLAPI cpthread_mutex_lock(cpthread_mutex_t* mutex);
extern int CPDLAPI cpthread_mutex_trylock(cpthread_mutex_t* mutex);
extern int CPDLAPI cpthread_mutex_unlock(cpthread_mutex_t* mutex);

extern int CPDLAPI cpthread_cond_init(cpthread_cond_t* cond);
extern int CPDLAPI cpthread_cond_destroy(cpthread_cond_t* cond);
extern int CPDLAPI cpthread_cond_wait(cpthread_cond_t* cond,
                                      cpthread_mutex_t* mutex);
extern int CPDLAPI cpthread_cond_timedwait(cpthread_cond_t* cond,
                                           cpthread_mutex_t* mutex,
                                           int ms);
extern int CPDLAPI cpthread_cond_signal(cpthread_cond_t* cond);
extern int CPDLAPI cpthread_cond_broadcast(cpthread_cond_t* cond);

extern void CPDLAPI cpthread_sleep(unsigned int ms);

#ifdef __cplusplus
}
#endif

#endif  /* not _CPTHREAD_H */

