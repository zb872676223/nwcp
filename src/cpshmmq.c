
/*
modification history
--------------------
02r,09apr18,nonwill  fix for linux and unix
01a,11nov11,sgu  created

DESCRIPTION
The message queue manipulation functions in this file are similar with the
Wind River VxWorks kernel message queue interfaces.

The memory architecture for a message queue:
----------------   -----------------------------------------------------------
| local memory |-->|                     shared memory                       |
----------------   -----------------------------------------------------------
       ^                                     ^
       |                                     |
----------------   -----------------------------------------------------------
|    MSG_Q     |   | MSG_SM | MSG_NODE list |       message queue data       |
----------------   -----------------------------------------------------------
                                    ^                         ^
                                    |                         |
             ---------------------------------------------    |
             | MSG_NODE1 | MSG_NODE2 | ... | MSG_NODE(N) |    |
             ---------------------------------------------    |
                                                              |
                                              ---------------------------------
                                              | data1 | data2 | ... | data(N) |
                                              ---------------------------------
*/

#include "../cpshmmq.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _NWCP_SUNOS
extern size_t strlcpy(char *dst, const char *src, size_t siz);
extern size_t strlcat(char *dst, const char *src, size_t siz);
#endif

#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
extern int cpmutex_create_ex(char *shm, cpmutex_t *mutex);
extern int cpmutex_open_ex(char *shm, cpmutex_t *mutex);
#else
extern int cpmutex_create(const char *name, cpmutex_t *mutex);
extern int cpmutex_open(const char *name, cpmutex_t *mutex);
#endif
extern int cpmutex_close(cpmutex_t *mutex);
extern int cpmutex_unlock(const cpmutex_t mutex);

#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
extern int cpsem_create_ex(char *shm, const unsigned int intval,
                                const unsigned int maxvalue, cpsem_t *sem);
extern int cpsem_open_ex(char *shm, cpsem_t *sem);
#else
extern int cpsem_create(const char *name, const unsigned int intval,
                                const unsigned int maxvalue, cpsem_t *sem);
extern int cpsem_open(const char *name, cpsem_t *sem);
#endif
extern int cpsem_wait(const cpsem_t sem, const int timeout);
extern int cpsem_post(const cpsem_t sem);
extern int cpsem_close(cpsem_t *sem);

extern int cpmutex_sem_lock(const cpsem_t sem, const cpmutex_t mutex, const int timeout);
extern int cpmutex_sem_unlock(const cpsem_t sem, const cpmutex_t mutex);

extern int cpshm_create(const char *shm_id, const unsigned int len, cpshm_t *shm_t);
extern int cpshm_map(const char *shm_id, cpshm_t *shm_t);
extern int cpshm_close(cpshm_t *shm_t);
extern int cpshm_data(const cpshm_t shm_t, cpshm_d *pdata, unsigned int *len);
extern int cpshm_exist(const char *shm_id);

/*! \file  cpshmmq.c
    \brief 基于共享内存的FIFO消息队列管理
*/

/*! \addtogroup CPSHMMQ 共享内存的FIFO消息队列
    \remarks  */
/*@{*/


/* defines */

/* invalid node index for message queue node */
#define MSG_Q_INVALID_NODE -1

/* objects name prefix */
#if defined(_NWCP_WIN32) || !defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
static const char *_MSG_Q_SEM_P_ = "cpshmmq_p_"; /* prefix for pruducer semaphore */
static const char *_MSG_Q_SEM_C_ = "cpshmmq_c_"; /* prefix for consumer semaphore */
static const char *_MSG_Q_MUTEX_ = "cpshmmq_l_"; /* prefix for mutex */
#endif
static const char *_MSG_Q_SHMEM_ = "cpshmmq_shm_"; /* prefix for shared memory */
#define MSG_Q_PREFIX_LEN   32                       /* max length of object prefix */

/* typedefs */

/* message queue status */
typedef struct tagCPSHMMQ_STATUS {
    char file[CPSHMMQ_NAME_LEN];
    //    unsigned int used;
    unsigned int maxMsgs;       /* max messages that can be queued */
    unsigned int maxMsgLength;  /* max bytes in a message */
    unsigned int msgNum;                 /* message number in the queue */
    unsigned int sendTimes;              /* number of sent */
    unsigned int recvTimes;              /* number of received */
} CPSHMMQ_STATUS;
/* message queue status */

/* message node structure */
typedef struct tagMSG_NODE {
    unsigned int length;     /* message length */
    int index;                /* node index */
    int free;                 /* next free message index */
    int used;                 /* next used message index */
}MSG_NODE;

/* message queue attributes */
typedef struct tagMSG_SM {
    struct tagCPSHMMQ_STATUS stat;
    int head;                   /* head index of the queue */
    int tail;                   /* tail index of the queue */
    int free;                   /* next free index of the queue */
}MSG_SM;

