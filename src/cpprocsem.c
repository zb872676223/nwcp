
#include "cpprocsem.h"
#include <string.h>
#include <stdlib.h>
#ifdef _NWCP_WIN32
#    include <limits.h>
#else
#    include <errno.h>
#    include <unistd.h>
#    include <sys/ipc.h>
#    include <sys/time.h>
#    include <sys/stat.h>
#    include <time.h>
#    include <fcntl.h>
#   ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
#    include <semaphore.h>
#   else
#    include <sys/sem.h>
#       if defined(_SEM_SEMUN_UNDEFINED) || defined(_NWCP_SUNOS)
union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                           (Linux-specific) */
};
#       endif
#   endif
#endif

/*! \file  cpsemaphore.h
    \brief 信号量管理
*/

/*! \addtogroup CPSEMAPHORE 信号量管理
    \remarks 在UNIX下, 如果系统实现了POSIX 1003.1b的semaphore, 则使用POSIX信号量, 否则
    使用System V的信号量*/
/*@{*/

#ifndef _NWCP_WIN32
/* 获取完整的POSIX IPC名称,将name加上前缀, 产生full_name
   因为各种UNIX对POSIX IPC的实现不同, 为保证可移植性, 用
   这个函数来生成IPC名称 */
void __get_ipc_name(const char *name, char *ipcname, const char *op)
{
    if(!name || !ipcname)
        return;
    strcpy(ipcname, "/tmp/");
    strcat(ipcname, op);
    strcat(ipcname, name);
}
key_t __tok_ipc_file(const char *ipcname, int id)
{
    key_t key = ftok(ipcname, id);
    if(key < 0)
    {
        key = open(ipcname, O_WRONLY|O_CREAT, 0777);
        if (key < 0 )
            return CPDL_ERROR;
        close(key);
        key = ftok(ipcname, id);
    }
    return key;
}

static const char *cpprocmutex_ipc_op = "cpmutex_";

struct cpposix_mutex
{
#  ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    cpshm_t         shm;
    pthread_mutex_t *pmutex;
    unsigned int *pcount;
    unsigned int locked;
#  else
    int semmutex;
    char create;
#  endif
};

extern int cpshm_create(const char *shm_id, const unsigned int len, cpshm_t *shm_t);
extern int cpshm_map(const char *shm_id, cpshm_t *shm_t);
extern int cpshm_data(const cpshm_t shm_t, cpshm_d *pdata, unsigned int *len);
extern int cpshm_close(cpshm_t *shm_t);
#  ifndef _NWCP_SUNOS
extern size_t strlcpy(char *dst, const char *src, size_t siz);
extern size_t strlcat(char *dst, const char *src, size_t siz);
#  endif

#  ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
const unsigned int CPMUTEX_EX_SHM_SIZE = sizeof(unsigned int) + sizeof(pthread_mutex_t);
#  endif
#ifdef _CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM
int cpmutex_create_ex(char *shm, cpmutex_t *mutex)
{
    pthread_mutex_t *pmutex = 0;
    pthread_mutexattr_t attr;
    if(CPDL_SUCCESS == cpmutex_open_ex(shm, mutex))
        return CPDL_SUCCESS;

    if(pthread_mutexattr_init(&attr))
        return CPDL_ERROR;
    if(pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED))
    {
        pthread_mutexattr_destroy(&attr);
        return CPDL_ERROR;
    }
    pmutex = (pthread_mutex_t*)(shm + sizeof(unsigned int));
    if(pthread_mutex_init(pmutex, &attr))
    {
        pthread_mutexattr_destroy(&attr);
        return CPDL_ERROR;
    }
    pthread_mutexattr_destroy(&attr);
    ++(*(unsigned int*)shm);
    *mutex = (struct cpposix_mutex*)malloc(sizeof(struct cpposix_mutex));
    (*mutex)->pmutex = pmutex;
    (*mutex)->pcount = (unsigned int*)shm;
    (*mutex)->shm = 0;
    (*mutex)->locked = 0;
    return CPDL_SUCCESS;
}

