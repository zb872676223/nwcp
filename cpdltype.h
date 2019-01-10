/* CPDL中的公用类型定义
*/

#ifndef _CPDLTYPE_H
#define _CPDLTYPE_H

#ifdef _NWCP_WIN32
#   include <winsock2.h>
#   ifdef _MSC_VER
#       pragma warning(disable:4251)
#       pragma warning(disable:4786)
#       pragma warning(disable:4819)
#       pragma warning(disable:4996)
#      if _MSC_VER <= 1200
#          define _WIN32_WINNT 0x0500
#      endif
#   endif
//#   include <windows.h>
#   ifndef _WIN32_WINNT
#       define _WIN32_WINNT 0x0600 // _WIN32_WINNT_WINXP
#   endif
#   if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)  /* Vista及以上版本 */
#       define HAVE_NT0600_PTP_TIMER 1
//#       define HAVE_NT0600_SRWLOCK 1
#   endif
#   include <stdio.h>
#   define snprintf _snprintf
#elif defined(_NWCP_LINUX) || defined(_NWCP_SUNOS)
#   include <pthread.h>
#   ifdef _NWCP_LINUX
#      include <bits/posix_opt.h>
#   endif
#   ifdef _POSIX_THREAD_PROCESS_SHARED
#       define _CPDL_USE_POSIX_THREAD_PROCESS_SHARED 1
#       define _CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM 1
#   endif
#   ifndef _POSIX_C_SOURCE
#       define _POSIX_C_SOURCE 200112L
#   endif
#   ifndef _XOPEN_SOURCE
#       define _XOPEN_SOURCE 600
#   endif
#elif _NWCP_IBMOS

#endif


#ifdef _NWCP_WIN32
#   ifdef CPDLDLL_DEF
#       define CPDLAPI __declspec(dllexport)
#   else
#       define CPDLAPI __declspec(dllimport)
#   endif
#else
#   define CPDLAPI
#endif  /* _NWCP_WIN32 */

/* 定义函数的返回值 */
#define CPDL_SUCCESS  0 /* 调用成功 */
#define CPDL_AEXIST   1 /* 数据已经存在或仍然存在(非错误) */
#define CPDL_ERROR   -1 /* 调用失败 */
#define CPDL_EINVAL  -2 /* 参数无效 */
#define CPDL_NODATA  -3 /* 访问无效 */
#define CPDL_TIMEOUT -4 /* 等待超时 */

#define CPDL_TIME_INFINITE  -1 /* 无限等待 */


/* for cpthread.h begin */
/* 定义线程ID类型 */
#ifdef _NWCP_WIN32
typedef DWORD cpthread_id;
/* 定义线程ID类型 */
typedef HANDLE cpthread_t;
/* 定义线程互斥对象 */
typedef CRITICAL_SECTION cpthread_mutex_t;
/* 定义线程的入口函数类型 */
typedef void (* CPTHREAD_ROUTINE)(void*);
/* 定义线程条件变量 */
typedef HANDLE cpthread_cond_t;
#else
typedef pthread_t cpthread_id;
typedef void* cpthread_result_t;
/* 定义线程ID类型 */
typedef pthread_t cpthread_t;
/* 定义线程互斥对象 */
typedef pthread_mutex_t cpthread_mutex_t;
/* 定义线程的入口函数类型 */
typedef void* (* CPTHREAD_ROUTINE)(void*);
/* 定义线程条件变量 */
typedef pthread_cond_t cpthread_cond_t;
#endif  /* _NWCP_WIN32 */
/* for cpthread.h end */


/* for cptimer.h begin */
typedef struct _cptimer_t* cptimer_t;
typedef void (*cptimer_proc)(int id, void* arg); /* 定时器事件处理函数 */
/* for cptimer.h end */


/* for cpshm.h begin */
typedef struct _cpshm_t* cpshm_t;
typedef char*           cpshm_d;
/* for cpshm.h end */


