
#include "cpsyslog.h"   

/*! \file  cpsyslog.h 
    \brief 事件记录日志
*/

/*! \addtogroup SSYSLOG 事件记录日志
    \remarks 提供跨平台的事件记录API，在UNIX下, 使用BSD的Syslog机制，在Windows下，
    使用Event Log机制*/
/*@{*/

/*! \brief 打开日志
    \param src - 事件源的名称
    \return 成功返回日志标识，否则返回CPDL_ERROR
    \remarks src通常为应用程序的名称，在UNIX下，此名称会成为所有事件文本的前缀。
        在Windows下，日志记录可通过Event Viewer查看，在UNIX下，日志的存放位置取
        决于/etc/syslog.conf的配置信息(Facility为USER)
    \sa ::cpsyslog, ::cpcloselog
*/
cplog_t cpopenlog(const char *src)
{
#ifdef _NWCP_WIN32
    HANDLE h;

    h = RegisterEventSource(NULL, src);
    if(!h)
        return (HANDLE)CPDL_ERROR;
    return h;
#else
    openlog(src, LOG_CONS, LOG_USER);
    return CPDL_SUCCESS;
#endif
}

/*! \brief 向日志中添加记录
    \param log - 日志标识
    \param ev_type - 事件类型，其值为CPLOG_INFO、CPLOG_WARNING、CPLOG_ERROR
        中的一个
    \param ev_id - 事件的ID，在UNIX下，无意义
	\param format - 格式描述，与sprintf函数意义相同
    \param ap - 参数列表，意义与vsprintf的参数相同
    \return 无
    \sa ::cpopenlog, ::cpcloselog, ::cpsyslog
*/
void cpvsyslog(cplog_t log, int ev_type, int ev_id, const char *format, va_list ap)
{
	char str[8192];
#ifdef _NWCP_WIN32
    //char *str1[1];
#endif

#ifdef _NWCP_WIN32
	strcpy(str, "\n");
    vsprintf(str+1, format, ap);
#else
	vsprintf(str, format, ap);
#endif

#ifdef _NWCP_WIN32
    //str1[0] = str;

    ReportEventA(log,
                ev_type,
                1,
                ev_id,
                NULL,
                1,
                0,
                (LPCSTR*)(&str),
                (PVOID)(0));
#else
    syslog(ev_type, "%s", str);
#endif
}

/*! \brief 向日志中添加记录
    \param log - 日志标识
    \param ev_type - 事件类型，其值为CPLOG_INFO、CPLOG_WARNING、CPLOG_ERROR
        中的一个
    \param ev_id - 事件的ID，在UNIX下，无意义
    \param format - 格式描述，与sprintf函数意义相同
    \return 无
    \sa ::cpopenlog, ::cpcloselog, cpvsyslog
*/
void cpsyslog(cplog_t log, int ev_type, int ev_id, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
	cpvsyslog(log, ev_type, ev_id, format, ap);
	va_end(ap);
}

/*! \brief 关闭日志
    \param log - 日志标识
    \return - 无
*/
void cpcloselog(cplog_t log)
{
#ifdef _NWCP_WIN32
    DeregisterEventSource(log);
#else
    closelog();
#endif
}

/*@}*/