int cpmutex_open_ex(char *shm, cpmutex_t *mutex)
{
    pthread_mutex_t *pmutex = 0;
    pthread_mutexattr_t attr;
    if((*(unsigned int*)shm) == 0)
        return CPDL_ERROR;

    if(pthread_mutexattr_init(&attr))
    {
        return CPDL_ERROR;
    }
    if(pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED))
    {
        pthread_mutexattr_destroy(&attr);
        return CPDL_ERROR;
    }
    pmutex = (pthread_mutex_t*)(shm + sizeof(unsigned int));
    if(pthread_mutex_init(pmutex, &attr))
    {
        pthread_mutexattr_destroy(&attr);
        return CPDL_ERROR;
    }
    pthread_mutexattr_destroy(&attr);

    ++(*(unsigned int*)shm);
    *mutex = (struct cpposix_mutex*)malloc(sizeof(struct cpposix_mutex));
    (*mutex)->pmutex = pmutex;
    (*mutex)->pcount = (unsigned int*)shm;
    (*mutex)->shm = 0;
    (*mutex)->locked = 0;
    return CPDL_SUCCESS;
}
#  endif
#endif

int cpmutex_create(const char *name, cpmutex_t *mutex)
{
#ifdef _NWCP_WIN32
    *mutex = CreateMutex(NULL, FALSE, name);
    if (0 == (*mutex))
        return CPDL_ERROR;
#else
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    cpshm_t         shm;
    cpshm_d         data = 0;
    unsigned int    *pcount = 0;
    pthread_mutex_t *pmutex = 0;
    pthread_mutexattr_t attr;
    char ipcname[256];
    if(CPDL_SUCCESS == cpmutex_open(name, mutex))
        return CPDL_SUCCESS;
    strcpy(ipcname, cpprocmutex_ipc_op); strlcat(ipcname, name, 128);
    if(CPDL_SUCCESS !=  cpshm_create(ipcname, CPMUTEX_EX_SHM_SIZE,  &shm))
        return CPDL_ERROR;
    if(CPDL_SUCCESS != cpshm_data(shm, &data, 0) || 0 == data)
    {
        cpshm_close( &shm);
        return CPDL_ERROR;
    }

    if(pthread_mutexattr_init(&attr))
    {
        cpshm_close( &shm);
        return CPDL_ERROR;
    }
    if(pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED))
    {
        pthread_mutexattr_destroy(&attr);
        cpshm_close( &shm);
        return CPDL_ERROR;
    }
    pcount = (unsigned int *)data;
    ++(*pcount);
    pmutex = (pthread_mutex_t*)(data + sizeof(unsigned int));
    if(pthread_mutex_init(pmutex, &attr))
    {
        pthread_mutexattr_destroy(&attr);
        cpshm_close( &shm);
        return CPDL_ERROR;
    }
    pthread_mutexattr_destroy(&attr);

    *mutex = (struct cpposix_mutex*)malloc(sizeof(struct cpposix_mutex));
    (*mutex)->pmutex = pmutex;
    (*mutex)->pcount = pcount;
    (*mutex)->shm = shm;
    (*mutex)->locked = 0;
#else
    key_t key;
    int semid = 0;
    char ipcname[256];
    unsigned short values[1];
    union semun arg;

    if(CPDL_SUCCESS == cpmutex_open(name, mutex))
        return CPDL_SUCCESS;

    __get_ipc_name(name, ipcname, cpprocmutex_ipc_op);
    key = __tok_ipc_file(ipcname, 0);
    if (key < 0 )
        return CPDL_ERROR;
    semid = semget(key,1, IPC_CREAT|IPC_EXCL | 0666);
    if(semid == -1)
        return CPDL_ERROR;

    values[0]=1;
    arg.array = values;
    if(semctl(semid, 0, SETALL, arg)==-1)
    {
        semctl(semid, 0, IPC_RMID);
        return CPDL_ERROR;
    }

    *mutex = (struct cpposix_mutex*)malloc(sizeof(struct cpposix_mutex));
    (*mutex)->semmutex = semid;
    (*mutex)->create = 1;
