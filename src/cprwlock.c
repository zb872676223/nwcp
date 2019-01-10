
#include "cprwlock.h"

#ifdef _NWCP_WIN32
//#include <process.h>
#else   /* UNIX */
#    include <stdlib.h>
#    include <string.h>
#    include <unistd.h>
#   ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED/*_POSIX_THREAD_PROCESS_SHARED*/
#    include <pthread.h>
#  else
#    include <errno.h>
#    include <sys/ipc.h>
#    include <sys/sem.h>
#       if defined(_SEM_SEMUN_UNDEFINED) || defined(_NWCP_SUNOS)
union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                           (Linux-specific) */
};
#   endif
#endif
#endif  /* _NWCP_WIN32 */


/*! \file  cprwlock.h
    \brief 读写锁
*/

/*! \addtogroup CPRWLOCK 读写锁接口 */
/*@{*/

static const char *CPPRWLOCK_FLGSHM_FIX = "cprwl_";
#ifdef _NWCP_WIN32 /* windows */
struct tagCPPRWLOCKFLAG
{
    unsigned int  readers;
    unsigned int  writers;
    unsigned char rlocked;
    unsigned char wlocked;
    unsigned char _pack2;
    unsigned char _pack1;
};
static const char *CPPRWLOCK_MUTEXL_FIX = "cpprwl_l_";
static const char *CPPRWLOCK_REVENT_FIX = "cpprwl_r_";
static const char *CPPRWLOCK_WEVENT_FIX = "cpprwl_w_";
struct tagCPPRWLOCKEVT
{
    HANDLE lock;
    HANDLE revent;
    HANDLE wevent;
};

struct tagCPRWLOCK
{
    struct tagCPPRWLOCKEVT event;
    struct tagCPPRWLOCKFLAG *flag;
};
struct tagCPPSRWLOCK
{
    cpshm_t shm_id;
    struct tagCPRWLOCK rwl;
    char    create;
};
#elif  defined(_CPDL_USE_POSIX_THREAD_PROCESS_SHARED)
struct tagCPPSRWLOCK
{
    cpshm_t shm_id;
    pthread_rwlock_t *rwl;
    char    create;
};
#else
enum cpprw_esc_t { cpprw_et_lock=0, cpprw_s_rlock, cpprw_s_wlock, cpprw_c_reader, cpprw_c_writer };
#ifdef _CPDL_PPRW_USE_SHM_COUNTER
struct tagCPPRWLOCKFLAG
{
    unsigned int  readers;
    unsigned int  writers;
};
struct tagCPPSRWLOCK
{
    cpshm_t shm_id;
    int *event; /* cpprw_esc_t */
    struct tagCPPRWLOCKFLAG *flag;
    char    create;
};
#else
struct tagCPPSRWLOCK
{
    int event; /* cpprw_esc_t */
    char    create;
};
#endif
#endif


#ifdef _NWCP_WIN32
/*inline*/ int __cpprwlock_create_event(struct tagCPPRWLOCKEVT *evt, const char *ml_name, const char *re_name, const char *we_name)
{
    evt->lock = CreateMutex(NULL, FALSE, ml_name);
    if(evt->lock == NULL)
        return CPDL_ERROR;
    evt->revent = CreateEvent(NULL, TRUE, TRUE, re_name);
    if(evt->revent == NULL)
    {
        CloseHandle(evt->lock);
        return CPDL_ERROR;
    }
    evt->wevent = CreateEvent(NULL, FALSE, TRUE, we_name);
    if(evt->wevent == NULL)
    {
        CloseHandle(evt->revent);
        CloseHandle(evt->lock);
        return CPDL_ERROR;
    }
    return CPDL_SUCCESS;
}

/*inline*/ int __cpprwlock_open_event(struct tagCPPRWLOCKEVT *evt, const char *ml_name, const char *re_name, const char *we_name)
{
    evt->lock = OpenMutex(MUTEX_ALL_ACCESS, FALSE, ml_name);
    if(evt->lock == NULL)
        return CPDL_ERROR;
    evt->revent = OpenEvent(EVENT_ALL_ACCESS, FALSE, re_name);
    if(evt->revent == NULL)
    {
        CloseHandle(evt->lock);
        return CPDL_ERROR;
    }
    evt->wevent = OpenEvent(EVENT_ALL_ACCESS, FALSE, we_name);
    if(evt->wevent == NULL)
    {
        CloseHandle(evt->revent);
        CloseHandle(evt->lock);
        return CPDL_ERROR;
    }
    return CPDL_SUCCESS;
}

/*inline*/ int __cpprwlock_wlock(struct tagCPRWLOCK *rwl, const int timeout)
{
    //HANDLE h2[2] = {rwl->event.lock, rwl->event.wevent};
    if(WAIT_OBJECT_0 != WaitForSingleObject(rwl->event.lock, INFINITE))
        return CPDL_ERROR;
    if(++rwl->flag->writers == 1)
    {
        rwl->flag->wlocked = 1;
        ResetEvent(rwl->event.revent);
        if(0==rwl->flag->rlocked)
        {
            ResetEvent(rwl->event.wevent);
            ReleaseMutex(rwl->event.lock);
            return CPDL_SUCCESS;
        }
    }
    ReleaseMutex(rwl->event.lock);

    if(WAIT_OBJECT_0==WaitForSingleObject(rwl->event.wevent, (DWORD)timeout))
        return CPDL_SUCCESS;
    WaitForSingleObject(rwl->event.lock, INFINITE);
    if(--rwl->flag->writers == 0)
    {
        rwl->flag->wlocked = 0;
        SetEvent(rwl->event.revent);
    }
    ReleaseMutex(rwl->event.lock);
    return CPDL_ERROR;
}

/*inline*/ int __cpprwlock_trywlock(struct tagCPRWLOCK *rwl)
{
    HANDLE h2[2] = {rwl->event.lock, rwl->event.wevent};
    switch (WaitForMultipleObjects(2, h2, TRUE, 0))
    {
    case WAIT_OBJECT_0:
    case WAIT_OBJECT_0 + 1:
        if(++rwl->flag->writers == 1)
        {
            rwl->flag->wlocked = 1;
            ResetEvent(rwl->event.revent);
        }
        ReleaseMutex(rwl->event.lock);
        return CPDL_SUCCESS;
    default:
        break;
    }
    return CPDL_ERROR;
}

