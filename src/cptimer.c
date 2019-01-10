
#include "cptimer.h"
#include "cpthread.h"
#include <string.h>
#include <stdlib.h>
#ifdef _NWCP_WIN32
#define _100NS_PER_MSECOND -10
#ifndef HAVE_NT0600_PTP_TIMER
#   include <process.h>
#endif
#else
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#ifndef SIGTIMER
#define SIGTIMER SIGALRM //SIGRTMIN //取代SIGALRM
#endif
#endif

/* 定义定时器类型 */
struct _cptimer_t
{
#ifdef _NWCP_WIN32
    HANDLE event;
#ifdef HAVE_NT0600_PTP_TIMER
    PTP_TIMER timer;
#else
    HANDLE timer;   /* Waitable Timer Handler */
    HANDLE thread;
#endif
#else
    timer_t timer;  /* POSIX Timer */
    sigset_t old_sigset; /* 保存的进程信号屏蔽字 */
    sem_t event;
#endif
//    int pid;
    int id;
    int ticket;
    cptimer_proc proc;      /* 定时器事件处理函数 */
    void *arg;             /* 定时器事件处理函数的参数 */
} ;


/*! \file  cptimer.h 
    \brief 定时器管理
*/

/*! \addtogroup STIMER 定时器操作
    \remarks 在WIN32下，采用Waitable Timer实现，在UNIX，
        采用POSIX定时器实现
*/
/*@{*/

/* 内部定义的定时器事件处理函数 
   WIN32下为Waitable Timer的事件处理函数
   UNIX下为信号SIGALRM的处理函数 */
#ifdef _NWCP_WIN32
#ifdef HAVE_NT0600_PTP_TIMER
VOID CALLBACK __cptimer_sig_proc(PTP_CALLBACK_INSTANCE pInstance,
    PVOID arg,// 函数的参数
    PTP_TIMER pTimer) // 一个指向“线程池定时器”的指针
#else
void CALLBACK __cptimer_sig_proc(void *arg, DWORD low_val, DWORD high_val)
#endif
#else
void __cptimer_sig_proc(int signo, siginfo_t *info, void *context)
#endif
{
    cptimer_t timer;
#ifdef _NWCP_WIN32
    timer = (cptimer_t )arg;
#else
    if(signo != SIGALRM)
    {
        //printf("__cptimer_sig_proc signo != SIGALRM");
        return;
    }
       
    timer = (cptimer_t )info->si_value.sival_ptr;
#endif    
    if(timer)
    {
        if(timer->proc)
        {
            timer->proc(timer->id, timer->arg);
        }
        else
        {
#ifndef _NWCP_WIN32
            sem_post(&timer->event);
#else
#ifdef HAVE_NT0600_PTP_TIMER
            if(timer->event)
            {
                SetEvent(timer->event);
            }
#endif
#endif
        }
    }
    else
    {
        //printf("__cptimer_sig_proc timer==0 || timer->proc==0");
    }
}

/* Windows平台Vista以下系统,定时器必须在其创建线程唤醒
 */
#ifdef _NWCP_WIN32
#ifndef HAVE_NT0600_PTP_TIMER
#define WIN_CPTIMER_THREAD_FAILED 44444
#define WIN_CPTIMER_THREAD_EXEEND 12345
    static void __win_cptimer_thread( void * para)
    {
        struct _cptimer_t *timer = (struct _cptimer_t *)(para);
        LARGE_INTEGER liDueTime;
        __int64 qwDueTime = 0;
        DWORD ticket = timer->ticket;
        qwDueTime = timer->ticket * _100NS_PER_MSECOND ;//第一次通知的时刻（负数表示相对值,值必须是100ns的倍数）
        liDueTime.LowPart  = (DWORD) ( qwDueTime & 0xFFFFFFFF );
        liDueTime.HighPart = (LONG)  ( qwDueTime >> 32 );
        if(!SetWaitableTimer(timer->timer, &liDueTime, ticket, __cptimer_sig_proc, timer, FALSE))
        {
             SetEvent(timer->event);
            _endthreadex(WIN_CPTIMER_THREAD_FAILED);
        }

        SetEvent(timer->event);

        while(timer->timer)
        {
            SleepEx(ticket, TRUE);
            if(WaitForSingleObject(timer->event, 0) == WAIT_OBJECT_0)
                break;
        }
        _endthreadex(WIN_CPTIMER_THREAD_EXEEND);
    }
#endif
#endif