/* objects handlers for message queue */
typedef struct _cpmq_t {
    cpsem_t semPId;    /* semaphore for producer */
    cpsem_t semCId;    /* semaphore for consumer */
    cpmutex_t mutex;     /* mutex for shared data protecting */
    cpshm_t hFile;     /* file handle for the shared memory file mapping */
    MSG_SM  *psm;     /* shared memory */
}tagMSG_Q, MSG_Q, *P_MSG_Q;

/* implementations */

size_t cpshmmq_shm_size(const unsigned int count, const unsigned int msglen)
{
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
   return CPSEM_EX_SHM_SIZE * 2 +CPMUTEX_EX_SHM_SIZE +
            sizeof(MSG_SM) + count * (sizeof(MSG_NODE) + msglen);
#else
    return sizeof(MSG_SM) + count * (sizeof(MSG_NODE) + msglen);
#endif
}

int cpshmmq_create_ex(const char *name, const unsigned int count,
                      const unsigned int msglen, const cpshm_d data,
                      cpshmmq_t *mq_t)
{
    P_MSG_Q qid = NULL;     /* message queue identify */
    char strName[CPSHMMQ_NAME_LEN + MSG_Q_PREFIX_LEN];
    MSG_NODE * pNode = NULL;
    unsigned int index = 0;

    /* check the inputed parameters */
    if (0==data || 0==mq_t || 0==count || 0 == msglen || name == NULL)
        return CPDL_EINVAL;

    /* shared memory for inter-process message queue */
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    memset(data, 0, CPMUTEX_EX_SHM_SIZE + CPSEM_EX_SHM_SIZE * 2);
#endif

    /* allocate the message queue control block memory */
    qid = (P_MSG_Q)malloc(sizeof(MSG_Q));
    memset(qid, 0, sizeof(MSG_Q));
    qid->hFile = 0;
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    qid->psm = (MSG_SM*)(data + CPMUTEX_EX_SHM_SIZE + CPSEM_EX_SHM_SIZE * 2);
#else
    qid->psm = (MSG_SM*)(data);
#endif

    /* create the producer semaphore */
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    if(CPDL_ERROR == cpsem_create_ex(data, 0, count, &qid->semPId))
#else
    snprintf(strName,  CPSHMMQ_NAME_LEN - 1, "%s%s", _MSG_Q_SEM_P_, name);
    if(CPDL_ERROR == cpsem_create(strName, 0, count, &qid->semPId))
#endif
    {
        cpshm_close(&qid->hFile);
        free(qid);
        return CPDL_ERROR;
    }

    /* create the consumer semaphore */
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    if(CPDL_ERROR == cpsem_create_ex(data + CPSEM_EX_SHM_SIZE, count, count, &qid->semCId))
#else
    snprintf(strName,  CPSHMMQ_NAME_LEN - 1, "%s%s", _MSG_Q_SEM_C_, name);
    if(CPDL_SUCCESS != cpsem_create(strName, count, count, &qid->semCId))
#endif
    {
        cpsem_close(&qid->semPId);
        cpshm_close(&qid->hFile);
        free(qid);
        return CPDL_ERROR;
    }

    /* create the mutex */
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    if(CPDL_SUCCESS != cpmutex_create_ex(data + CPSEM_EX_SHM_SIZE * 2, &qid->mutex))
#else
    snprintf(strName,  CPSHMMQ_NAME_LEN - 1, "%s%s", _MSG_Q_MUTEX_, name);
    if(CPDL_SUCCESS != cpmutex_create(strName, &qid->mutex))
#endif
    {
        cpsem_close(&qid->semCId);
        cpsem_close(&qid->semPId);
        cpshm_close(&qid->hFile);
        free(qid);
        return CPDL_ERROR;
    }

    /* set message queue attributes in shared memory */
    strlcpy(qid->psm->stat.file, name, CPSHMMQ_NAME_LEN);
    qid->psm->stat.maxMsgs = count;
    qid->psm->stat.maxMsgLength = msglen;
    qid->psm->stat.msgNum = 0;
    qid->psm->stat.sendTimes = 0;
    qid->psm->stat.recvTimes = 0;
    qid->psm->head = MSG_Q_INVALID_NODE;
    qid->psm->tail = MSG_Q_INVALID_NODE;
    qid->psm->free = 0;

    /* set the message queue nodes links */
    pNode = (MSG_NODE*)((char*)qid->psm + sizeof(MSG_SM));
    for(index = 0; index < qid->psm->stat.maxMsgs; index++)
    {
        pNode->length = 0;
        pNode->index = index;
        pNode->free = index + 1;
        pNode->used = MSG_Q_INVALID_NODE;
        pNode++;
    }

    /* set the next free pointer as INVALID for the last Node */
    pNode--;
    pNode->free = MSG_Q_INVALID_NODE;

    *mq_t = qid;
    return CPDL_SUCCESS;
}