/*inline*/ int __cpprwlock_wunlock(struct tagCPRWLOCK *rwl)
{
    if(WAIT_OBJECT_0 != WaitForSingleObject(rwl->event.lock, INFINITE))
        return  CPDL_ERROR;
    if(--rwl->flag->writers == 0)
    {
        rwl->flag->wlocked = 0;
        SetEvent(rwl->event.revent);
    }
    SetEvent(rwl->event.wevent);
    ReleaseMutex(rwl->event.lock);
    return CPDL_SUCCESS;
}

/*inline*/ int __cpprwlock_rlock(struct tagCPRWLOCK *rwl, const unsigned int timeout)
{
    HANDLE h2[2] = { rwl->event.lock, rwl->event.revent };
    switch (WaitForMultipleObjects(2, h2, TRUE, timeout))
    {
    case WAIT_OBJECT_0:
    case WAIT_OBJECT_0 + 1:
        if(++rwl->flag->readers == 1)
        {
            rwl->flag->rlocked = 1;
            ResetEvent(rwl->event.wevent);
        }
        ReleaseMutex(rwl->event.lock);
        return CPDL_SUCCESS;
    default:
        break;
    }
    return CPDL_ERROR;
}

/*inline*/ int __cpprwlock_runlock(struct tagCPRWLOCK *rwl)
{
    if (WAIT_OBJECT_0 != WaitForSingleObject(rwl->event.lock, INFINITE))
        return CPDL_ERROR;
    if(--rwl->flag->readers == 0) // 没有等待写的任务
    {
        rwl->flag->rlocked =  0; // 清写标志
        SetEvent(rwl->event.wevent);
    }
    ReleaseMutex(rwl->event.lock);
    return CPDL_SUCCESS;
}
#endif



/*! \brief 创建读写锁对象
    \param rwl - 读写锁实例对象的地址
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_destroy
*/
int cprwlock_create(cprwlock_t *rwl)
{
#ifdef _NWCP_WIN32
    /*
#   ifdef HAVE_NT0600_SRWLOCK
    *rwl = (SRWLOCK*)malloc(sizeof(SRWLOCK));
    InitializeSRWLock(*rwl);
#   else
*/
    struct tagCPPRWLOCKEVT event;
    //memset(&srwl, 0, sizeof(struct tagCPRWLOCK));
    if(CPDL_SUCCESS != __cpprwlock_create_event(&event, (const char*)0, (const char*)0, (const char*)0))
        return CPDL_ERROR;
    *rwl = (struct tagCPRWLOCK*)malloc(sizeof(struct tagCPRWLOCK));
    (*rwl)->flag = (struct tagCPPRWLOCKFLAG*)malloc(sizeof(struct tagCPPRWLOCKFLAG));
    memset((*rwl)->flag, 0, sizeof(struct tagCPPRWLOCKFLAG));
    memcpy(&(*rwl)->event, &event, sizeof(struct tagCPPRWLOCKEVT));
    /*
#   endif
*/
#else
    pthread_rwlock_t prw;
    if(0 != pthread_rwlock_init(&prw, 0))
        return CPDL_ERROR;
    *rwl = (pthread_rwlock_t*)malloc(sizeof(pthread_rwlock_t));
    memcpy(*rwl, &prw, sizeof(pthread_rwlock_t));
#endif
    return CPDL_SUCCESS;
}

/*! \brief 销毁读写锁对象
    \param rwl -读写锁实例对象的地址
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_create
*/
int cprwlock_close(cprwlock_t *rwl)
{
#ifdef _NWCP_WIN32
    /*
#   ifdef HAVE_NT0600_SRWLOCK
    free(*rwl);
    return CPDL_SUCCESS;
#   else
*/
    CloseHandle((*rwl)->event.wevent);
    CloseHandle((*rwl)->event.revent);
    CloseHandle((*rwl)->event.lock);
    free((*rwl)->flag);
    free(*rwl);
    return CPDL_SUCCESS;
    /*
#   endif
*/
#else
    if( 0 == pthread_rwlock_destroy(*rwl))
    {
        free(*rwl);
        return CPDL_SUCCESS;
    }
    return CPDL_ERROR;
#endif
}

/*! \brief 获取写锁(带超时)
    \param rwl - 读写锁实例对象
    \param timeout - 超时
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_wunlock
*/
int cprwlock_wlock(cprwlock_t rwl, const int timeout)
{
#ifdef _NWCP_WIN32
    /*
#   ifdef HAVE_NT0600_SRWLOCK
    AcquireSRWLockExclusive(rwl);
    return CPDL_SUCCESS;
#   else
*/
    return __cpprwlock_wlock(rwl, timeout);
    /*
#   endif
*/
#else
    struct timespec ts;
    if( timeout < 0)
        return ( 0 == pthread_rwlock_wrlock(rwl) ? CPDL_SUCCESS : CPDL_ERROR );
    else if( 0 == timeout)
        return ( 0 == pthread_rwlock_trywrlock(rwl) ? CPDL_SUCCESS : CPDL_ERROR );
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += ((timeout % 1000) * 1000 * 1000);
    ts.tv_sec +=  ((long)(ts.tv_nsec / 1000 / 1000 / 1000) + (long)(timeout / 1000));
    ts.tv_nsec %= 1000 * 1000 * 1000;
    return ( 0 == pthread_rwlock_timedwrlock(rwl, &ts) ? CPDL_SUCCESS : CPDL_ERROR );
#endif
}

/*! \brief 获取写锁(非堵塞)
    \param rwl - 读写锁实例对象
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_wunlock
*/
int cprwlock_trywlock(cprwlock_t rwl)
{
#ifdef _NWCP_WIN32
    /*
#   ifdef HAVE_NT0600_SRWLOCK
    return ( (0 != TryAcquireSRWLockExclusive(rwl)) ? CPDL_SUCCESS : CPDL_ERROR);
#   else
*/
    return __cpprwlock_trywlock(rwl);
    /*
#   endif
*/
#else
    return ( 0 == pthread_rwlock_trywrlock(rwl) ? CPDL_SUCCESS : CPDL_ERROR );
#endif
    return CPDL_ERROR;
}

