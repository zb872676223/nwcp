
#ifndef _CPTIMER_H
#define _CPTIMER_H

#include "cpdltype.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int CPDLAPI cptimer_create(const int id, cptimer_t* timer);
extern int CPDLAPI cptimer_set(cptimer_t timer, int ms, cptimer_proc proc, void* arg);
extern int CPDLAPI cptimer_wait(cptimer_t timer);
extern int CPDLAPI cptimer_destroy(cptimer_t* timer);

#ifdef __cplusplus
}
#endif

#endif  /* not _CPTIMER_H */