/*******************************************************************************
 * cpshmmq_create - create a message queue, queue pended tasks in FIFO order
 *
 * create a message queue, queue pended tasks in FIFO order.
 * <name> message name, if name equals NULL, create an inter-thread message
 * queue, or create an inter-process message queue.
 *
 */
int cpshmmq_create(const char * name, /* message name */
                   const unsigned int count,     /* max messages that can be queued */
                   const unsigned int msglen,/* max bytes in a message */
                   cpshmmq_t *mq_t)
{
    P_MSG_Q qid = NULL;     /* message queue identify */
    char strName[CPSHMMQ_NAME_LEN + MSG_Q_PREFIX_LEN];
    cpshm_t shm_t = 0;
    cpshm_d data = 0;
    MSG_NODE * pNode = NULL;
    unsigned int index = 0;
    unsigned int mapLen = 0;
    int create = 0;
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    const unsigned int memSize = CPSEM_EX_SHM_SIZE * 2 +CPMUTEX_EX_SHM_SIZE +
            sizeof(MSG_SM) + count * (sizeof(MSG_NODE) + msglen);
#else
    const unsigned int memSize = sizeof(MSG_SM) + count * (sizeof(MSG_NODE) + msglen);
#endif

    /* check the inputed parameters */
    if (0==mq_t || 0==count || 0 == msglen || name == NULL)
        return CPDL_EINVAL;

    /* allocate the message queue memory */

    /* allocate shared memory for inter-process message queue */
    snprintf(strName,  CPSHMMQ_NAME_LEN + MSG_Q_PREFIX_LEN - 1, "%s%s", _MSG_Q_SHMEM_, name);
    if( CPDL_SUCCESS != cpshm_map(strName, &shm_t)/*cpshm_exist(strName)*/ )
    {
        if( CPDL_SUCCESS!=cpshm_create(strName, memSize, &shm_t) )
            return CPDL_ERROR;
        if( CPDL_SUCCESS != cpshm_data(shm_t, &data, 0))
        {
            cpshm_close(&shm_t);
            return CPDL_ERROR;
        }
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
        memset(data, 0, CPMUTEX_EX_SHM_SIZE + CPSEM_EX_SHM_SIZE * 2);
#endif
        create = 1;
    }
    else if( CPDL_SUCCESS != cpshm_data(shm_t, &data, &mapLen) || mapLen != memSize)
    {
        cpshm_close(&shm_t);
        return CPDL_ERROR;
    }

    /* allocate the message queue control block memory */
    qid = (P_MSG_Q)malloc(sizeof(MSG_Q));
    memset(qid, 0, sizeof(MSG_Q));
    qid->hFile = shm_t;
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    qid->psm = (MSG_SM*)(data + CPMUTEX_EX_SHM_SIZE + CPSEM_EX_SHM_SIZE * 2);
#else
    qid->psm = (MSG_SM*)(data);
#endif

    /* create the producer semaphore */
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    if(CPDL_ERROR == cpsem_create_ex(data, 0, count, &qid->semPId))
#else
    snprintf(strName,  CPSHMMQ_NAME_LEN - 1, "%s%s", _MSG_Q_SEM_P_, name);
    if(CPDL_ERROR == cpsem_create(strName, 0, count, &qid->semPId))
#endif
    {
        cpshm_close(&qid->hFile);
        free(qid);
        return CPDL_ERROR;
    }

    /* create the consumer semaphore */
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    if(CPDL_ERROR == cpsem_create_ex(data + CPSEM_EX_SHM_SIZE, count, count, &qid->semCId))
#else
    snprintf(strName,  CPSHMMQ_NAME_LEN - 1, "%s%s", _MSG_Q_SEM_C_, name);
    if(CPDL_SUCCESS != cpsem_create(strName, count, count, &qid->semCId))
#endif
    {
        cpsem_close(&qid->semPId);
        cpshm_close(&qid->hFile);
        free(qid);
        return CPDL_ERROR;
    }

    /* create the mutex */
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    if(CPDL_SUCCESS != cpmutex_create_ex(data + CPSEM_EX_SHM_SIZE * 2, &qid->mutex))
#else
    snprintf(strName,  CPSHMMQ_NAME_LEN - 1, "%s%s", _MSG_Q_MUTEX_, name);
    if(CPDL_SUCCESS != cpmutex_create(strName, &qid->mutex))
#endif
    {
        cpsem_close(&qid->semCId);
        cpsem_close(&qid->semPId);
        cpshm_close(&qid->hFile);
        free(qid);
        return CPDL_ERROR;
    }

    if (create)
    {
        /* set message queue attributes in shared memory */
        strlcpy(qid->psm->stat.file, name, CPSHMMQ_NAME_LEN);
        //qid->psm->stat.used = 1;
        qid->psm->stat.maxMsgs = count;
        qid->psm->stat.maxMsgLength = msglen;
        qid->psm->stat.msgNum = 0;
        qid->psm->stat.sendTimes = 0;
        qid->psm->stat.recvTimes = 0;
        qid->psm->head = MSG_Q_INVALID_NODE;
        qid->psm->tail = MSG_Q_INVALID_NODE;
        qid->psm->free = 0;

        /* set the message queue nodes links */
        pNode = (MSG_NODE*)((char*)qid->psm + sizeof(MSG_SM));
        for(index = 0; index < qid->psm->stat.maxMsgs; index++)
        {
            pNode->length = 0;
            pNode->index = index;
            pNode->free = index + 1;
            pNode->used = MSG_Q_INVALID_NODE;
            pNode++;
        }

        /* set the next free pointer as INVALID for the last Node */
        pNode--;
        pNode->free = MSG_Q_INVALID_NODE;
    }
    else
    {
        //++qid->psm->stat.used;
    }

    *mq_t = qid;
    return CPDL_SUCCESS;
}