/*! \brief 释放写锁(非堵塞)
    \param rwl - 读写锁实例对象
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_wlock, ::cprwlock_trywlock
*/
int cprwlock_wunlock(cprwlock_t rwl)
{
#ifdef _NWCP_WIN32
    /*
#   ifdef HAVE_NT0600_SRWLOCK
    ReleaseSRWLockExclusive(rwl);
#   else
*/
    return __cpprwlock_wunlock(rwl);
    /*
#   endif
*/
#else
    return ( 0 == pthread_rwlock_unlock(rwl)) ? CPDL_SUCCESS : CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

/*! \brief 获取读锁(带超时)
    \param rwl - 读写锁实例对象
    \param timeout - 超时
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_runlock
*/
int cprwlock_rlock(cprwlock_t rwl, const int timeout)
{
#ifdef _NWCP_WIN32
    /*
#   ifdef HAVE_NT0600_SRWLOCK
    AcquireSRWLockShared(rwl);
    return CPDL_SUCCESS;
#   else
*/
    return __cpprwlock_rlock(rwl, timeout);
    /*
#   endif
*/
#else
    struct timespec ts;
    if( timeout < 0)
        return ( 0 == pthread_rwlock_wrlock(rwl) ? CPDL_SUCCESS : CPDL_ERROR );
    else if( 0 == timeout)
        return ( 0 == pthread_rwlock_trywrlock(rwl) ? CPDL_SUCCESS : CPDL_ERROR );
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += ((timeout % 1000) * 1000 * 1000);
    ts.tv_sec +=  ((long)(ts.tv_nsec / 1000 / 1000 / 1000) + (long)(timeout / 1000));
    ts.tv_nsec %= 1000 * 1000 * 1000;
    return ( 0 == pthread_rwlock_timedrdlock(rwl, &ts) ? CPDL_SUCCESS : CPDL_ERROR );
#endif
}

/*! \brief 获取读锁(非堵塞)
    \param rwl - 读写锁实例对象
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_runlock, ::cprwlock_rlock
*/
int cprwlock_tryrlock(cprwlock_t rwl)
{
#ifdef _NWCP_WIN32
    /*
#   ifdef HAVE_NT0600_SRWLOCK
    return ( (0!=TryAcquireSRWLockShared(rwl)) ? CPDL_SUCCESS : CPDL_ERROR );
#   else
*/
    return __cpprwlock_rlock(rwl, 0);
    /*
#   endif
*/
#else
    return ( 0 == pthread_rwlock_tryrdlock(rwl) ? CPDL_SUCCESS : CPDL_ERROR );
#endif
}

/*! \brief 释放读锁
    \param rwl - 读写锁实例对象
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_rlock, ::cprwlock_tryrlock
*/
int cprwlock_runlock(cprwlock_t rwl)
{
#ifdef _NWCP_WIN32
    /*
#   ifdef HAVE_NT0600_SRWLOCK
    ReleaseSRWLockShared(rwl);
    return CPDL_SUCCESS;
#   else
*/
    return __cpprwlock_runlock(rwl);
    /*
#   endif
*/
#else
    return ( 0 == pthread_rwlock_unlock(rwl) ? CPDL_SUCCESS : CPDL_ERROR );
#endif
}


extern int cpshm_create(const char *shm_id, const unsigned int len, cpshm_t *shm_t);
extern int cpshm_map(const char *shm_id, cpshm_t *shm_t);
extern int cpshm_data(const cpshm_t shm_t, cpshm_d *pdata, unsigned int *len);
extern int cpshm_close(cpshm_t *shm_t);
#ifndef _NWCP_WIN32
extern void __get_ipc_name(const char *name, char *ipcname, const char *op);
extern key_t __tok_ipc_file(const char *ipcname, int id);
#endif

/*! \brief 创建口具名读写锁对象
    \param rwl - 对象锁实例对象的地址
    \param name - 对象锁实例对象名称
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cpprwlock_destroy
*/
int cpprwlock_create(const char *name, cpprwlock_t *rwl)
{
#ifdef _NWCP_WIN32
    char *rw_fshm = 0, *ml_name = 0, *re_name = 0, *we_name = 0;
    int ret = 0;
    struct tagCPPSRWLOCK srwl;
    memset(&srwl, 0, sizeof(struct tagCPPSRWLOCK));
    if(0==name || (ret=strlen(name)) == 0)
        return CPDL_ERROR;

    rw_fshm = (char*)malloc(strlen(CPPRWLOCK_FLGSHM_FIX) + ret + 1);
    strcpy(rw_fshm, CPPRWLOCK_FLGSHM_FIX); strcat(rw_fshm, name);

    if( CPDL_SUCCESS != cpshm_map(rw_fshm, &srwl.shm_id)/*cpshm_exist(strName)*/ )
    {
        if(CPDL_SUCCESS!=cpshm_create(rw_fshm, sizeof(struct tagCPPRWLOCKFLAG), &srwl.shm_id))
        {
            free(rw_fshm);
            return CPDL_ERROR;
        }
    }
    free(rw_fshm);

    if( CPDL_SUCCESS != cpshm_data(srwl.shm_id, (cpshm_d*)(&srwl.rwl.flag), 0) )
    {
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }

    ml_name = (char*)malloc(strlen(CPPRWLOCK_MUTEXL_FIX) + ret + 1);
    re_name = (char*)malloc(strlen(CPPRWLOCK_REVENT_FIX) + ret + 1);
    we_name = (char*)malloc(strlen(CPPRWLOCK_WEVENT_FIX) + ret + 1);
    strcpy(ml_name, CPPRWLOCK_MUTEXL_FIX); strcat(ml_name, name);
    strcpy(re_name, CPPRWLOCK_REVENT_FIX); strcat(re_name, name);
    strcpy(we_name, CPPRWLOCK_WEVENT_FIX); strcat(we_name, name);
    ret = __cpprwlock_create_event(&srwl.rwl.event, ml_name, re_name, we_name);
    free(we_name);
    free(re_name);
    free(ml_name);

    if(0!=ret)
    {
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }

    memset(srwl.rwl.flag, 0, sizeof(struct tagCPPRWLOCKFLAG));
    *rwl = (struct tagCPPSRWLOCK*)malloc(sizeof(struct tagCPPSRWLOCK));
    memcpy(*rwl, &srwl, sizeof(struct tagCPPSRWLOCK));
#elif defined(_CPDL_USE_POSIX_THREAD_PROCESS_SHARED)
    int ret = 0;
    char *rw_fshm = 0;
    pthread_rwlockattr_t attr;
    struct tagCPPSRWLOCK srwl;
    if(0==name || (ret=strlen(name)) == 0)
        return CPDL_ERROR;

    rw_fshm = (char*)malloc(strlen(CPPRWLOCK_FLGSHM_FIX) + ret + 1);
    strcpy(rw_fshm, CPPRWLOCK_FLGSHM_FIX); strcat(rw_fshm, name);
    memset(&srwl, 0, sizeof(struct tagCPPSRWLOCK));

    if( CPDL_SUCCESS != cpshm_map(rw_fshm, &srwl.shm_id))
    {
        if(CPDL_SUCCESS!=cpshm_create(rw_fshm, sizeof(pthread_rwlock_t), &srwl.shm_id))
        {
            free(rw_fshm);
            return CPDL_ERROR;
        }
        srwl.create = 1;
    }
    free(rw_fshm);

    if( CPDL_SUCCESS != cpshm_data(srwl.shm_id, (cpshm_d*)(&srwl.rwl), 0) )
    {
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }

    if(pthread_rwlockattr_init(&attr))
    {
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }
    if(pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED))
    {
        pthread_rwlockattr_destroy(&attr);
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }
    if(pthread_rwlock_init(srwl.rwl, &attr))
    {
        pthread_rwlockattr_destroy(&attr);
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }
    pthread_rwlockattr_destroy(&attr);

    *rwl = (struct tagCPPSRWLOCK*)malloc(sizeof(struct tagCPPSRWLOCK));
    memcpy(*rwl, &srwl, sizeof(struct tagCPPSRWLOCK));
#elif defined(_CPDL_PPRW_USE_SHM_COUNTER)
    char *ipcname = 0; // *ml_name = 0, *re_name = 0, *we_name = 0;
    key_t key;
    cpshm_d data = 0;
    int ret = 0;
    unsigned short values[3];
    union semun arg;
    struct tagCPPSRWLOCK srwl;
    if(CPDL_SUCCESS == cpprwlock_open(name, rwl))
    {
        return CPDL_SUCCESS;
    }
    if(0==name || (ret=strlen(name)) == 0)
        return CPDL_ERROR;
    ipcname = (char*)malloc(256);
    __get_ipc_name(name, ipcname, CPPRWLOCK_FLGSHM_FIX);
    key = __tok_ipc_file(ipcname, 0);
    if (key < 0)
    {
        free(ipcname);
        return CPDL_ERROR;
    }

    memset(&srwl, 0, sizeof(struct tagCPPSRWLOCK));
    ipcname = (char*)realloc(ipcname, strlen(CPPRWLOCK_FLGSHM_FIX) + ret + 1);
    strcpy(ipcname, CPPRWLOCK_FLGSHM_FIX); strcat(ipcname, name);
    if(CPDL_SUCCESS!=cpshm_create(ipcname, sizeof(int)+sizeof(struct tagCPPRWLOCKFLAG), &srwl.shm_id))
    {
        free(ipcname);
        return CPDL_ERROR;
    }
    srwl.create = 1;
    free(ipcname);

    if( CPDL_SUCCESS != cpshm_data(srwl.shm_id, &data, 0) )
    {
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }
    srwl.event = (int*)(data);
    srwl.flag = (struct tagCPPRWLOCKFLAG*)(data + sizeof(int));
    *(srwl.event) = semget(key, 3, IPC_CREAT|IPC_EXCL | 0666);
    if(*(srwl.event) == -1)
    {
        cpshm_close( &srwl.shm_id);
        return CPDL_ERROR;
    }
    values[0]=1;
    values[1]=1;
    values[2]=1;
    arg.array = values;
    if(semctl(*(srwl.event), 3, SETALL, arg)==-1)
    {
        semctl(*(srwl.event), 0, IPC_RMID);
        cpshm_close( &srwl.shm_id);
        return CPDL_ERROR;
    }
    memset(srwl.flag, 0, sizeof(struct tagCPPRWLOCKFLAG));
    *rwl = (struct tagCPPSRWLOCK*)malloc(sizeof(struct tagCPPSRWLOCK));
    memcpy(*rwl, &srwl, sizeof(struct tagCPPSRWLOCK));
#else
    char *ipcname = 0; // *ml_name = 0, *re_name = 0, *we_name = 0;
    key_t key;
    int ret = 0;
    unsigned short values[5];
    union semun arg;
    struct tagCPPSRWLOCK srwl;
    if(CPDL_SUCCESS == cpprwlock_open(name, rwl))
    {
        return CPDL_SUCCESS;
    }
    if(0==name || (ret=strlen(name)) == 0)
        return CPDL_ERROR;
    ipcname = (char*)malloc(256);
    __get_ipc_name(name, ipcname, CPPRWLOCK_FLGSHM_FIX);
    key = __tok_ipc_file(ipcname, 0);
    free(ipcname);
    if (key < 0)
    {
        return CPDL_ERROR;
    }

    srwl.event = semget(key, 5, IPC_CREAT|IPC_EXCL | 0666);
    if(srwl.event == -1)
        return CPDL_ERROR;
    srwl.create = 1;
    values[cpprw_et_lock]=1;
    values[cpprw_s_rlock]=1;
    values[cpprw_s_wlock]=1;
    values[cpprw_c_reader]=0;
    values[cpprw_c_writer]=0;
    arg.array = values;
    if(semctl(srwl.event, 5, SETALL, arg)==-1)
    {
        semctl(srwl.event, 0, IPC_RMID);
        return CPDL_ERROR;
    }
    *rwl = (struct tagCPPSRWLOCK*)malloc(sizeof(struct tagCPPSRWLOCK));
    memcpy(*rwl, &srwl, sizeof(struct tagCPPSRWLOCK));
#endif
    return CPDL_SUCCESS;
}

