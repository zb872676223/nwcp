
#include "cpthread.h"

#ifdef _NWCP_WIN32
#include <process.h>
#else   /* UNIX */
#include <errno.h>
#include <time.h>
#endif  /* _NWCP_WIN32 */


/*! \file  cpthread.h
    \brief 线程管理
*/

/*! \addtogroup CPTHREAD 线程管理*/
/*@{*/

/*! \brief 创建线程
    \param tid - 返回线程的ID
    \param start_routine - 线程的入口函数
    \param arg - 传递给start_routine的参数
    \param joinable - 线程是否为JOINABLE
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \remarks 若创建失败，tid返回的值为0，只有对JOINABLE
         的线程，才能调用cpthread_join，如果线程属性为
         JOINABLE，必须调用cpthread_join才能彻底清除线程
    \sa ::cpthread_join, ::cpthread_exit
*/
int cpthread_create(cpthread_t *tid,
    CPTHREAD_ROUTINE start_routine, void *arg, int joinable)
{
#ifdef _NWCP_WIN32
    /* unsigned id; */
    HANDLE hthread;

    *tid = 0;
    hthread = (HANDLE)_beginthread(start_routine, 0, arg);
    /*hthread = _beginthreadex( NULL, 0,
        (unsigned (__stdcall * )(void *))start_routine, arg, 0, &id );*/
    if(hthread == (HANDLE)-1)
        return CPDL_ERROR;
    *tid = hthread;
#else
    int ret;
    pthread_attr_t attr;
    int detachstate;

    ret = pthread_attr_init(&attr);
    if(ret)
        return CPDL_ERROR;
    if(joinable)
        detachstate = PTHREAD_CREATE_JOINABLE;
    else
        detachstate = PTHREAD_CREATE_DETACHED;
    pthread_attr_setdetachstate(&attr, detachstate);
    *tid = 0;
    ret = pthread_create(tid, &attr, start_routine, arg);
    if(ret)
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

/*! \brief 阻塞，直到指定的线程终止
    \param tid - 线程的ID
    \param value_ptr - 接收线程退出时通过::cpthread_exit函数传递的值
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \remarks 此函数由线程的创建者调用，在WIN32下，此函数还将释放线程的句柄
    \sa ::cpthread_exit
*/
int cpthread_join(cpthread_t tid, int *value_ptr)
{
#ifdef _NWCP_WIN32
    DWORD exit_code, ret;
    exit_code = 0;
    if(FALSE == GetExitCodeThread(tid, &exit_code) )
        return CPDL_ERROR;

    while(exit_code == STILL_ACTIVE )
    {
        ret = WaitForSingleObject(tid, INFINITE);
        if(ret == WAIT_OBJECT_0)
            GetExitCodeThread(tid, &exit_code);
        else if(6==GetLastError()) // 句柄无效
        {
            return CPDL_NODATA;
        }
    }

    CloseHandle(tid);
    *value_ptr = exit_code;

    /*
    DWORD exit_code;
    do
    {
        if(!GetExitCodeThread(tid, &exit_code))
            return CPDL_ERROR;
        Sleep(1);
    }
    while(exit_code == STILL_ACTIVE);
    CloseHandle(tid);
    */
#else
    int ret;
    void *p;

    ret = pthread_join(tid, (void **)&p);
    *value_ptr = (int)p;
    if(ret)
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

/*! \brief 中止线程
    \param value - 子线程通过\a value向其创建者传值
    \return 无
    \sa ::cpthread_join
*/
void cpthread_exit(int value)
{
#ifdef _NWCP_WIN32
    //_endthread();
    _endthreadex(value);
#else
    pthread_exit((void *)value);
#endif
}

/*! \brief 初始化线程互斥对象
    \param mutex - 互斥对象指针
    \return CPDL_SUCCESS - 成功, CPDL_ERROR   - 失败
    \sa ::cpthread_mutex_destroy, ::cpthread_mutex_lock, ::cpthread_mutex_unlock
*/
int cpthread_mutex_init(cpthread_mutex_t *mutex)
{
#ifdef _NWCP_WIN32
    InitializeCriticalSection(mutex);
#else
    if(pthread_mutex_init(mutex, NULL))
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

/*! \brief 删除互斥对象
    \param mutex - 互斥对象指针
    \return CPDL_SUCCESS - 成功, CPDL_ERROR   - 失败
    \sa ::cpthread_mutex_init, ::cpthread_mutex_lock, ::cpthread_mutex_unlock
*/
int cpthread_mutex_destroy(cpthread_mutex_t *mutex)
{
#ifdef _NWCP_WIN32
    DeleteCriticalSection(mutex);
#else
    if(!pthread_mutex_destroy(mutex))
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

/*! \brief 对互斥对象加锁
    \param mutex - 互斥对象指针
    \return CPDL_SUCCESS - 成功, CPDL_ERROR   - 失败
    \sa ::cpthread_mutex_init, ::cpthread_mutex_destroy, ::cpthread_mutex_unlock
*/
int cpthread_mutex_lock(cpthread_mutex_t *mutex)
{
#ifdef _NWCP_WIN32
    EnterCriticalSection(mutex);
#else
    if(pthread_mutex_lock(mutex))
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

/*! \brief 对互斥对象加锁
    \param mutex - 互斥对象指针
    \return CPDL_SUCCESS - 成功, CPDL_ERROR   - 失败
    \sa ::cpthread_mutex_init, ::cpthread_mutex_destroy, ::cpthread_mutex_unlock
*/
int cpthread_mutex_trylock(cpthread_mutex_t *mutex)
{
#ifdef _NWCP_WIN32
    if(FALSE==TryEnterCriticalSection(mutex))
        return CPDL_ERROR;
#else
    if(pthread_mutex_trylock(mutex))
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

/*! \brief 对互斥对象解锁
    \param mutex - 互斥对象指针
    \return CPDL_SUCCESS - 成功, CPDL_ERROR   - 失败
    \sa ::cpthread_mutex_init, ::cpthread_mutex_destroy, ::cpthread_mutex_lock
*/
int cpthread_mutex_unlock(cpthread_mutex_t *mutex)
{
#ifdef _NWCP_WIN32
    LeaveCriticalSection(mutex);
#else
    if(pthread_mutex_unlock(mutex))
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

/*! \brief 创建条件变量
    \param cond - 条件变量指针
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \remarks 在WIN32下, 使用Event实现, 在UNIX下，使用PTHREAD实现
    \sa ::cpthread_cond_destroy, ::cpthread_cond_wait,
        ::cpthread_cond_timedwait, ::cpthread_cond_signal,
        ::cpthread_cond_broadcast
*/
int cpthread_cond_init(cpthread_cond_t *cond)
{
#ifdef _NWCP_WIN32
    HANDLE h;

    h = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(!h)
        return CPDL_ERROR;
    *cond = h;
#else
    if(pthread_cond_init(cond, NULL))
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

/*! \brief 删除条件变量
    \param cond - 条件变量指针
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cpthread_cond_init
*/
int cpthread_cond_destroy(cpthread_cond_t *cond)
{
#ifdef _NWCP_WIN32
    CloseHandle(*cond);
#else
    int ret;

    ret = pthread_cond_destroy(cond);
    if(ret)
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

/*! \brief 等待条件变量的信号
    \param cond - 条件变量指针
    \param mutex -  与条件变量\a cond相连的互斥锁
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \remarks 如果条件变量无信号, 调用线程阻塞并对\a mutex解锁,
        当接收到条件变量的信号时，函数返回<p><p>
        通常的用法如下:
        \code Waiting event signal:
        cpthread_mutex_lock(&mutex);
        while(!test_condition())
            cpthread_cond_wait(&cond, &mutex);
        cpthread_mutex_unlock(&mutex);
        \endcode
        \code Sending event signal:
        cpthread_mutex_lock(&mutex);
        cpthread_cond_signal(&cond);
        cpthread_mutex_unlock(&mutex);
        \endcode
    \attention 调用本函数前必须对\a mutex加锁, 函数返回后必须对它解锁
    \sa ::cpthread_cond_signal, ::cpthread_cond_broadcast,
        ::cpthread_cond_timedwait
*/
int cpthread_cond_wait(cpthread_cond_t *cond, cpthread_mutex_t *mutex)
{
#ifdef _NWCP_WIN32
    return cpthread_cond_timedwait(cond, mutex, CPDL_TIME_INFINITE);
#else
    int ret;

    ret = pthread_cond_wait(cond, mutex);
    if(ret)
        return CPDL_ERROR;
    return CPDL_SUCCESS;
#endif
}

/*! \brief 定时等待条件变量的信号
    \param cond - 条件变量指针
    \param mutex - 与条件变量\a cond相连的互斥锁
    \param ms - 等待的毫秒数
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败 CPDL_TIMEOUT - 超时
    \remarks 此函数与::cpthread_cond_wait的区别在于即使没有条件变量的信号
             到来, 当等待时间大于等于\a ms的时候, 函数也将返回
    \warning 注意此函数的时间参数与POSIX函数\a pthread_cond_timedwait的区别
    \sa ::cpthread_cond_signal, ::cpthread_cond_broadcast, ::cpthread_cond_wait
*/
int cpthread_cond_timedwait(cpthread_cond_t *cond,
    cpthread_mutex_t *mutex, int ms)
{
#ifdef _NWCP_WIN32
    DWORD ret;
    DWORD millisecond;

    if(ms == CPDL_TIME_INFINITE)
        millisecond = INFINITE;
    else
        millisecond = ms;

    cpthread_mutex_unlock(mutex);
    ret = WaitForSingleObject(*cond, millisecond);
    cpthread_mutex_lock(mutex);
    if(ret == WAIT_OBJECT_0)
        return CPDL_SUCCESS;
    else if(ret == WAIT_TIMEOUT)
        return CPDL_TIMEOUT;
    else
        return CPDL_ERROR;
#else
    int ret;
    struct timespec t;

    if(ms == CPDL_TIME_INFINITE)
        return cpthread_cond_wait(cond, mutex);

    clock_gettime(CLOCK_REALTIME, &t); /* POSIX.1b时钟 */
    //t.tv_nsec += ms * 1000 * 1000;
    //t.tv_sec += t.tv_nsec / 1000 / 1000 / 1000;
    t.tv_nsec += ((ms % 1000) * 1000000);
    t.tv_sec +=  ((long)(t.tv_nsec / 1000 / 1000 / 1000) + (long)(ms / 1000));
    t.tv_nsec %= 1000 * 1000 * 1000;

    ret = pthread_cond_timedwait(cond, mutex, &t);
    if(ret == ETIMEDOUT)
        return CPDL_TIMEOUT;
    else if(!ret)
        return CPDL_SUCCESS;
    else
        return CPDL_ERROR;
#endif
}

/*! \brief 发出条件变量的信号, 唤醒等待此信号的线程
    \param cond - 条件变量指针
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \remarks 此函数调用将唤醒一个等待此条件变量信号的线程, 在UNIX下,
             多个线程等待时, 哪一个线程被唤醒取决于具体的系统实现,
             在Windows系统下, 此函数与::cpthread_cond_broadcast相同
    \sa ::cpthread_cond_broadcast, ::cpthread_cond_wait, ::cpthread_cond_timedwait
*/
int cpthread_cond_signal(cpthread_cond_t *cond)
{
#ifdef _NWCP_WIN32
     if(!SetEvent(*cond))
        return CPDL_ERROR;
#else
    int ret;

    ret = pthread_cond_signal(cond);
    if(ret)
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

/*! \brief 发出条件变量的信号, 唤醒等待此信号的所有线程
    \param cond - 条件变量指针
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \remarks 此函数调用将唤醒所有等待此条件变量信号的线程
    \sa ::cpthread_cond_signal, ::cpthread_cond_wait, ::cpthread_cond_timedwait
*/
int cpthread_cond_broadcast(cpthread_cond_t *cond)
{
#ifdef _NWCP_WIN32
    return cpthread_cond_signal(cond);
#else
    int ret;

    ret = pthread_cond_broadcast(cond);
    if(ret)
        return CPDL_ERROR;
    else
        return CPDL_SUCCESS;
#endif
}

/*! \brief 线程休眠（毫秒精度）
    \param ms - 休眠的时间（毫秒）
    \return 无
*/
void cpthread_sleep(unsigned int ms)
{
#ifdef _NWCP_WIN32
    Sleep(ms);
#else
    cpthread_mutex_t mtx;
    cpthread_cond_t cnd;
    if(0==ms)
#ifdef _NWCP_LINUX
        pthread_yield();
#else
        sched_yield();
#endif
    else
    {
        /*cpthread_mutex_init(&mtx);
        cpthread_cond_init(&cnd);

        cpthread_mutex_lock(&mtx);
        (void) cpthread_cond_timedwait(&cnd, &mtx, ms);
        cpthread_mutex_unlock(&mtx);

        cpthread_cond_destroy(&cnd);
        cpthread_mutex_destroy(&mtx);*/
        struct timespec ts = { ms / 1000, (ms % 1000) * 1000000 };
        nanosleep(&ts, NULL);
    }
#endif
}

cpthread_id cpthread_self()
{
#ifdef _NWCP_WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

/*@}*/