/*! \brief 创建定时器
    \param id - 定时器ID
    \param timer - 返回创建的定时器指针
    \return CPDL_SUCCESS - 成功，CPDL_ERROR - 失败，
            CPDL_EINVAL - 参数错误
    \remarks 此函数在WIN32下创建一个Waitable Timer，
             在UNIX下创建一个POSIX定时器(POSIX.1c)。
             如果name或者timer为NULL，返回CPDL_EINVAL
    \sa ::cptimer_destroy, ::cptimer_set, ::cptimer_wait             
*/             
int cptimer_create(const int id, cptimer_t *timer)
{
#ifdef _NWCP_WIN32
#ifndef HAVE_NT0600_PTP_TIMER
    char name[256];
#endif
    HANDLE htimer = 0;
#else
    struct sigevent sigev;
#endif    
    if(0==timer)
        return CPDL_EINVAL;
#ifdef _NWCP_WIN32
#ifndef HAVE_NT0600_PTP_TIMER
    snprintf(name, 255, "cpdl_timer_p%08d_t%08ld_id%08d_%p",
             getpid(), GetCurrentThreadId(), id, timer);
    htimer = CreateWaitableTimer(NULL, FALSE, name);
    if(0==htimer)
        return CPDL_ERROR;
#endif
    *timer = (struct _cptimer_t*)malloc(sizeof(struct _cptimer_t));
#ifndef HAVE_NT0600_PTP_TIMER
    (*timer)->thread = 0;
#endif
    (*timer)->event = 0;
    (*timer)->timer = htimer;
#else
    /* 创建POSIX定时器 */
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGTIMER;
    sigev.sigev_value.sival_ptr = *timer = (struct _cptimer_t*)malloc(sizeof(struct _cptimer_t))/*timer*/;
    (*timer)->timer = 0;
    if(timer_create(CLOCK_REALTIME, &sigev, &(*timer)->timer) < 0)
    {
        free((void*)(*timer));
        return CPDL_ERROR;
    }
#endif
    //(*timer)->pid = getpid();
    (*timer)->id = id;
    return CPDL_SUCCESS;
}       

/*! \brief 设置定时器
    \param timer - 定时器指针
    \param ms - 定时器间隔(毫秒)
    \param proc - 定时器事件处理函数，可以为NULL
    \param arg - 定时器事件处理函数的参数
    \return CPDL_SUCCESS - 成功，CPDL_ERROR - 失败
    \remarks 定时器每隔\a ms毫秒发出事件(信号)，若proc非空，则调用proc函数,
        在UNIX下，如果调用了cptimer_wait函数，将不调用proc函数，在Windows下
        proc函数不被调用(原因不明)
    \sa ::cptimer_wait, ::cptimer_create, cptimer_destroy             
*/    
int cptimer_set(cptimer_t timer, int ms, cptimer_proc proc, void *arg)
{
#ifndef _NWCP_WIN32
    struct sigaction sigact;
    struct itimerspec interval;
#else
    LARGE_INTEGER liDueTime;
    __int64 qwDueTime = 0;
#ifdef HAVE_NT0600_PTP_TIMER
    FILETIME ft;
#else
    DWORD te_code = 0;
#endif
#endif
    if(0==timer)
        return CPDL_EINVAL;
    timer->proc = proc;
    timer->arg = arg;
    timer->ticket = ms;
#ifdef _NWCP_WIN32
    qwDueTime = ms * _100NS_PER_MSECOND ;//第一次通知的时刻（负数表示相对值,值必须是100ns的倍数）
    liDueTime.LowPart  = (DWORD) ( qwDueTime & 0xFFFFFFFF );
    liDueTime.HighPart = (LONG)  ( qwDueTime >> 32 );
#ifdef HAVE_NT0600_PTP_TIMER
    if(0==proc)
    {
        timer->event = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(0==timer->event)
            return CPDL_ERROR;
    }
    timer->timer = CreateThreadpoolTimer(__cptimer_sig_proc, timer, 0);
    if(0==timer->timer)
    {
        CloseHandle(timer->event);
        timer->event = 0;
        return CPDL_ERROR;
    }

    ft.dwHighDateTime = liDueTime.HighPart;
    ft.dwLowDateTime = liDueTime.LowPart;
    SetThreadpoolTimer( timer->timer,        // 一个“线程池定时器”指针
                        &ft,    // 回调函数被调用的时间
                        ms,          // 周期性调用回调函数的间隔时间（毫秒）
                        0);   // 周期时间的波动范围（毫秒）
#else
    if(proc)
    {
        timer->event = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(0==timer->event)
            return CPDL_ERROR;

        timer->thread = (HANDLE)_beginthread(__win_cptimer_thread, 0, timer);
        if(INVALID_HANDLE_VALUE == timer->thread || 0 == timer->thread)
        {
            CloseHandle(timer->event);
            timer->event = 0;
            return CPDL_ERROR;
        }

        te_code = WaitForSingleObject(timer->event, timer->ticket);
        if(WAIT_OBJECT_0 == te_code || WAIT_TIMEOUT == te_code)
        {
            if(TRUE == GetExitCodeThread(timer->thread, &te_code) &&
                    (WIN_CPTIMER_THREAD_EXEEND == te_code ||
                     WIN_CPTIMER_THREAD_FAILED == te_code) )
            {
                CloseHandle(timer->thread);
                CloseHandle(timer->event);
                timer->thread = 0;
                timer->event = 0;
                return CPDL_ERROR;
            }
        }
    }
    else
    {
        if(!SetWaitableTimer(timer->timer, &liDueTime, ms, 0, 0, FALSE))
            return CPDL_ERROR;
    }
#endif
#else
    if(0==proc)
    {
        if (sem_init(&timer->event, 0, 0) == -1)
            return CPDL_ERROR;
    }

    /* 设置信号处理函数 */
    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGTIMER);
    sigact.sa_flags = SA_SIGINFO;
    sigact.sa_sigaction = __cptimer_sig_proc;
    if(sigaction(SIGTIMER, &sigact, NULL) < 0)
        return CPDL_ERROR;
        
    interval.it_interval.tv_sec = ms / 1000;
    interval.it_interval.tv_nsec = (ms % 1000) * 1000 * 1000; 
    interval.it_value = interval.it_interval;
    if(timer_settime(timer->timer, 0, &interval, 0) < 0)
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}
/*! \brief 等待定时器事件
    \param timer - 定时器指针
    \return CPDL_SUCCESS - 成功，CPDL_ERROR - 失败
    \remarks 此函数将阻塞调用线程，直到定时器事件发生
    \sa ::cptimer_set, ::cptimer_create, cptimer_destroy
*/        
int cptimer_wait(cptimer_t timer)
{
#ifdef _NWCP_WIN32
#ifdef HAVE_NT0600_PTP_TIMER
    if(0==timer->proc && 0!=timer->event)
    {
        if(WaitForSingleObject(timer->event, INFINITE) != WAIT_OBJECT_0)
            return CPDL_ERROR;
    }
#else
    if(0==timer->proc && 0==timer->thread)
    {
        if(WaitForSingleObject(timer->timer, INFINITE) != WAIT_OBJECT_0)
            return CPDL_ERROR;
    }
#endif
#else
    struct timespec ts;
    int interrupt = 0;
    if(0==timer->proc)
    {
        do {
            clock_gettime(CLOCK_REALTIME, &ts); /* POSIX.1b时钟 */
            ts.tv_nsec += ((timer->ticket % 1000) * 1000 * 1000);
            ts.tv_sec +=  ((long)(ts.tv_nsec / 1000 / 1000 / 1000) + (long)(timer->ticket / 1000));
            ts.tv_nsec %= 1000 * 1000 * 1000;
            if (sem_timedwait(&timer->event, &ts) < 0)
            {
                if(EINTR != errno && ETIMEDOUT != errno)
                    return CPDL_ERROR;
                ++interrupt;
                if(interrupt > 5)
                {
                     printf("sem_timedwait had been interrupted %d times\n", interrupt);
                     return CPDL_ERROR;
                }
            }
            else
            {
                return CPDL_SUCCESS;
            }
        } while (1);
    }
#endif
    return CPDL_SUCCESS;
}