/*! \brief 创建口具名读写锁对象
    \param rwl - 对象锁实例对象的地址
    \param name - 对象锁实例对象名称
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cpprwlock_destroy
*/
int cpprwlock_open(const char *name, cpprwlock_t *rwl)
{
#ifdef _NWCP_WIN32
    char *rw_fshm = 0, *ml_name = 0, *re_name = 0, *we_name = 0;
    int ret = 0;
    struct tagCPPSRWLOCK srwl;
    memset(&srwl, 0, sizeof(struct tagCPPSRWLOCK));
    if(0==name || (ret=strlen(name)) == 0)
        return CPDL_ERROR;

    rw_fshm = (char*)malloc(strlen(CPPRWLOCK_FLGSHM_FIX) + ret + 1);
    strcpy(rw_fshm, CPPRWLOCK_FLGSHM_FIX); strcat(rw_fshm, name);

    if( CPDL_SUCCESS != cpshm_map(rw_fshm, &srwl.shm_id))
    {
        free(rw_fshm);
        return CPDL_ERROR;
    }
    free(rw_fshm);

    if( CPDL_SUCCESS != cpshm_data(srwl.shm_id, (cpshm_d*)(&srwl.rwl.flag), 0) )
    {
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }

    ml_name = (char*)malloc(strlen(CPPRWLOCK_MUTEXL_FIX) + ret + 1);
    re_name = (char*)malloc(strlen(CPPRWLOCK_REVENT_FIX) + ret + 1);
    we_name = (char*)malloc(strlen(CPPRWLOCK_WEVENT_FIX) + ret + 1);
    strcpy(ml_name, CPPRWLOCK_MUTEXL_FIX); strcat(ml_name, name);
    strcpy(re_name, CPPRWLOCK_REVENT_FIX); strcat(re_name, name);
    strcpy(we_name, CPPRWLOCK_WEVENT_FIX); strcat(we_name, name);
    ret = __cpprwlock_open_event(&srwl.rwl.event, ml_name, re_name, we_name);
    free(we_name);
    free(re_name);
    free(ml_name);

    if(0!=ret)
    {
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }
    memset(srwl.rwl.flag, 0, sizeof(struct tagCPPRWLOCKFLAG));
    *rwl = (struct tagCPPSRWLOCK*)malloc(sizeof(struct tagCPPSRWLOCK));
    memcpy(*rwl, &srwl, sizeof(struct tagCPPSRWLOCK));
#elif defined(_CPDL_USE_POSIX_THREAD_PROCESS_SHARED)
    int ret = 0;
    char *rw_fshm = 0;
    pthread_rwlockattr_t attr;
    struct tagCPPSRWLOCK srwl;
    if(0==name || (ret=strlen(name)) == 0)
        return CPDL_ERROR;

    rw_fshm = (char*)malloc(strlen(CPPRWLOCK_FLGSHM_FIX) + ret + 1);
    strcpy(rw_fshm, CPPRWLOCK_FLGSHM_FIX); strcat(rw_fshm, name);
    memset(&srwl, 0, sizeof(struct tagCPPSRWLOCK));

    if( CPDL_SUCCESS != cpshm_map(rw_fshm, &srwl.shm_id))
    {
        free(rw_fshm);
        return CPDL_ERROR;
    }
    free(rw_fshm);

    if( CPDL_SUCCESS != cpshm_data(srwl.shm_id, (cpshm_d*)(&srwl.rwl), 0) )
    {
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }

    if(pthread_rwlockattr_init(&attr))
    {
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }
    if(pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED))
    {
        pthread_rwlockattr_destroy(&attr);
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }
    if(pthread_rwlock_init(srwl.rwl, &attr))
    {
        pthread_rwlockattr_destroy(&attr);
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }
    pthread_rwlockattr_destroy(&attr);

    *rwl = (struct tagCPPSRWLOCK*)malloc(sizeof(struct tagCPPSRWLOCK));
    memcpy(*rwl, &srwl, sizeof(struct tagCPPSRWLOCK));
