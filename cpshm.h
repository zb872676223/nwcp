#ifndef _CPSHM_H
#define _CPSHM_H
#include "cpdltype.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int CPDLAPI cpshm_create(const char* shm_id, const unsigned int len, cpshm_t* shm_t);
extern int CPDLAPI cpshm_map(const char* shm_id, cpshm_t* shm_t);
extern int CPDLAPI cpshm_close(cpshm_t* shm_t);
extern int CPDLAPI cpshm_delete(cpshm_t* shm_t);
extern int CPDLAPI cpshm_unmap(cpshm_t shm_t);
extern int CPDLAPI cpshm_data(const cpshm_t shm_t, cpshm_d* pdata, unsigned int* len);
extern int CPDLAPI cpshm_exist(const char* shm_id);

#ifdef __cplusplus
}
#endif


#endif