int cpshmmq_open_ex(const char * name, const cpshm_d data, cpshmmq_t *mq_t,
                    unsigned int *count, unsigned int *msglen)
{
    P_MSG_Q qid = NULL;     /* message queue identify */
    char strName[CPSHMMQ_NAME_LEN + MSG_Q_PREFIX_LEN];
    cpshm_t shm_t = 0;

#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    const unsigned int minMemSize = CPSEM_EX_SHM_SIZE * 2 +CPMUTEX_EX_SHM_SIZE;
#else
    const unsigned int minMemSize = 0;
#endif

    if (0==data || 0 == mq_t || 0 == name)
        return CPDL_EINVAL;

    /* open the message queue share data */
    snprintf(strName,  CPSHMMQ_NAME_LEN + MSG_Q_PREFIX_LEN - 1, "%s%s", _MSG_Q_SHMEM_, name);
    if( CPDL_SUCCESS != cpshm_map(strName, &shm_t) )
        return CPDL_ERROR;
    /* allocate the message queue memory */
    if( strncmp(name, ((MSG_SM*)(data + minMemSize))->stat.file, CPSHMMQ_NAME_LEN) )
    {
        return CPDL_ERROR;
    }

    /* allocate the message queue control block memory */
    qid = (P_MSG_Q)malloc(sizeof(MSG_Q));
    memset(qid, 0, sizeof(MSG_Q));
    qid->hFile = 0;
    qid->psm = (MSG_SM*)(data + minMemSize);

    /* open the producer semaphore */
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    if(CPDL_SUCCESS != cpsem_open_ex(data, &qid->semPId))
#else
    snprintf(strName,  CPSHMMQ_NAME_LEN + MSG_Q_PREFIX_LEN - 1, "%s%s", _MSG_Q_SEM_P_, name);
    if(CPDL_SUCCESS != cpsem_open(strName, &qid->semPId))
#endif
    {
        cpshm_close(&qid->hFile);
        free(qid);
        return CPDL_ERROR;
    }

    /* open the consumer semaphore */
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    if(CPDL_SUCCESS != cpsem_open_ex(data + CPSEM_EX_SHM_SIZE, &qid->semCId))
#else
    snprintf(strName,  CPSHMMQ_NAME_LEN + MSG_Q_PREFIX_LEN - 1, "%s%s", _MSG_Q_SEM_C_, name);
    if(CPDL_SUCCESS != cpsem_open(strName, &qid->semCId))
#endif
    {
        cpsem_close(&qid->semPId);
        cpshm_close(&qid->hFile);
        free(qid);
        return CPDL_ERROR;
    }

    /* open the protecting mutex */
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    if(CPDL_SUCCESS != cpmutex_open_ex(data + CPSEM_EX_SHM_SIZE * 2, &qid->mutex))
#else
    snprintf(strName,  CPSHMMQ_NAME_LEN + MSG_Q_PREFIX_LEN - 1, "%s%s", _MSG_Q_MUTEX_, name);
    if(CPDL_SUCCESS != cpmutex_open(strName, &qid->mutex))
#endif
    {
        cpsem_close(&qid->semCId);
        cpsem_close(&qid->semPId);
        cpshm_close(&qid->hFile);
        free(qid);
        return CPDL_ERROR;
    }

    //++qid->psm->stat.used;

    if(count)
        *count = qid->psm->stat.maxMsgs;
    if(msglen)
        *msglen = qid->psm->stat.maxMsgLength;

    *mq_t = qid;
    return CPDL_SUCCESS;
}

/*******************************************************************************
 * cpshmmq_open - open a message queue
 *
 * open a message queue.
 *
 */