/* for cpprocsem.h begin */
/* 定义有名信号量和互斥量类型 */
#ifdef _NWCP_WIN32
typedef HANDLE cpsem_t;
typedef HANDLE cpmutex_t;
#else
#ifdef _CPDL_USE_POSIX_THREAD_ID_NAMEDBY_SHM
extern const unsigned int CPDLAPI CPMUTEX_EX_SHM_SIZE;
extern const unsigned int CPDLAPI CPSEM_EX_SHM_SIZE;
#endif
typedef struct cpposix_mutex* cpmutex_t;
typedef struct cpposix_sem* cpsem_t;
#endif
/* for cpprocsem.h end */


/* for cprwlock.h begin */
#ifdef _NWCP_WIN32
/*
#   ifdef HAVE_NT0600_SRWLOCK
typedef SRWLOCK* cprwlock_t;
#   else
*/
typedef struct tagCPRWLOCK* cprwlock_t;
/*
#   endif
*/
#else
typedef pthread_rwlock_t* cprwlock_t;
#endif
typedef struct tagCPPSRWLOCK* cpprwlock_t;
/* for cprwlock.h end */


/* for cpshmmq.h begin */
/* sending priority options for sending a message */
#define CPSHMMQ_PRI_NORMAL  0x0000  /* put the message at the end of the queue */
#define CPSHMMQ_PRI_URGENT  0x0001  /* put the message at the frond of the queue */
#define CPSHMMQ_NAME_LEN    64
typedef struct _cpmq_t* cpshmmq_t;
/* for cpshmmq.h end */


/* for cpprocess.h end */
#if defined(_NWCP_WIN32) && !defined(__MINGW32__)
typedef DWORD cppid_t;
#else
#include <sys/types.h>
typedef pid_t cppid_t;
#endif
/* for cpprocess.h end */


/* for cpdll.h begin*/
#ifdef _NWCP_WIN32
typedef HMODULE cpdll_t;
typedef FARPROC cpdlsym_t;
#else
typedef void* cpdll_t;
typedef void* cpdlsym_t;
#endif
/* for cpdll.h end*/


/* for tcp or spibus communication begin */
enum tagCPIOPending
{
    pendingNone = 0,
    pendingInput,  //是否可读
    pendingOutput, //是否可写
    pendingError   //是否错误
};
typedef enum tagCPIOPending CPIOPending;

/* for cpspibus.h : serial port begin*/
typedef struct _cpspirs_t* cpspirs_t;

enum  tagCPSPIRS_BR
{
    RS_BR110 = 0,
    RS_BR300,
    RS_BR600,
    RS_BR1200,
    RS_BR2400,
    RS_BR4800,
    RS_BR9600,
    RS_BR14400,
    RS_BR19200,
    RS_BR38400,
    RS_BR56000,
    RS_BR57600,
    RS_BR115200,
    RS_BR128000,
    RS_BR256000
};
typedef enum tagCPSPIRS_BR CPSPIRS_BR;

enum tagCPSPIRS_PARITY
{
    RS_PARITY_NONE = 0,
    RS_PARITY_ODD,
    RS_PARITY_EVEN
};
typedef enum tagCPSPIRS_PARITY CPSPIRS_PARITY;

enum tagCPSPIRS_FLOW
{
    RS_FLOW_NONE = 0,
    RS_FLOW_SOFT,
    RS_FLOW_HARD,
    RS_FLOW_BOTH
};
typedef enum tagCPSPIRS_FLOW CPSPIRS_FLOW;

enum tagCPSPIRS_DBITS
{
    RS_DATABITS_5 = 0,
    RS_DATABITS_6,
    RS_DATABITS_7,
    RS_DATABITS_8
};
typedef enum tagCPSPIRS_DBITS CPSPIRS_DBITS;

enum tagCPSPIRS_SBITS
{
    RS_STOPBITS_1 = 1,
    RS_STOPBITS_2,
};
typedef enum tagCPSPIRS_SBITS CPSPIRS_SBITS;
/* for cpspibus.h : serial port end */

/* for tcp or spibus communication end*/

#endif  /* not _CPDLTYPE_H */