#endif
    // end !Windows
#endif
    return CPDL_SUCCESS;
}

int cpmutex_open(const char *name, cpmutex_t *mutex)
{
#ifdef _NWCP_WIN32
    *mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, name);
    if (0 == (*mutex))
        return CPDL_ERROR;
#else
    // !Windows
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    cpshm_t         shm;
    cpshm_d         data = 0;
    unsigned int len;
    unsigned int    *pcount = 0;
    pthread_mutex_t *pmutex = 0;
    pthread_mutexattr_t attr;
    char ipcname[256];
    strcpy(ipcname, cpprocmutex_ipc_op); strlcat(ipcname, name, 128);
    if(CPDL_SUCCESS !=  cpshm_map(ipcname, &shm))
        return CPDL_ERROR;
    if(CPDL_SUCCESS != cpshm_data(shm, &data, &len) || 0 == data || CPMUTEX_EX_SHM_SIZE != len)
    {
        cpshm_close( &shm);
        return CPDL_ERROR;
    }

    if(pthread_mutexattr_init(&attr))
    {
        cpshm_close( &shm);
        return CPDL_ERROR;
    }
    if(pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED))
    {
        pthread_mutexattr_destroy(&attr);
        cpshm_close( &shm);
        return CPDL_ERROR;
    }
    pcount = (unsigned int *)data;
    ++(*pcount);
    pmutex = (pthread_mutex_t*)(data + sizeof(unsigned int));
    if(pthread_mutex_init(pmutex, &attr))
    {
        pthread_mutexattr_destroy(&attr);
        cpshm_close( &shm);
        return CPDL_ERROR;
    }
    pthread_mutexattr_destroy(&attr);

    *mutex = (struct cpposix_mutex*)malloc(sizeof(struct cpposix_mutex));
    (*mutex)->pmutex = pmutex;
    (*mutex)->pcount = pcount;
    (*mutex)->shm = shm;
    (*mutex)->locked = 0;
#else
    int semid = 0;
    key_t key = -1;
    char ipcname[256];
    __get_ipc_name(name, ipcname, cpprocmutex_ipc_op);
    key = __tok_ipc_file(ipcname, 0);
    if (key < 0 )
        return CPDL_ERROR;
    semid = semget(key, 0, 0);
    if(semid == -1)
        return CPDL_ERROR;
    *mutex = (struct cpposix_mutex*)malloc(sizeof(struct cpposix_mutex));
    (*mutex)->semmutex = semid;
    (*mutex)->create = 0;
#endif
    // end !Windows
#endif
    return CPDL_SUCCESS;
}

int cpmutex_close(cpmutex_t *mutex)
{
#ifndef _NWCP_WIN32
    // !Windows
#ifndef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    int  semid = -1;
    char create = 0;
    union semun arg;
#endif
    // end !Windows
#endif

    if(0==mutex || 0==(*mutex))
        return CPDL_EINVAL;

#ifdef _NWCP_WIN32
    if(CloseHandle(*mutex) == FALSE)
        return CPDL_ERROR;
#else
    // !Windows
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    if((*mutex)->pmutex)
    {
        if ((*mutex)->locked)
        {
            pthread_mutex_unlock((*mutex)->pmutex);
        }
        pthread_mutex_destroy((*mutex)->pmutex);
    }
    --(*(*mutex)->pcount);
    if ((*mutex)->shm && CPDL_EINVAL == cpshm_close( &(*mutex)->shm ))
    {
    }
#else
    semid = (*mutex)->semmutex;
    create = (*mutex)->create;
    if( 1==create && semid != -1 )
    {
        arg.val = 0;
        semctl(semid, 0, IPC_RMID, arg);
    }
#endif
    free((void*)(*mutex));
    *mutex = 0;
    // end !Windows
#endif
    return CPDL_SUCCESS;
}