int cpshmmq_open(const char * name, cpshmmq_t *mq_t, unsigned int *count, unsigned int *msglen)
{
    P_MSG_Q qid = NULL;     /* message queue identify */
    char strName[CPSHMMQ_NAME_LEN + MSG_Q_PREFIX_LEN];
    cpshm_t shm_t = 0;
    cpshm_d data = 0;
    unsigned int mlen = 0;
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    const unsigned int minMemSize = CPSEM_EX_SHM_SIZE * 2 +CPMUTEX_EX_SHM_SIZE;
#else
    const unsigned int minMemSize = 0;
#endif

    if (0 == mq_t || 0 == name)
        return CPDL_EINVAL;

    /* open the message queue share data */
    snprintf(strName,  CPSHMMQ_NAME_LEN + MSG_Q_PREFIX_LEN - 1, "%s%s", _MSG_Q_SHMEM_, name);
    if( CPDL_SUCCESS != cpshm_map(strName, &shm_t) )
        return CPDL_ERROR;
    /* allocate the message queue memory */
    if( CPDL_SUCCESS != cpshm_data(shm_t, &data, &mlen) || data == NULL ||
            mlen < (minMemSize + sizeof(MSG_SM)) ||
            strncmp(name, ((MSG_SM*)(data + minMemSize))->stat.file, CPSHMMQ_NAME_LEN) )
    {
        cpshm_close(&qid->hFile);
        return CPDL_ERROR;
    }

    /* allocate the message queue control block memory */
    qid = (P_MSG_Q)malloc(sizeof(MSG_Q));
    memset(qid, 0, sizeof(MSG_Q));
    qid->hFile = shm_t;
    qid->psm = (MSG_SM*)(data + minMemSize);

    /* open the producer semaphore */
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    if(CPDL_SUCCESS != cpsem_open_ex(data, &qid->semPId))
#else
    snprintf(strName,  CPSHMMQ_NAME_LEN + MSG_Q_PREFIX_LEN - 1, "%s%s", _MSG_Q_SEM_P_, name);
    if(CPDL_SUCCESS != cpsem_open(strName, &qid->semPId))
#endif
    {
        cpshm_close(&qid->hFile);
        free(qid);
        return CPDL_ERROR;
    }

    /* open the consumer semaphore */
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    if(CPDL_SUCCESS != cpsem_open_ex(data + CPSEM_EX_SHM_SIZE, &qid->semCId))
#else
    snprintf(strName,  CPSHMMQ_NAME_LEN + MSG_Q_PREFIX_LEN - 1, "%s%s", _MSG_Q_SEM_C_, name);
    if(CPDL_SUCCESS != cpsem_open(strName, &qid->semCId))
#endif
    {
        cpsem_close(&qid->semPId);
        cpshm_close(&qid->hFile);
        free(qid);
        return CPDL_ERROR;
    }

    /* open the protecting mutex */
#if !defined(_NWCP_WIN32) && defined(_CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM)
    if(CPDL_SUCCESS != cpmutex_open_ex(data + CPSEM_EX_SHM_SIZE * 2, &qid->mutex))
#else
    snprintf(strName,  CPSHMMQ_NAME_LEN + MSG_Q_PREFIX_LEN - 1, "%s%s", _MSG_Q_MUTEX_, name);
    if(CPDL_SUCCESS != cpmutex_open(strName, &qid->mutex))
#endif
    {
        cpsem_close(&qid->semCId);
        cpsem_close(&qid->semPId);
        cpshm_close(&qid->hFile);
        free(qid);
        return CPDL_ERROR;
    }

    //++qid->psm->stat.used;

    if(count)
        *count = qid->psm->stat.maxMsgs;
    if(msglen)
        *msglen = qid->psm->stat.maxMsgLength;

    *mq_t = qid;
    return CPDL_SUCCESS;
}

/*******************************************************************************
 * cpshmmq_close - delete a message queue
 *
 * delete a message queue.
 *
 */
int cpshmmq_close( cpshmmq_t *mq_t )
{
    int failed = 0;
    if(CPDL_SUCCESS != cpsem_close(&(*mq_t)->semPId))
        failed++;
    if(CPDL_SUCCESS != cpsem_close(&(*mq_t)->semCId))
        failed++;
    if(CPDL_SUCCESS != cpmutex_close(&(*mq_t)->mutex))
        failed++;
    if( (*mq_t)->hFile && CPDL_EINVAL == cpshm_close(&(*mq_t)->hFile) )
        failed++;
    free((void*)(*mq_t));
    //(*mq_t) = 0;
    return failed == 0 ? CPDL_SUCCESS : CPDL_ERROR;
}

