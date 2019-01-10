#ifndef _CPTIME_H
#define _CPTIME_H

#include "cpdltype.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define cptime time
#define cpmktime mktime
#define cpstrftime strftime

extern CPDLAPI struct tm* cpgmtime(const time_t* timer, struct tm* result);
extern CPDLAPI struct tm* cplocaltime(const time_t* timer, struct tm* result);
extern char CPDLAPI* cpasctime(const struct tm* tmptr, char* buf);
extern char CPDLAPI* cpctime(const time_t* timer, char* buf);
extern CPDLAPI unsigned int cpgetticks();

/* 扩展原有的struct tm，使其包括毫秒 */
typedef struct _cp_tm
{
    struct tm tm_s;
    int    tm_ms;      /* milliseconds after the second - [0,999] */
} cp_tm;

#define CP_TM_STRING_FORMAT "%04d-%02d-%02d %02d:%02d:%02d.%03d"
#define CP_TM_STRING_LENGTH   24

extern void CPDLAPI cpgettime(cp_tm* tmptr);
extern void CPDLAPI cpsettime(cp_tm* tmptr);
/* 计算两个时间差(单位为秒) */
/* start比end新返回正值 */
extern double CPDLAPI cpdifftime(cp_tm* start, cp_tm* end);
extern double CPDLAPI cpdifftime_ms(cp_tm* start, cp_tm* end);
/* 字符串转换成时间(单位为秒)格式必须为CP_TM_STRING_FORMAT */
extern CPDLAPI cp_tm* cpcstr2time(const char* strtime, cp_tm* time);
/* 时间转换成字符串 */
extern char CPDLAPI* cptime2cstr(char* strtime, const cp_tm* time);


#ifdef __cplusplus
}
#endif

#endif /* not _CPTIME_H */
