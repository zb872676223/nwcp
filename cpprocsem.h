
#ifndef _CPSEMAPHORE_H
#define _CPSEMAPHORE_H

#include "cpdltype.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int CPDLAPI cpmutex_create(const char* name, cpmutex_t* mutex);
extern int CPDLAPI cpmutex_open(const char* name, cpmutex_t* mutex);
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
extern int CPDLAPI cpmutex_create_ex(char* shm, cpmutex_t* mutex);
extern int CPDLAPI cpmutex_open_ex(char* shm, cpmutex_t* mutex);
#endif
extern int CPDLAPI cpmutex_close(cpmutex_t* mutex);
extern int CPDLAPI cpmutex_trylock(const cpmutex_t mutex);
extern int CPDLAPI cpmutex_lock(const cpmutex_t mutex);
extern int CPDLAPI cpmutex_unlock(const cpmutex_t mutex);

extern int CPDLAPI cpsem_create(const char* name, const unsigned int intval,
                                const unsigned int maxvalue, cpsem_t* sem);
extern int CPDLAPI cpsem_open(const char* name, cpsem_t* sem);
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
extern int CPDLAPI cpsem_create_ex(char* shm, const unsigned int intval,
                                   const unsigned int maxvalue, cpsem_t* sem);
extern int CPDLAPI cpsem_open_ex(char* shm, cpsem_t* sem);
#endif
extern int CPDLAPI cpsem_close(cpsem_t* sem);
extern int CPDLAPI cpsem_wait(const cpsem_t sem, const int timeout);
extern int CPDLAPI cpsem_trywait(const cpsem_t sem);
extern int CPDLAPI cpsem_post(const cpsem_t sem);

// 在timeout时间内等待sem信号，获取sem(-1)成功后，对mutex加锁
extern int CPDLAPI cpmutex_sem_lock(const cpsem_t sem, const cpmutex_t mutex,
                                    const int timeout);
// 释放mutex，发送信号sem(+1)
extern int CPDLAPI cpmutex_sem_unlock(const cpsem_t sem, const cpmutex_t mutex);

#ifdef __cplusplus
}
#endif

#endif /* not _CPSEMAPHORE_H */