int cpshmmq_rdlock(const cpshmmq_t mq_t, const int timeout)
{
    if(CPDL_SUCCESS != cpmutex_sem_lock(mq_t->semPId, mq_t->mutex, timeout))
    {
        return CPDL_ERROR;
    }
    if(MSG_Q_INVALID_NODE == mq_t->psm->tail && 0==mq_t->psm->stat.msgNum)
    {
        cpmutex_unlock(mq_t->mutex);
        cpsem_post(mq_t->semPId);
        return CPDL_NODATA;
    }
    return mq_t->psm->stat.msgNum;
}

int cpshmmq_rwunlock(const cpshmmq_t mq_t)
{
    if(CPDL_SUCCESS != cpmutex_unlock(mq_t->mutex))
    {
        return CPDL_ERROR;
    }
    return CPDL_SUCCESS;
}

int cpshmmq_rdpop(const cpshmmq_t mq_t, char * buf,
                  unsigned int *buflen)
{
    MSG_NODE * pNode = NULL;
    if(MSG_Q_INVALID_NODE == mq_t->psm->tail && 0==mq_t->psm->stat.msgNum)
    {
        return CPDL_NODATA;
    }

    /* get the message node we want to process */
    pNode = (MSG_NODE*)((char*)mq_t->psm + sizeof(MSG_SM) +
                        mq_t->psm->tail * sizeof(MSG_NODE));

    /* calculate the message length */
    if((*buflen) > pNode->length)
        *buflen = pNode->length;
    //(*buflen) = ((*buflen) > pNode->length) ? pNode->length : (*buflen);

    /* copy the message to buffer */
    memcpy(buf, (char*)mq_t->psm + sizeof(MSG_SM) +
           mq_t->psm->stat.maxMsgs * sizeof(MSG_NODE) +
           mq_t->psm->stat.maxMsgLength * pNode->index, (*buflen));

    /* update the tail of the used message link */
    mq_t->psm->tail = pNode->used;

    /* free and append the message node to the free message link */
    pNode->used = MSG_Q_INVALID_NODE;
    pNode->free = mq_t->psm->free;
    mq_t->psm->free = pNode->index;

    /* there is no message if the tail equals to MSG_Q_INVALID_NODE */
    if (mq_t->psm->tail == MSG_Q_INVALID_NODE)
        mq_t->psm->head = MSG_Q_INVALID_NODE;

    /* update the message counting attributes */
    --mq_t->psm->stat.msgNum;
    ++mq_t->psm->stat.recvTimes;
    if(CPDL_SUCCESS != cpsem_post(mq_t->semCId))
    {
        return CPDL_ERROR;
    }
    return mq_t->psm->stat.msgNum;
}

/*******************************************************************************
 * cpshmmq_pop - receive a message from a message queue
 *
 * receive a message from a message queue.
 *
 */
int cpshmmq_pop(const cpshmmq_t mq_t,  /* message queue from which to receive */
                char * buf,    /* buffer to receive message */
                unsigned int *buflen,   /* length of buffer */
                const int timeout       /* ticks to wait */
                )
{
    MSG_NODE * pNode = NULL;

//    if(0==mq_t || 0==buf || 0==buflen || (*buflen) < 1)
//    {
//        return CPDL_EINVAL;
//    }

    /* message is available if the producer semaphore can be taken */
    /* take the mutex for shared memory protecting */
    /*if(CPDL_SUCCESS != cpsem_wait(mq_t->semPId, timeout))
    {
        return CPDL_ERROR;
    }
    if(CPDL_SUCCESS != cpmutex_lock(mq_t->mutex/))
    {
        // release the producer semaphore if failed to take the mutex
        cpsem_post(mq_t->semPId);

        return CPDL_ERROR;
    }*/
    if(CPDL_SUCCESS != cpmutex_sem_lock(mq_t->semPId, mq_t->mutex, timeout))
    {
        return CPDL_ERROR;
    }

    if(MSG_Q_INVALID_NODE == mq_t->psm->tail && 0==mq_t->psm->stat.msgNum)
    {
        cpmutex_unlock(mq_t->mutex);
        cpsem_post(mq_t->semPId);
        return CPDL_NODATA;
    }

    /* get the message node we want to process */
    pNode = (MSG_NODE*)((char*)mq_t->psm + sizeof(MSG_SM) +
                        mq_t->psm->tail * sizeof(MSG_NODE));

    /* calculate the message length */
    if((*buflen) > pNode->length)
        *buflen = pNode->length;
    //(*buflen) = ((*buflen) > pNode->length) ? pNode->length : (*buflen);

    /* copy the message to buffer */
    memcpy(buf, (char*)mq_t->psm + sizeof(MSG_SM) +
           mq_t->psm->stat.maxMsgs * sizeof(MSG_NODE) +
           mq_t->psm->stat.maxMsgLength * pNode->index, (*buflen));

    /* update the tail of the used message link */
    mq_t->psm->tail = pNode->used;

    /* free and append the message node to the free message link */
    pNode->used = MSG_Q_INVALID_NODE;
    pNode->free = mq_t->psm->free;
    mq_t->psm->free = pNode->index;

    /* there is no message if the tail equals to MSG_Q_INVALID_NODE */
    if (mq_t->psm->tail == MSG_Q_INVALID_NODE)
        mq_t->psm->head = MSG_Q_INVALID_NODE;

    /* update the message counting attributes */
    --mq_t->psm->stat.msgNum;
    ++mq_t->psm->stat.recvTimes;

    /* release mutex */
    /* release the consumer semaphore */
    /*if(CPDL_SUCCESS != cpmutex_unlock(mq_t->mutex))
    {
        return CPDL_ERROR;
    }
    if(CPDL_SUCCESS != cpsem_post(mq_t->semCId))
    {
        return CPDL_ERROR;
    }*/
    if(CPDL_SUCCESS != cpmutex_sem_unlock(mq_t->semCId, mq_t->mutex))
    {
        return CPDL_ERROR;
    }
    return CPDL_SUCCESS;
}

