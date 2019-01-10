
#include "cptime.h"
#ifndef _NWCP_WIN32
#include <stdio.h>
#endif
#include <string.h>
#include <sys/timeb.h>

/*! \file  cptime.h 
    \brief 时间处理
*/

/*! \addtogroup CPTIME 时间处理
    \remarks 重新定义几个时间处理的C Runtime函数，在WIN32下，这些函数是
        线程安全的，在UNIX下，这些函数不是线程安全的，但有形如xxxxx_r的
        线程安全函数，本模块将这些线程安全的函数统一用cpxxxx的名称封装。
        增加一个新的函数cpgettime
*/
/*@{*/

struct tm *cpgmtime(const time_t *timer, struct tm *result)
{
#ifdef _NWCP_WIN32
    struct tm *tm_ptr;
    
    tm_ptr = gmtime(timer);
    memcpy(result, tm_ptr, sizeof(struct tm));
    return result;
#else
    return gmtime_r(timer, result);
#endif
}

struct tm *cplocaltime(const time_t *timer, struct tm *result)
{
#ifdef _NWCP_WIN32
    struct tm *tm_ptr;

    tm_ptr = localtime(timer);
    memcpy(result, tm_ptr, sizeof(struct tm));
    return result;
#else
    return localtime_r(timer, result);
#endif
}

char *cpasctime(const struct tm *tmptr, char *buf)
{
#ifdef _NWCP_WIN32
    char *ptr;
    
    ptr = asctime(tmptr);
    strcpy(buf, ptr);
    return buf;
#elif _NWCP_SUNOS
    return asctime_r(tmptr, buf, 26);
#else
    return asctime_r(tmptr, buf);
#endif
}

char* cpctime(const time_t *timer, char *buf)
{
#ifdef _NWCP_WIN32
    char *ptr;
    
    ptr = ctime(timer);
    strcpy(buf, ptr);
    return buf;
#elif _NWCP_SUNOS
    return ctime_r(timer, buf, 26);
#else
    return ctime_r(timer, buf);
#endif
}     

/*! \brief 获取当前系统时间
    \param tmptr - 指向cp_tm结构的指针，返回当前时间
    \return 无
    \remarks 精度到毫秒
*/
void cpgettime(cp_tm *tmptr)       
{
#ifdef _NWCP_WIN32
    struct _timeb tmb;
#else
    struct timeb tmb;
#endif
    //struct tm t1;

#ifdef _NWCP_WIN32
    _ftime(&tmb);
#else
    ftime(&tmb);
#endif
    cplocaltime(&(tmb.time), &tmptr->tm_s);
    tmptr->tm_ms = tmb.millitm;
}

/*! \brief 设置当前系统时间
    \param tmptr - 指向cp_tm结构的指针，值为欲设置的时间
    \return 无
    \remarks 精度到毫秒
*/
void cpsettime(cp_tm *tmptr)
{
#ifdef _NWCP_WIN32
    SYSTEMTIME sycptime;
    sycptime.wYear = tmptr->tm_s.tm_year + 1900;
    sycptime.wMonth = tmptr->tm_s.tm_mon + 1;
    sycptime.wDay = tmptr->tm_s.tm_mday;
    sycptime.wHour = tmptr->tm_s.tm_hour;
    sycptime.wMinute = tmptr->tm_s.tm_min;
    sycptime.wSecond = tmptr->tm_s.tm_sec;
    sycptime.wMilliseconds = tmptr->tm_ms;
    SetLocalTime(&sycptime);
#else
    struct timespec tp;
    tp.tv_sec = cpmktime(&tmptr->tm_s);
    tp.tv_nsec = tmptr->tm_ms * 1000 * 1000;
    clock_settime(CLOCK_REALTIME, &tp);
#endif
}

/*! \brief 获取两个时间之间的时间差（秒）
    \param start - 现在的某个时间  end - 过去的某个时间
    \param end - 过去的某个时间
    \return 从end到start的秒数
    \remarks 精度到毫秒
*/
double cpdifftime( cp_tm *start, cp_tm *end)
{
    return difftime(cpmktime(&start->tm_s), cpmktime(&end->tm_s)) +
            (double)(start->tm_ms - end->tm_ms) / 1000.0;
}

/*! \brief 获取两个时间之间的时间差（毫秒）
    \param start - 现在的某个时间  end - 过去的某个时间
    \param end - 过去的某个时间
    \return 从end到start的秒数
    \remarks 精度到毫秒
*/
double cpdifftime_ms( cp_tm *start, cp_tm *end)
{
    return difftime(cpmktime(&start->tm_s), cpmktime(&end->tm_s)) * 1000 +
            (start->tm_ms - end->tm_ms);
}

/*! \brief 字符串转换成时间
    \param strtime - 字符串格式时间，必须为CP_TM_STRING_FORMAT
    \param t - 转换后的时间存放区（当strtime为空时获取当前时间）
    \return strtime为空或时间字符串格式不对时返回0，否则返回t
    \remarks 精度到毫秒
*/
cp_tm *cpcstr2time(const char *strtime, cp_tm *t)
{
    if(strtime==0 || strlen(strtime)==0)
    {
        cpgettime(t);
        return 0;
    }
    else
    {
        if( 7 != sscanf(strtime, CP_TM_STRING_FORMAT, &t->tm_s.tm_year,
                        &t->tm_s.tm_mon, &t->tm_s.tm_mday, &t->tm_s.tm_hour,
                        &t->tm_s.tm_min, &t->tm_s.tm_sec, &t->tm_ms) )
        {
            return 0;
        }
        t->tm_s.tm_year -= 1900;
        t->tm_s.tm_mon -= 1;
    }
    return t;
}

/*! \brief 取得操作系统启动所经过（elapsed）的毫秒数
    \return 取得系统运行至当前的TICKS
    \remarks 精度到毫秒
*/
unsigned int cpgetticks()
{
#ifdef _NWCP_WIN32
    static LARGE_INTEGER perfCounterFreq = {{0, 0}};
    if(perfCounterFreq.QuadPart == 0)
    {
        if(QueryPerformanceFrequency(&perfCounterFreq) == FALSE)
        {
            //looks like the system does not support high resolution tick counter
            return GetTickCount();
        }
    }
    LARGE_INTEGER ticks;
    if(QueryPerformanceCounter(&ticks) == FALSE)
    {
        return GetTickCount();
    }
    return (unsigned int)((ticks.QuadPart * 1000) / perfCounterFreq.QuadPart);
#elif _NWCP_LINUX
    timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
    {
        (unsigned int)(-1);
    }

    return (unsigned int)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#else
    //Mac os X doesn't support clock_gettime
    timeval t;
    gettimeofday(&t, 0);
    return (unsigned int)(t.tv_sec * 1000 + t.tv_usec / 1000);
#endif
}

/*@
* 时间转换成字符串
* 字符串格式必须为CP_TM_STRING_FORMAT
*/

/*@
* 字符串转换成时间
* 字符串格式必须为CP_TM_STRING_FORMAT
*/
/*! \brief 时间转换成字符串
    \param strtime - 字符串格式时间存放缓冲区
    \param t - 需要转换的时间
    \return 返回strtime
    \remarks 精度到毫秒
*/
char* cptime2cstr(char *strtime, const cp_tm *t)
{
    sprintf(strtime,  CP_TM_STRING_FORMAT, t->tm_s.tm_year+1900, t->tm_s.tm_mon+1,
            t->tm_s.tm_mday, t->tm_s.tm_hour, t->tm_s.tm_min, t->tm_s.tm_sec, t->tm_ms);
    return strtime;
}


/*@}*/