#elif defined(_CPDL_PPRW_USE_SHM_COUNTER)
    char *ipcname = 0;
    cpshm_d data = 0;
    int ret = 0;
    struct tagCPPSRWLOCK srwl;

    if(0==name || (ret=strlen(name)) == 0)
        return CPDL_ERROR;

    memset(&srwl, 0, sizeof(struct tagCPPSRWLOCK));
    ipcname = (char*)realloc(ipcname, strlen(CPPRWLOCK_FLGSHM_FIX) + ret + 1);
    strcpy(ipcname, CPPRWLOCK_FLGSHM_FIX); strcat(ipcname, name);
    if(CPDL_SUCCESS!=cpshm_map(ipcname, &srwl.shm_id))
    {
        free(ipcname);
        return CPDL_ERROR;
    }
    free(ipcname);

    if( CPDL_SUCCESS != cpshm_data(srwl.shm_id, &data, 0) )
    {
        cpshm_close(&srwl.shm_id);
        return CPDL_ERROR;
    }
    srwl.event = (int*)(data);
    srwl.flag = (struct tagCPPRWLOCKFLAG*)(data + sizeof(int));
    *rwl = (struct tagCPPSRWLOCK*)malloc(sizeof(struct tagCPPSRWLOCK));
    memcpy(*rwl, &srwl, sizeof(struct tagCPPSRWLOCK));
#else
    char *ipcname = 0;
    key_t key;
    int ret = 0;
    struct tagCPPSRWLOCK srwl;
    if(0==name || (ret=strlen(name)) == 0)
        return CPDL_ERROR;
    ipcname = (char*)malloc(256);
    __get_ipc_name(name, ipcname, CPPRWLOCK_FLGSHM_FIX);
    key = __tok_ipc_file(ipcname, 0);
    free(ipcname);
    if (key < 0)
    {
        return CPDL_ERROR;
    }
    srwl.event = semget(key, 0, 0);
    if(srwl.event == -1)
        return CPDL_ERROR;
    srwl.create = 0;
    *rwl = (struct tagCPPSRWLOCK*)malloc(sizeof(struct tagCPPSRWLOCK));
    memcpy(*rwl, &srwl, sizeof(struct tagCPPSRWLOCK));
#endif
    return CPDL_SUCCESS;
}


/*! \brief 销毁具名读写锁对象
    \param rwl - 读写锁实例对象的地址
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_create
*/
int cpprwlock_close(cpprwlock_t *rwl)
{
#ifdef _NWCP_WIN32
    CloseHandle((*rwl)->rwl.event.wevent);
    CloseHandle((*rwl)->rwl.event.revent);
    CloseHandle((*rwl)->rwl.event.lock);
    cpshm_close(&(*rwl)->shm_id);
#elif defined(_CPDL_USE_POSIX_THREAD_PROCESS_SHARED)
    pthread_rwlock_destroy((*rwl)->rwl);
    if (CPDL_EINVAL == cpshm_close( &(*rwl)->shm_id ))
    {
    }
#elif defined(_CPDL_PPRW_USE_SHM_COUNTER)
    int semid = *((*rwl)->event);
    char create = (*rwl)->create;
    cpshm_t  shm_t = (*rwl)->shm_id;
    free((void*)(*rwl));
    *rwl = 0;
    if (CPDL_SUCCESS == cpshm_close( &shm_t ))
    {
        if( 1==create && semid != -1 )
            semctl(semid, 3, IPC_RMID);
    }
#else
    int semid = (*rwl)->event;
    char create = (*rwl)->create;
    free((void*)(*rwl));
    *rwl = 0;
    if( 1==create && semid != -1 )
        semctl(semid, 0, IPC_RMID);
#endif
    free(*rwl);
    return CPDL_SUCCESS;
}