/*! \brief 删除定时器
    \param timer - 定时器指针
    \return CPDL_SUCCESS - 成功，CPDL_ERROR - 失败
    \sa ::cptimer_create, ::cptimer_set, ::cptimer_wait
*/    
int cptimer_destroy(cptimer_t *timer)
{
#ifdef _NWCP_WIN32
#ifndef HAVE_NT0600_PTP_TIMER
    DWORD exit_code = 0;
#endif
#endif
    if(0==timer || 0==(*timer))
        return CPDL_EINVAL;

#ifdef _NWCP_WIN32
#ifdef HAVE_NT0600_PTP_TIMER
    if((*timer)->event)
        SetEvent((*timer)->event);
    WaitForThreadpoolTimerCallbacks((*timer)->timer, TRUE);
    CloseThreadpoolTimer((*timer)->timer);
    if((*timer)->event)
        CloseHandle((*timer)->event);
#else
    if(0 != (*timer)->proc && 0 != (*timer)->thread)
    {
        SetEvent((*timer)->event);
        if(TRUE == GetExitCodeThread((*timer)->thread, &exit_code) )
        {
            while(exit_code == STILL_ACTIVE)
            {
                if(WaitForSingleObject((*timer)->thread, INFINITE/*(*timer)->ticket*/) == WAIT_OBJECT_0)
                    GetExitCodeThread((*timer)->thread, &exit_code);
            }
        }
        CloseHandle((*timer)->thread);
        CloseHandle((*timer)->event);
    }

    if((*timer)->timer)
    {
        CloseHandle((*timer)->timer);
    }
#endif
#else
    /* 恢复进程的信号屏蔽字 */
    //sigprocmask(SIG_SETMASK, &(*timer)->old_sigset, NULL);
    if(0==(*timer)->proc)
    {
        sem_post(&(*timer)->event);
        sem_destroy(&(*timer)->event);
    }
    if((*timer)->timer )
    {
        timer_delete((*timer)->timer);
    }
#endif
    free((void*)(*timer));
    return CPDL_SUCCESS;
}

/*@}*/