int cpshmmq_wrlock(const cpshmmq_t mq_t, const int timeout)
{
    if(CPDL_SUCCESS != cpmutex_sem_lock(mq_t->semCId, mq_t->mutex, timeout))
    {
        return CPDL_ERROR;
    }
    if(MSG_Q_INVALID_NODE == mq_t->psm->free &&
            mq_t->psm->stat.maxMsgs==mq_t->psm->stat.msgNum)
    {
        cpmutex_unlock(mq_t->mutex);
        cpsem_post(mq_t->semCId);
        return CPDL_NODATA;
    }
    return mq_t->psm->stat.maxMsgs - mq_t->psm->stat.msgNum;
}

int cpshmmq_wrpush(const cpshmmq_t mq_t, const char * buf,
                   unsigned int *buflen, const int priority)
{
    MSG_NODE * pNode = NULL;
    if(MSG_Q_INVALID_NODE == mq_t->psm->free &&
            mq_t->psm->stat.maxMsgs==mq_t->psm->stat.msgNum)
    {
        return CPDL_NODATA;
    }

    if(priority != CPSHMMQ_PRI_NORMAL && priority != CPSHMMQ_PRI_URGENT)
    {
        return CPDL_EINVAL;
    }
    /* get a free message node we want to use */
    pNode = (MSG_NODE*)((char*)mq_t->psm + sizeof(MSG_SM) +
                        mq_t->psm->free * sizeof(MSG_NODE));

    /* update the next free message node number */
    mq_t->psm->free = pNode->free;

    /* check the message length */
    if((*buflen) > mq_t->psm->stat.maxMsgLength)
    {
        (*buflen) = mq_t->psm->stat.maxMsgLength;
    }

    /* set the node attributes */
    pNode->free = MSG_Q_INVALID_NODE;
    pNode->used = MSG_Q_INVALID_NODE;
    pNode->length = (*buflen);

    /* copy the buffer to the message node */
    memcpy((char*)mq_t->psm + sizeof(MSG_SM) +
           mq_t->psm->stat.maxMsgs * sizeof(MSG_NODE) +
           mq_t->psm->stat.maxMsgLength * pNode->index, buf, (*buflen));

    /* both the head and tail pointer to this node if it's the first message */
    if (mq_t->psm->head == MSG_Q_INVALID_NODE)
    {
        mq_t->psm->head = pNode->index;
        mq_t->psm->tail = pNode->index;
    }
    else
    {
        /* or send a normal message or urgent message */
        if (priority == CPSHMMQ_PRI_NORMAL)
        {
            /* get the head message node */
            MSG_NODE * temp = (MSG_NODE*)((char*)mq_t->psm + sizeof(MSG_SM) +
                                          mq_t->psm->head * sizeof(MSG_NODE));

            /* append the new message node to the head message node */
            temp->used = pNode->index;
            mq_t->psm->head = pNode->index;
        }
        else
        {
            /* append the new message node to the tail message node */
            pNode->used = mq_t->psm->tail;
            mq_t->psm->tail = pNode->index;
        }
    }

    /* update the message counting attributes */
    ++mq_t->psm->stat.msgNum;
    ++mq_t->psm->stat.sendTimes;

    if(CPDL_SUCCESS != cpsem_post(mq_t->semPId))
    {
        return CPDL_ERROR;
    }
    return mq_t->psm->stat.maxMsgs - mq_t->psm->stat.msgNum;
}

/*******************************************************************************
 * cpshmmq_push - send a message to a message queue
 *
 * send a message to a message queue.
 *
 */