int cpmutex_trylock(const cpmutex_t mutex)
{
#ifdef _NWCP_WIN32
    if(WaitForSingleObject(mutex, 0) != WAIT_OBJECT_0)
        return CPDL_ERROR;
#else
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    if(0==mutex->pmutex)
        return CPDL_EINVAL;
    if(pthread_mutex_trylock(mutex->pmutex))
        return CPDL_ERROR;
    ++mutex->locked;
#else
    struct sembuf op[1];
    int rc;
    op[0].sem_num = 0;
    op[0].sem_op = -1;
    op[0].sem_flg = SEM_UNDO | IPC_NOWAIT;
    do
    {
        rc = semop(mutex->semmutex, op, 1);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0)
    {
        return CPDL_ERROR;
    }
#endif
#endif
    return CPDL_SUCCESS;
}

int cpmutex_lock(const cpmutex_t mutex)
{
    if(0==mutex)
        return CPDL_EINVAL;
#ifdef _NWCP_WIN32
    if(WaitForSingleObject(mutex, INFINITE) != WAIT_OBJECT_0)
        return CPDL_ERROR;
#else
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    if(0==mutex->pmutex)
        return CPDL_EINVAL;
    if(pthread_mutex_lock(mutex->pmutex))
        return CPDL_ERROR;
    ++mutex->locked;
#else
    struct sembuf op[1];
    int rc;
    op[0].sem_num = 0;
    op[0].sem_op = -1;
    op[0].sem_flg = SEM_UNDO;
    do {
        rc = semop(mutex->semmutex,op,1);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0)
    {
        return CPDL_ERROR;
    }
#endif
#endif
    return CPDL_SUCCESS;
}

int cpmutex_unlock(const cpmutex_t mutex)
{
    if(0==mutex)
        return CPDL_EINVAL;
#ifdef _NWCP_WIN32
    if(FALSE == ReleaseMutex(mutex))
        return CPDL_ERROR;
#else
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    if(0==mutex->pmutex)
        return CPDL_EINVAL;
    if(pthread_mutex_unlock(mutex->pmutex))
        return CPDL_ERROR;
    --mutex->locked;
#else
    struct sembuf op[1];
    int rc;
    op[0].sem_num = 0;
    op[0].sem_op = 1;
    op[0].sem_flg = SEM_UNDO;
    do
    {
        rc = semop(mutex->semmutex,op,1);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0)
    {
        return CPDL_ERROR;
    }
#endif
#endif
    return CPDL_SUCCESS;
}

#ifndef _NWCP_WIN32
static const char *cpprocsem_ipc_op = "cpsem_";
struct cpposix_sem
{
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    cpshm_t shm_t;
    sem_t    *psem;
    unsigned int *pcount;
#else
    int sem;
#endif
    char create;
};

#  ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
const unsigned int CPSEM_EX_SHM_SIZE =  sizeof(unsigned int) + sizeof(sem_t);
#  endif
#  ifdef _CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM
int cpsem_create_ex(char *shm, const unsigned int intval,
                    const unsigned int max, cpsem_t *sem)
{
    sem_t *psem = 0;
    if(CPDL_SUCCESS == cpsem_open_ex(shm, sem))
        return CPDL_SUCCESS;
    psem = (sem_t*)(shm+sizeof(unsigned int));
    if (sem_init(psem, 1, intval) == -1)
        return CPDL_ERROR;
    ++(*(unsigned int*)shm);
    *sem = (struct cpposix_sem*)malloc(sizeof(struct cpposix_sem));
    (*sem)->psem = psem;
    (*sem)->pcount = (unsigned int*)shm;
    (*sem)->create = 1;
    (*sem)->shm_t = 0;
    return CPDL_SUCCESS;
}