#if !defined( _NWCP_WIN32) && !defined(_CPDL_USE_POSIX_THREAD_PROCESS_SHARED)
#if defined(_NWCP_SUNOS) && defined(__SUNPRO_C)
int __rwlock_sem_op(int sem, unsigned short num, short opval, short flg)
#else
inline int __rwlock_sem_op(int sem, unsigned short num, short opval, short flg)
#endif
{
    struct sembuf op[1];
    int rc;
    op[0].sem_num = num;
    op[0].sem_op = opval;
    op[0].sem_flg = flg;
    do {
        rc = semop(sem,op,1);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0)
    {
        return CPDL_ERROR;
    }
    return CPDL_SUCCESS;
}

#if defined(_NWCP_SUNOS) && defined(__SUNPRO_C)
int __rwlock_sem_opov(int sem, unsigned short num, short opval, short flg, int ovt)
#else
inline int __rwlock_sem_opov(int sem, unsigned short num, short opval, short flg, int ovt)
#endif
{
    struct sembuf op[1];
    struct timespec ts;
    int rc = -1;
    op[0].sem_num = num;
    op[0].sem_op = opval;
    op[0].sem_flg = flg;

    ts.tv_sec = ovt / 1000;
    ts.tv_nsec = (ovt % 1000) * 1000;
    do {
        rc = semtimedop(sem, op, 1, &ts);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0)
    {
        return CPDL_ERROR;
    }
    return CPDL_SUCCESS;
}

#if defined(_NWCP_SUNOS) && defined(__SUNPRO_C)
int __rwlock_sem_op2ov(int sem, unsigned short num[2], short opval[2], short flg[2], int ovt)
#else
inline int __rwlock_sem_op2ov(int sem, unsigned short num[2], short opval[2], short flg[2], int ovt)
#endif
{
    struct sembuf op[2];
    struct timespec ts;
    int rc;
    op[0].sem_num = num[0];
    op[0].sem_op = opval[0];
    op[0].sem_flg = flg[0];
    op[1].sem_num = num[1];
    op[1].sem_op = opval[1];
    op[1].sem_flg = flg[1];
    ts.tv_sec = ovt / 1000;
    ts.tv_nsec = (ovt % 1000) * 1000;
    do {
        rc = semtimedop(sem,op,2, &ts);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0)
    {
        return CPDL_ERROR;
    }
    return CPDL_SUCCESS;
}

#if defined(_NWCP_SUNOS) && defined(__SUNPRO_C)
int __rwlock_sem_op2(int sem, unsigned short num[2], short opval[2], short flg[2])
#else
inline int __rwlock_sem_op2(int sem, unsigned short num[2], short opval[2], short flg[2])
#endif
{
    struct sembuf op[2];
    int rc;
    op[0].sem_num = num[0];
    op[0].sem_op = opval[0];
    op[0].sem_flg = flg[0];
    op[1].sem_num = num[1];
    op[1].sem_op = opval[1];
    op[1].sem_flg = flg[1];
    do {
        rc = semop(sem,op,2);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0)
    {
        return CPDL_ERROR;
    }
    return CPDL_SUCCESS;
}
#endif

/*! \brief 获取具名写锁(带超时)
    \param rwl - 读写锁实例对象
    \param timeout - 超时
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_wunlock
*/
int cpprwlock_wlock(cpprwlock_t rwl, const int timeout)
{
#ifdef _NWCP_WIN32
    return __cpprwlock_wlock(&rwl->rwl, timeout);
#elif defined(_CPDL_USE_POSIX_THREAD_PROCESS_SHARED)
    return cprwlock_wlock(rwl->rwl, timeout);
#elif defined(_CPDL_PPRW_USE_SHM_COUNTER)
    if(CPDL_SUCCESS != __rwlock_sem_op(*(rwl->event), cpprw_et_lock, -1, SEM_UNDO)) // 获取锁
        return CPDL_ERROR;
    if(++rwl->flag->writers == 1)
    {
        __rwlock_sem_op(*(rwl->event), cpprw_s_rlock, -1, SEM_UNDO); // 复归读信号
        if(0==rwl->flag->readers)
        {
            __rwlock_sem_op(*(rwl->event), cpprw_s_wlock, -1, SEM_UNDO); // 复归写信号
            __rwlock_sem_op(*(rwl->event), cpprw_et_lock, 1, SEM_UNDO); // 释放锁
            return CPDL_SUCCESS;
        }
    }
    __rwlock_sem_op(*(rwl->event), cpprw_et_lock, 1, SEM_UNDO); // 释放锁

    if(CPDL_SUCCESS == __rwlock_sem_opov(*(rwl->event), cpprw_s_wlock, -1, SEM_UNDO, timeout)) // 获取写信号
        return CPDL_SUCCESS;
    // 写操作锁定失败时的处理
    __rwlock_sem_op(*(rwl->event), cpprw_et_lock, -1, SEM_UNDO); // 获取锁
    if(--rwl->flag->writers == 0)
    {
        __rwlock_sem_op(*(rwl->event), cpprw_s_rlock, 1, SEM_UNDO); // 重置读信号
    }
    __rwlock_sem_op(*(rwl->event), cpprw_et_lock, 1, SEM_UNDO); // 释放锁
    return CPDL_ERROR;
#else
    if(CPDL_SUCCESS != __rwlock_sem_op(rwl->event, cpprw_et_lock, -1, SEM_UNDO)) // 获取锁
        return CPDL_ERROR;
    __rwlock_sem_op(rwl->event, cpprw_c_writer, 1, SEM_UNDO); // 写计数++
    if(semctl(rwl->event, cpprw_c_writer, GETVAL) == 1)
    {
        __rwlock_sem_op(rwl->event, cpprw_s_rlock, -1, SEM_UNDO); // 复归读信号
        if(0==semctl(rwl->event, cpprw_c_reader, GETVAL))
        {
            __rwlock_sem_op(rwl->event, cpprw_s_wlock, -1, SEM_UNDO); // 复归写信号
            __rwlock_sem_op(rwl->event, cpprw_et_lock, 1, SEM_UNDO); // 释放锁
            return CPDL_SUCCESS;
        }
    }
    __rwlock_sem_op(rwl->event, cpprw_et_lock, 1, SEM_UNDO); // 释放锁

    if(CPDL_SUCCESS == __rwlock_sem_opov(rwl->event, cpprw_s_wlock, -1, SEM_UNDO, timeout)) // 获取写信号
        return CPDL_SUCCESS;
    // 写操作锁定失败时的处理
    __rwlock_sem_op(rwl->event, cpprw_et_lock, -1, SEM_UNDO); // 获取锁
    __rwlock_sem_op(rwl->event, cpprw_c_writer, -1, SEM_UNDO); // 写计数--
    if(semctl(rwl->event, cpprw_c_writer, GETVAL) == 0)
    {
        __rwlock_sem_op(rwl->event, cpprw_s_rlock, 1, SEM_UNDO); // 重置读信号
    }
    __rwlock_sem_op(rwl->event, cpprw_et_lock, 1, SEM_UNDO); // 释放锁
    return CPDL_ERROR;
#endif
}