int cpshmmq_push(const cpshmmq_t mq_t, /* message queue on which to send */
                 const char * buf,   /* message to send */
                 unsigned int *buflen,     /* length of message */
                 const int timeout,     /* ticks to wait */
                 const int priority     /* MSG_PRI_NORMAL or MSG_PRI_URGENT */
                 )
{
    MSG_NODE * pNode = NULL;
//    if(0==mq_t || 0==buf || 0==buflen || (*buflen) < 1)
//    {
//        return CPDL_EINVAL;
//    }
    if(priority != CPSHMMQ_PRI_NORMAL && priority != CPSHMMQ_PRI_URGENT)
    {
        return CPDL_EINVAL;
    }

    /* there is free slot in queue if the consumer semaphore can be taken */
    /* take the mutex for shared memory protecting */
    /*if(CPDL_SUCCESS != cpsem_wait(mq_t->semCId, timeout))
    {
        return CPDL_ERROR;
    }
    if(CPDL_SUCCESS != cpmutex_lock(mq_t->mutex))
    {
        // release the consumer semaphore if failed to take the mutex
        cpsem_post(mq_t->semCId);
        return CPDL_ERROR;
    }*/
    if(CPDL_SUCCESS != cpmutex_sem_lock(mq_t->semCId, mq_t->mutex, timeout))
    {
        return CPDL_ERROR;
    }

    if(MSG_Q_INVALID_NODE == mq_t->psm->free &&
            mq_t->psm->stat.maxMsgs==mq_t->psm->stat.msgNum)
    {
        cpmutex_unlock(mq_t->mutex);
        cpsem_post(mq_t->semCId);
        return CPDL_NODATA;
    }

    /* get a free message node we want to use */
    pNode = (MSG_NODE*)((char*)mq_t->psm + sizeof(MSG_SM) +
                        mq_t->psm->free * sizeof(MSG_NODE));

    /* update the next free message node number */
    mq_t->psm->free = pNode->free;

    /* check the message length */
    if((*buflen) > mq_t->psm->stat.maxMsgLength)
    {
        *buflen = mq_t->psm->stat.maxMsgLength;
    }

    /* set the node attributes */
    pNode->free = MSG_Q_INVALID_NODE;
    pNode->used = MSG_Q_INVALID_NODE;
    pNode->length = (*buflen);

    /* copy the buffer to the message node */
    memcpy((char*)mq_t->psm + sizeof(MSG_SM) +
           mq_t->psm->stat.maxMsgs * sizeof(MSG_NODE) +
           mq_t->psm->stat.maxMsgLength * pNode->index, buf, (*buflen));

    /* both the head and tail pointer to this node if it's the first message */
    if (mq_t->psm->head == MSG_Q_INVALID_NODE)
    {
        mq_t->psm->head = pNode->index;
        mq_t->psm->tail = pNode->index;
    }
    else
    {
        /* or send a normal message or urgent message */
        if (priority == CPSHMMQ_PRI_NORMAL)
        {
            /* get the head message node */
            MSG_NODE * temp = (MSG_NODE*)((char*)mq_t->psm + sizeof(MSG_SM) +
                                          mq_t->psm->head * sizeof(MSG_NODE));

            /* append the new message node to the head message node */
            temp->used = pNode->index;
            mq_t->psm->head = pNode->index;
        }
        else
        {
            /* append the new message node to the tail message node */
            pNode->used = mq_t->psm->tail;
            mq_t->psm->tail = pNode->index;
        }
    }

    /* update the message counting attributes */
    ++mq_t->psm->stat.msgNum;
    ++mq_t->psm->stat.sendTimes;

    /* release the mutex */
    /* release the producer semaphore */
    /*if(CPDL_SUCCESS != cpmutex_unlock(mq_t->mutex))
    {
        return CPDL_ERROR;
    }
    if(CPDL_SUCCESS != cpsem_post(mq_t->semPId))
    {
        return CPDL_ERROR;
    }*/
    if(CPDL_SUCCESS != cpmutex_sem_unlock(mq_t->semPId, mq_t->mutex))
    {
        return CPDL_ERROR;
    }
    return CPDL_SUCCESS;
}

/*******************************************************************************
 * cpshmmq_status - get the status of message queue
 *
 * get the detail status of a message queue.
 *
 */
int cpshmmq_status(const cpshmmq_t mq_t, unsigned int *msgcount, unsigned int *allsend,
                   unsigned int *allrecv, unsigned int *capacity, unsigned int *msglen,
                   char *name)
{
    /* save all the message queue attributes */
    CPSHMMQ_STATUS status;
    memcpy(&status, &mq_t->psm->stat, sizeof(CPSHMMQ_STATUS));
    if(msgcount)
        *msgcount = status.msgNum;
    if(allsend)
        *allsend = status.sendTimes;
    if(allrecv)
        *allrecv = status.recvTimes;
    if(capacity)
        *capacity = status.maxMsgs;
    if(msglen)
        *msglen = status.maxMsgLength;
    if(name)
        strlcpy(name, status.file, CPSHMMQ_NAME_LEN-1);
    return CPDL_SUCCESS;
}