int cpsem_open_ex(char *shm, cpsem_t *sem)
{
    if((*(unsigned int*)shm)==0)
        return CPDL_ERROR;
    ++(*(unsigned int*)shm);
    *sem = (struct cpposix_sem*)malloc(sizeof(struct cpposix_sem));
    (*sem)->psem = (sem_t*)(shm+sizeof(unsigned int));
    (*sem)->pcount = (unsigned int*)shm;
    (*sem)->create = 0;
    (*sem)->shm_t = 0;
    return CPDL_SUCCESS;
}
#  endif
#endif

/*! \brief 创建进程信号量
    \param name - 信号量的名称
    \param value - 信号量的初始值
    \param sem - 返回创建的进程信号量
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \remarks 此函数用于创建进程信号量, 如果同名的进程信号量已经存在,
             则打开已存在的进程信号量。
    \sa ::cpsem_wait, ::cpsem_post, ::cpsem_open
*/
int cpsem_create(const char *name, const unsigned int intval,
                 const unsigned int max, cpsem_t *sem)
{
#ifndef _NWCP_WIN32
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    cpshm_t shm_t = 0;
    cpshm_d data = 0;
    unsigned int *pcount = 0;
    sem_t *psem = 0;
#else
    key_t key;
    int semid = 0;
    unsigned short values[1];
    union semun arg;
#endif
    char ipcname[256];
#endif
    if(CPDL_SUCCESS == cpsem_open(name, sem))
        return CPDL_SUCCESS;
#ifdef _NWCP_WIN32
    *sem = CreateSemaphore(NULL, intval, max, name);
    if(0==(*sem))
        return CPDL_ERROR;
#else
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    strcpy(ipcname, cpprocsem_ipc_op); strlcat(ipcname, name, 128);
    if(CPDL_SUCCESS !=  cpshm_create(ipcname, CPSEM_EX_SHM_SIZE,  &shm_t))
        return CPDL_ERROR;

    if(CPDL_SUCCESS != cpshm_data(shm_t, &data, 0) || 0 == data)
    {
        cpshm_close( &shm_t);
        return CPDL_ERROR;
    }
    psem = (sem_t*)(data + sizeof(unsigned int));
    if (sem_init(psem, 1, intval) == -1)
    {
        cpshm_close( &shm_t);
        return CPDL_ERROR;
    }
    pcount = (unsigned int*)data;
    ++(*pcount);
    *sem = (struct cpposix_sem*)malloc(sizeof(struct cpposix_sem));
    (*sem)->psem = psem;
    (*sem)->pcount = pcount;
    (*sem)->create = 1;
    (*sem)->shm_t = shm_t;
#else
    __get_ipc_name(name, ipcname, cpprocsem_ipc_op);
    key = __tok_ipc_file(ipcname, 0);
    if (key < 0 )
        return CPDL_ERROR;
    semid = semget(key,1, IPC_CREAT|IPC_EXCL | 0666);
    if(semid == -1)
        return CPDL_ERROR;

    values[0]=intval;
    arg.array = values;
    if(semctl(semid, 0, SETALL, arg)==-1)
    {
        semctl(semid, 0, IPC_RMID);
        return CPDL_ERROR;;
    }

    *sem = (struct cpposix_sem*)malloc(sizeof(struct cpposix_sem));
    (*sem)->sem = semid;
    (*sem)->create = 1;
#endif
#endif
    return CPDL_SUCCESS;
}

/*! \brief 打开一个已存在的进程信号量
    \param name - 信号量的名称
    \param sem - 返回创建的进程信号量
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cpsem_wait, ::cpsem_post, ::cpsem_init
*/
int cpsem_open(const char *name, cpsem_t *sem)
{
#ifdef _NWCP_WIN32
    *sem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, name);
    if(0 == (*sem))
        return CPDL_ERROR;