/*! \brief 获取具名写锁(非堵塞)
    \param rwl - 读写锁实例对象
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_wunlock
*/
int cpprwlock_trywlock(cpprwlock_t rwl)
{
#ifdef _NWCP_WIN32
    return __cpprwlock_trywlock(&rwl->rwl);
#elif defined(_CPDL_USE_POSIX_THREAD_PROCESS_SHARED)
    return ( 0 == pthread_rwlock_trywrlock(rwl->rwl) ? CPDL_SUCCESS : CPDL_ERROR );
#elif defined(_CPDL_PPRW_USE_SHM_COUNTER)
    unsigned short num[2] = {cpprw_et_lock, cpprw_s_wlock};
    short opv[2] = { -1, -1}, flg[2] = {SEM_UNDO|IPC_NOWAIT, SEM_UNDO|IPC_NOWAIT};// 获取锁和写信号
    if(CPDL_SUCCESS== __rwlock_sem_op2(*(rwl->event), num, opv, flg))
    {
        if(++rwl->flag->writers == 1)
        {
            __rwlock_sem_op(*(rwl->event), cpprw_s_rlock, -1, SEM_UNDO); // 复归读信号
        }
        __rwlock_sem_op(*(rwl->event), cpprw_et_lock, 1, SEM_UNDO); // 释放锁
        return CPDL_SUCCESS;
    }
    return CPDL_ERROR;
#else
    unsigned short num[2] = {cpprw_et_lock, cpprw_s_wlock};
    short opv[2] = { -1, -1}, flg[2] = {SEM_UNDO|IPC_NOWAIT, SEM_UNDO|IPC_NOWAIT};// 获取锁和写信号
    if(CPDL_SUCCESS== __rwlock_sem_op2(rwl->event, num, opv, flg))
    {
        __rwlock_sem_op(rwl->event, cpprw_c_writer, 1, SEM_UNDO); // 写计数++
        if(semctl(rwl->event, cpprw_c_writer, GETVAL) == 1)
        {
            __rwlock_sem_op(rwl->event, cpprw_s_rlock, -1, SEM_UNDO); // 复归读信号
        }
        __rwlock_sem_op(rwl->event, cpprw_et_lock, 1, SEM_UNDO); // 释放锁
        return CPDL_SUCCESS;
    }
    return CPDL_ERROR;
#endif
}

/*! \brief 释放具名写锁(非堵塞)
    \param rwl - 读写锁实例对象
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_wlock, ::cprwlock_trywlock
*/
int cpprwlock_wunlock(cpprwlock_t rwl)
{
#ifdef _NWCP_WIN32
    return __cpprwlock_wunlock(&rwl->rwl);
#elif defined(_CPDL_USE_POSIX_THREAD_PROCESS_SHARED)
    return ( 0 == pthread_rwlock_unlock(rwl->rwl)) ? CPDL_SUCCESS : CPDL_ERROR;
#elif defined(_CPDL_PPRW_USE_SHM_COUNTER)
    if(CPDL_SUCCESS != __rwlock_sem_op(*(rwl->event), cpprw_et_lock, -1, SEM_UNDO))// 获取锁
        return  CPDL_ERROR;
    if(--rwl->flag->writers == 0)
    {
        __rwlock_sem_op(*(rwl->event), cpprw_s_rlock, 1, SEM_UNDO); // 置读信号
    }
    __rwlock_sem_op(*(rwl->event), cpprw_s_wlock, 1, SEM_UNDO); // 置写信号
    __rwlock_sem_op(*(rwl->event), cpprw_et_lock, 1, SEM_UNDO); // 释放锁
    return CPDL_SUCCESS;
#else
    if(CPDL_SUCCESS != __rwlock_sem_op(rwl->event, cpprw_et_lock, -1, SEM_UNDO))// 获取锁
        return  CPDL_ERROR;
    __rwlock_sem_op(rwl->event, cpprw_c_writer, -1, SEM_UNDO); // 写计数--
    if(semctl(rwl->event, cpprw_c_writer, GETVAL) == 0)
    {
        __rwlock_sem_op(rwl->event, cpprw_s_rlock, 1, SEM_UNDO); // 置读信号
    }
    __rwlock_sem_op(rwl->event, cpprw_s_wlock, 1, SEM_UNDO); // 置写信号
    __rwlock_sem_op(rwl->event, cpprw_et_lock, 1, SEM_UNDO); // 释放锁
    return CPDL_SUCCESS;
#endif
}

