#ifndef _CPSYCPLOG_H
#define _CPSYCPLOG_H

#include "cpdltype.h"
#include <stdarg.h>
#ifdef _NWCP_WIN32
#   include<windows.h>
#else
#   include <stdio.h>
#   include<syslog.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _NWCP_WIN32
typedef HANDLE cplog_t;
#else
typedef int cplog_t; /* in fact UNIX need not this */
#endif

#ifdef _NWCP_WIN32
#   define CPLOG_INFO EVENTLOG_INFORMATION_TYPE
#   define CPLOG_WARNING EVENTLOG_WARNING_TYPE
#   define CPLOG_ERROR EVENTLOG_ERROR_TYPE
#else
#   define CPLOG_INFO LOG_INFO
#   define CPLOG_WARNING LOG_WARNING
#   define CPLOG_ERROR LOG_ERR
#endif

extern cplog_t CPDLAPI cpopenlog(const char* src);
extern void CPDLAPI cpsyslog(cplog_t log, int ev_type, int ev_id, const char* format, ...);
extern void CPDLAPI cpvsyslog(cplog_t log, int ev_type, int ev_id,
                              const char* format, va_list ap);
extern void CPDLAPI cpcloselog(cplog_t log);

#ifdef __cplusplus
}
#endif

#endif /* not _CPSYCPLOG_H */
