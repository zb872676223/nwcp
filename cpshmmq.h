#ifndef _CPSHMMESSAGEQUEUE_H
#define _CPSHMMESSAGEQUEUE_H

#include "cpdltype.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int CPDLAPI cpshmmq_create(const char* name, const unsigned int count,
                                  const unsigned int msglen, cpshmmq_t* mq_t);
extern size_t CPDLAPI cpshmmq_shm_size(const unsigned int count, const unsigned int msglen);
extern int CPDLAPI cpshmmq_create_ex(const char* name, const unsigned int count,
                                     const unsigned int msglen, const cpshm_d data,
                                     cpshmmq_t* mq_t);
extern int CPDLAPI cpshmmq_open(const char* name, cpshmmq_t* mq_t,
                                unsigned int* count, unsigned int* msglen);
extern int CPDLAPI cpshmmq_open_ex(const char* name, const cpshm_d data, cpshmmq_t* mq_t,
                                   unsigned int* count, unsigned int* msglen);
extern int CPDLAPI cpshmmq_close(cpshmmq_t* mq_t);
extern int CPDLAPI cpshmmq_pop(const cpshmmq_t mq_t, char* buf,
                               unsigned int* buflen, const int timeout);
extern int CPDLAPI cpshmmq_push(const cpshmmq_t mq_t, const char* buf,
                                unsigned int* buflen, const int timeout,
                                const int priority);
extern int CPDLAPI cpshmmq_status(const cpshmmq_t mq_t, unsigned int* msgcount,
                                  unsigned int* allsend, unsigned int* allrecv,
                                  unsigned int* capacity, unsigned int* msglen,
                                  char* name);

/* 尝试获取读保护锁
 * 获取失败返回CPDL_ERROR;
 * 超时timeout或无消息可读则释放读保护锁返回CPDL_NODATA;
 * 有消息读返回可读信息的数量.
 */
extern int CPDLAPI cpshmmq_rdlock(const cpshmmq_t mq_t, const int timeout);
/* 通过cpshmmq_rdlock接口返回有消息可读时,读取消息
 * 每调用一次读取一条消息,返回剩余可读的消息数量;无消息可读时返回CPDL_NODATA
 */
extern int CPDLAPI cpshmmq_rdpop(const cpshmmq_t mq_t, char* buf, unsigned int* buflen);
/* 尝试获取写保护锁
 * 获取失败返回CPDL_ERROR;
 * 超时timeout或队列满则释放读保护锁返回CPDL_NODATA;
 * 队列可写返回可空隙数量.
 */
extern int CPDLAPI cpshmmq_wrlock(const cpshmmq_t mq_t, const int timeout);
/* 通过cpshmmq_wrlock接口返回队列可写时,写消息
 * 每调用一次写一条消息,返回剩余可写的空隙数量;无空隙时返回CPDL_NODATA
 */
extern int CPDLAPI cpshmmq_wrpush(const cpshmmq_t mq_t, const char* buf,
                                  unsigned int* buflen, const int priority);
/* 通过cpshmmq_rdlock或cpshmmq_wrlock接口返回有消息可读,读取消息后调用此接口释放读保护锁
 */
extern int CPDLAPI cpshmmq_rwunlock(const cpshmmq_t mq_t);


#ifdef __cplusplus
}
#endif


#endif