/*! \brief 获取具名读锁(带超时)
    \param rwl - 读写锁实例对象
    \param timeout - 超时
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_runlock
*/
int cpprwlock_rlock(cpprwlock_t rwl, const int timeout)
{
#ifdef _NWCP_WIN32
    return __cpprwlock_rlock(&rwl->rwl, timeout);
#elif defined(_CPDL_USE_POSIX_THREAD_PROCESS_SHARED)
    return cprwlock_rlock(rwl->rwl, timeout);
#elif defined(_CPDL_PPRW_USE_SHM_COUNTER)
    unsigned short num[2] = {cpprw_et_lock, cpprw_s_rlock};
    short opv[2] = { -1, -1}, flg[2] = {SEM_UNDO, SEM_UNDO};
    if(CPDL_SUCCESS== __rwlock_sem_op2ov(*(rwl->event), num, opv, flg, timeout))// 获取锁和读信号
    {
        if(++rwl->flag->readers == 1)
        {
            __rwlock_sem_op(*(rwl->event), cpprw_s_wlock, -1, SEM_UNDO); // 复归写信号
        }
        __rwlock_sem_op(*(rwl->event), cpprw_s_rlock, 1, SEM_UNDO); // 置读信号
        __rwlock_sem_op(*(rwl->event), cpprw_et_lock, 1, SEM_UNDO); // 释放锁
        return CPDL_SUCCESS;
    }
    return CPDL_ERROR;
#else
    unsigned short num[2] = {cpprw_et_lock, cpprw_s_rlock};
    short opv[2] = { -1, -1}, flg[2] = {SEM_UNDO, SEM_UNDO};
    if(CPDL_SUCCESS== __rwlock_sem_op2ov(rwl->event, num, opv, flg, timeout))// 获取锁和读信号
    {
        __rwlock_sem_op(rwl->event, cpprw_c_reader, 1, SEM_UNDO); // 读计数++
        if(semctl(rwl->event, cpprw_c_reader, GETVAL) == 1)
        {
            __rwlock_sem_op(rwl->event, cpprw_s_wlock, -1, SEM_UNDO); // 复归写信号
        }
        __rwlock_sem_op(rwl->event, cpprw_s_rlock, 1, SEM_UNDO); // 置读信号
        __rwlock_sem_op(rwl->event, cpprw_et_lock, 1, SEM_UNDO); // 释放锁
        return CPDL_SUCCESS;
    }
    return CPDL_ERROR;
#endif
}

/*! \brief 获取具名读锁(非堵塞)
    \param rwl - 读写锁实例对象
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_runlock, ::cprwlock_rlock
*/
int cpprwlock_tryrlock(cpprwlock_t rwl)
{
#ifdef _NWCP_WIN32
    return __cpprwlock_rlock(&rwl->rwl, 0);
#elif defined(_CPDL_USE_POSIX_THREAD_PROCESS_SHARED)
    return ( 0 == pthread_rwlock_tryrdlock(rwl->rwl) ? CPDL_SUCCESS : CPDL_ERROR );
#elif defined(_CPDL_PPRW_USE_SHM_COUNTER)
    unsigned short num[2] = {cpprw_et_lock, cpprw_s_rlock};
    short opv[2] = { -1, -1}, flg[2] = {SEM_UNDO|IPC_NOWAIT, SEM_UNDO|IPC_NOWAIT};
    if(CPDL_SUCCESS== __rwlock_sem_op2(*(rwl->event), num, opv, flg)) // 获取锁和读信号
    {
        if(++rwl->flag->readers == 1)
        {
            __rwlock_sem_op(*(rwl->event), cpprw_s_wlock, -1, SEM_UNDO); // 复归写信号
        }
        __rwlock_sem_op(*(rwl->event), cpprw_s_rlock, 1, SEM_UNDO); // 置读信号
        __rwlock_sem_op(*(rwl->event), cpprw_et_lock, 1, SEM_UNDO); // 释放锁
        return CPDL_SUCCESS;
    }
    return CPDL_ERROR;
#else
    unsigned short num[2] = {cpprw_et_lock, cpprw_s_rlock};
    short opv[2] = { -1, -1}, flg[2] = {SEM_UNDO|IPC_NOWAIT, SEM_UNDO|IPC_NOWAIT};
    if(CPDL_SUCCESS== __rwlock_sem_op2(rwl->event, num, opv, flg)) // 获取锁和读信号
    {
        __rwlock_sem_op(rwl->event, cpprw_c_reader, 1, SEM_UNDO); // 读计数++
        if(semctl(rwl->event, cpprw_c_reader, GETVAL) == 1)
        {
            __rwlock_sem_op(rwl->event, cpprw_s_wlock, -1, SEM_UNDO); // 复归写信号
        }
        __rwlock_sem_op(rwl->event, cpprw_s_rlock, 1, SEM_UNDO); // 置读信号
        __rwlock_sem_op(rwl->event, cpprw_et_lock, 1, SEM_UNDO); // 释放锁
        return CPDL_SUCCESS;
    }
    return CPDL_ERROR;
#endif
}

/*! \brief 释放具名读锁
    \param rwl - 读写锁实例对象
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cprwlock_rlock, ::cprwlock_tryrlock
*/
int cpprwlock_runlock(cpprwlock_t rwl)
{
#ifdef _NWCP_WIN32
    return __cpprwlock_runlock(&rwl->rwl);
#elif defined(_CPDL_USE_POSIX_THREAD_PROCESS_SHARED)
    return ( 0 == pthread_rwlock_unlock(rwl->rwl) ? CPDL_SUCCESS : CPDL_ERROR );
#elif defined(_CPDL_PPRW_USE_SHM_COUNTER)
    if(CPDL_SUCCESS != __rwlock_sem_op(*(rwl->event), cpprw_et_lock, -1, SEM_UNDO)) // 获取锁
        return CPDL_ERROR;
    if(--rwl->flag->readers == 0) // 没有等待读的任务
    {
        __rwlock_sem_op(*(rwl->event), cpprw_s_wlock, 1, SEM_UNDO);// 置写信号
    }
     __rwlock_sem_op(*(rwl->event), cpprw_et_lock, 1, SEM_UNDO); // 释放锁
    return CPDL_SUCCESS;
#else
    if(CPDL_SUCCESS != __rwlock_sem_op(rwl->event, cpprw_et_lock, -1, SEM_UNDO)) // 获取锁
        return CPDL_ERROR;
    __rwlock_sem_op(rwl->event, cpprw_c_reader, -1, SEM_UNDO); // 读计数--
    if(semctl(rwl->event, cpprw_c_reader, GETVAL) == 0) // 没有等待读的任务
    {
        __rwlock_sem_op(rwl->event, cpprw_s_wlock, 1, SEM_UNDO);// 置写信号
    }
     __rwlock_sem_op(rwl->event, cpprw_et_lock, 1, SEM_UNDO); // 释放锁
    return CPDL_SUCCESS;
#endif
}

/*@}*/