#else
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    cpshm_t shm_t = 0;
    cpshm_d data = 0;
    unsigned int *pcount = 0;
    sem_t *psem = 0;
    unsigned int len = 0;
    int ret = 0;
    char ipcname[256];
    strcpy(ipcname, cpprocsem_ipc_op); strlcat(ipcname, name, 128);
    ret = cpshm_map(ipcname, &shm_t);
    if(CPDL_SUCCESS != ret)
        return CPDL_ERROR;

    ret = cpshm_data(shm_t, &data, &len);
    if(CPDL_SUCCESS != ret || 0 == data || CPSEM_EX_SHM_SIZE != len)
    {
        cpshm_close( &shm_t);
        return CPDL_ERROR;
    }
    pcount = (unsigned int *)data;
    ++(*pcount);
    psem = (sem_t*)(data + sizeof(unsigned int));
    *sem = (struct cpposix_sem*)malloc(sizeof(struct cpposix_sem));
    (*sem)->psem = psem;
    (*sem)->pcount = pcount;
    (*sem)->create = 0;
    (*sem)->shm_t = shm_t;
#else
    int semid = 0;
    key_t key = -1;
    char ipcname[256];
    __get_ipc_name(name, ipcname, cpprocsem_ipc_op);
    key = __tok_ipc_file(ipcname, 0);
    if (key < 0 )
        return CPDL_ERROR;
    semid = semget(key, 0, 0);
    if(semid == -1)
        return CPDL_ERROR;

    *sem = (struct cpposix_sem*)malloc(sizeof(struct cpposix_sem));
    (*sem)->sem = semid;
    (*sem)->create = 0;
#endif
#endif
    return CPDL_SUCCESS;
}

/*! \brief 关闭并删除进程信号量
    \param sem - 进程信号量
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
*/
int cpsem_close(cpsem_t *sem)
{
#ifdef _NWCP_WIN32
    CloseHandle(*sem);
#else
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    sem_t   *psem = (*sem)->psem;
    cpshm_t  shm_t = (*sem)->shm_t;
    int create = (*sem)->create;
    --(*((*sem)->pcount));
    free((void*)(*sem));
    *sem = 0;

    if (create)
    {
        if (sem_destroy(psem) < 0)
        {
        }
    }

    if (shm_t && CPDL_EINVAL == cpshm_close( &shm_t ))
    {
    }
#else
    int semid = (*sem)->sem;
    char create = (*sem)->create;
    free((void*)(*sem));
    *sem = 0;
    if( 1==create && semid != -1 )
        semctl(semid, 0, IPC_RMID);
#endif
#endif
    return CPDL_SUCCESS;
}

/*! \brief 等待信号量
    \param sem - 进程信号量
    \return  CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \remarks 若信号量值小于等于0将阻塞最多ms毫秒(<0则无线等待)，
            有信号量则将信号量减1返回成功，否则超时失败
    \sa ::cpsem_post
*/
int cpsem_wait(const cpsem_t sem, const int ms)
{
#ifdef _NWCP_WIN32
    DWORD millisecond;
    if(ms <= CPDL_TIME_INFINITE)
        millisecond = INFINITE;
    else
        millisecond = ms;
    if(WAIT_OBJECT_0 != WaitForSingleObject(sem, millisecond))
        return CPDL_ERROR;
#else
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    int ret;
    struct timespec ts;
    if (ms < 0)
    {
        ret = sem_wait(sem->psem);
        if(ret < 0 && errno != EINTR)
            return CPDL_ERROR;
    }
    else
    {
        clock_gettime(CLOCK_REALTIME, &ts); /* POSIX.1b时钟 */
        ts.tv_nsec += ((ms % 1000) * 1000 * 1000);
        ts.tv_sec +=  ((long)(ts.tv_nsec / 1000 / 1000 / 1000) + (long)(ms / 1000));
        ts.tv_nsec %= 1000 * 1000 * 1000;
        if (sem_timedwait(sem->psem, &ts) == -1)
            return CPDL_ERROR;
    }
#else
    struct sembuf op[1];
    struct timespec ts;
    int rc = -1;
    op[0].sem_num = 0;
    op[0].sem_op = -1;
    op[0].sem_flg = 0;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000;
    do {
        rc = semtimedop(sem->sem, op, 1, &ts);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0)
    {
        return CPDL_ERROR;
    }
#endif
#endif
    return CPDL_SUCCESS;
}

/*! \brief 尝试消费信号量
    \param sem - 进程信号量
    \return  CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \remarks 若信号量值不等于0, 并将信号量减1并返回成功，否则失败
    \sa ::cpsem_post
*/
int cpsem_trywait(const cpsem_t sem)
{
#ifdef _NWCP_WIN32
    if(WAIT_OBJECT_0 != WaitForSingleObject(sem, 0))
        return CPDL_ERROR;
#else
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    if(sem_trywait(sem->psem) < 0)
        return CPDL_ERROR;
#else
    struct sembuf op[1];
    int rc = -1;
    op[0].sem_num = 0;
    op[0].sem_op = -1;
    op[0].sem_flg = IPC_NOWAIT;
    do {
        rc = semop(sem->sem, op, 1);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0)
    {
        return CPDL_ERROR;
    }
#endif
#endif
    return CPDL_SUCCESS;
}

/*! \brief 产生信号量
    \param sem - 进程信号量
    \return  CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \remarks 将信号量加1
    \sa ::cpsem_wait
*/
int cpsem_post(const cpsem_t sem)
{
#ifdef _NWCP_WIN32
    if(FALSE == ReleaseSemaphore(sem, 1, NULL))
        return CPDL_ERROR;
#else
#ifdef _CPDL_USE_POSIX_THREAD_PROCESS_SHARED
    if(sem_post(sem->psem) < 0)
        return CPDL_ERROR;
#else
    struct sembuf op[1];
    int rc = -1;
    op[0].sem_num = 0;
    op[0].sem_op =  1;
    op[0].sem_flg = 0;
    do
    {
        rc = semop(sem->sem, op, 1);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0)
    {
        return CPDL_ERROR;
    }
#endif
#endif
    return CPDL_SUCCESS;
}
/*@}*/


/*! \brief 等待信号量并锁mutex
    \param sem - 进程信号量
    \param mutex - 进程互斥锁
    \return  CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \remarks 若有信号量，则解锁mutex，解锁成功信号量减1，失败后恢复信号量
    \sa ::cpmutex_sem_lock, ::cpmutex_lock, ::cpsem_wait
*/
int cpmutex_sem_lock(const cpsem_t sem, const cpmutex_t mutex, const int timeout)
{
    if(CPDL_SUCCESS != cpsem_wait(sem, timeout))
    {
        return CPDL_ERROR;
    }

    if(CPDL_SUCCESS != cpmutex_lock(mutex))
    {
        cpsem_post(sem);
        return CPDL_ERROR;
    }
    return CPDL_SUCCESS;
}

/*! \brief 解锁mutex并置信号量
    \param sem - 进程信号量
    \param mutex - 进程互斥锁
    \return  CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \remarks 解锁mutex后将信号量加1
    \sa ::cpmutex_sem_lock, ::cpmutex_unlock, ::cpsem_post
*/
int cpmutex_sem_unlock(const cpsem_t sem, const cpmutex_t mutex)
{
    if(CPDL_SUCCESS != cpmutex_unlock(mutex))
    {
        return CPDL_ERROR;
    }

    if(CPDL_SUCCESS != cpsem_post(sem))
    {
        return CPDL_ERROR;
    }
    return CPDL_SUCCESS;
}
