#include "../cpspibus.h"

#include <stdlib.h>
#include <string.h>
#ifndef _NWCP_WIN32
#  ifdef _NWCP_SUNOS
#    define __EXTENSIONS__ 1
#  endif
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
//#include <sys/select.h>
#include <termios.h>
#include <fcntl.h>
#endif

#ifndef	MAX_INPUT
#define	MAX_INPUT 255
#endif

#ifndef MAX_CANON
#define MAX_CANON MAX_INPUT
#endif

#ifdef	__FreeBSD__
#undef	_PC_MAX_INPUT
#undef	_PC_MAX_CANON
#endif

#if defined(__QNX__)
#define CRTSCTS (IHFLOW | OHFLOW)
#endif

#if defined(_THR_UNIXWARE) || defined(__hpux) || defined(_AIX)
#include <sys/termiox.h>
#define	CRTSCTS	(CTSXON | RTSXOFF)
#endif

// IRIX

#ifndef	CRTSCTS
#ifdef	CNEW_RTSCTS
#define	CRTSCTS (CNEW_RTSCTS)
#endif
#endif

#if defined(CTSXON) && defined(RTSXOFF) && !defined(CRTSCTS)
#define	CRTSCTS (CTSXON | RTSXOFF)
#endif

#ifndef	CRTSCTS
#define	CRTSCTS	0
#endif


/*! \file  cpspibus.c
    \brief 串行接口通信封装
*/

/*! \addtogroup CPSPIBUS 串行接口通信
    \remarks  */
/*@{*/

static const unsigned int  CPSPIRS_BR_MAP[] = {
#ifdef _NWCP_WIN32
    CBR_110,
    CBR_300,
    CBR_600,
    CBR_1200,
    CBR_2400,
    CBR_4800,
    CBR_9600,
    CBR_14400,
    CBR_19200,
    CBR_38400,
    CBR_56000,
    CBR_57600,
    CBR_115200,
    CBR_128000,
    CBR_256000
#else
    B110,
    B300,
    B600,
    B1200,
    B2400,
    B4800,
    B9600,
#ifdef B14400
    B14400,
#else
    14400,
#endif
    B19200,
    B38400,
#ifdef B56000
    B56000,
#else
        56000,
#endif
    B57600,
    B115200,
#ifdef B128000
    128000,
#else
     56000,
#endif
#ifdef B256000
    B256000
#else
    256000,
#endif
#endif
};

static const unsigned int CPSPIRS_PARITY_MAP[] = {
    #ifdef _NWCP_WIN32
    NOPARITY,
    ODDPARITY,
    EVENPARITY
    #else
    /*RS_PARITY_NONE*/ 0,
    (PARENB | PARODD),
    PARENB
    #endif
};

static const unsigned int CPSPIRS_DBITS_MAP[] = {
    #ifdef _NWCP_WIN32
    5,
    6,
    7,
    8
    #else
    CS5,
    CS6,
    CS7,
    CS8
    #endif
};

#ifdef _NWCP_WIN32
typedef HANDLE cp_rs_dev;
typedef DCB    cp_rs_dcb;
#   define CPRS_INVALID_HANDLE INVALID_HANDLE_VALUE
#else
typedef int    cp_rs_dev;
typedef struct termios    cp_rs_dcb;
#   define CPRS_INVALID_HANDLE -1
#endif

struct _cpspirs_t
{
    cp_rs_dev rs_dev;
    cp_rs_dcb rs_dcb_original;
    cp_rs_dcb rs_dcb_current;
};

/*! \brief 打开串口(串口实例信息)
    \param spirs - 串口号
    \param spirs_t - 串口实例句柄指针(当确定不再使用时需要调用::cpshm_close销毁)
    \return CPDL_SUCCESS - 成功    CPDL_ERROR - 失败
    \sa ::cpspirs_close, ::cpspirs_read, ::cpspirs_write, ::cpspirs_status
*/
int cpspirs_open(const char *spirs, cpspirs_t *spirs_t)
{
    cp_rs_dev rsdev = 0;
    cp_rs_dcb rsdcb_original;
    cp_rs_dcb rsdcb_current;
#ifndef	_NWCP_WIN32
#if defined(TIOCM_RTS) && defined(TIOCMODG)
    int mcs = 0;
#endif
    long ioflags = 0;
    int cflags = O_RDWR | O_NDELAY;
    rsdev = open(spirs, cflags);
#else
    // open COMM device
    rsdev = CreateFileA(spirs,
                        GENERIC_READ | GENERIC_WRITE,
                        0,                    // exclusive access
                        NULL,                 // no security attrs
                        OPEN_EXISTING,
                        FILE_FLAG_OVERLAPPED |
                        FILE_ATTRIBUTE_NORMAL |
                        FILE_FLAG_WRITE_THROUGH |
                        FILE_FLAG_NO_BUFFERING,
                        NULL);
#endif
    if(rsdev == CPRS_INVALID_HANDLE)
        return CPDL_ERROR;

#ifndef	_NWCP_WIN32
    ioflags = fcntl(rsdev, F_GETFL);
    tcgetattr(rsdev, &rsdcb_original);
    memcpy(&rsdcb_current, &rsdcb_original, sizeof(cp_rs_dcb));

    rsdcb_current.c_oflag = rsdcb_current.c_lflag = 0;
    rsdcb_current.c_cflag = CLOCAL | CREAD | HUPCL;
    rsdcb_current.c_iflag = IGNBRK;
    memset(rsdcb_current.c_cc, 0, sizeof(rsdcb_current.c_cc));
    rsdcb_current.c_cc[VMIN] = 1;
    // inherit original settings, maybe we should keep more??
    cfsetispeed(&rsdcb_current, cfgetispeed(&rsdcb_original));
    cfsetospeed(&rsdcb_current, cfgetospeed(&rsdcb_original));
    rsdcb_current.c_cflag |= ( rsdcb_original.c_cflag &
                               (CRTSCTS | CSIZE | PARENB | PARODD | CSTOPB) );
    rsdcb_current.c_iflag |= ( rsdcb_original.c_iflag &
                               (IXON | IXANY | IXOFF) );

    tcsetattr(rsdev, TCSANOW, &rsdcb_current);
    fcntl(rsdev, F_SETFL, ioflags & ~O_NDELAY);
#if defined(TIOCM_RTS) && defined(TIOCMODG)
    ioctl(rsdev, TIOCMODG, &mcs);
    mcs |= TIOCM_RTS;
    ioctl(rsdev, TIOCMODS, &mcs);
#endif

#ifdef	_COHERENT
    ioctl(rsdev, TIOCSRTS, 0);
#endif

#else // for windows
    rsdcb_current.DCBlength = sizeof(cp_rs_dcb);
    rsdcb_original.DCBlength = sizeof(cp_rs_dcb);
    GetCommState(rsdev, &rsdcb_original);
    memcpy(&rsdcb_current, &rsdcb_original, sizeof(cp_rs_dcb));
#endif
    *spirs_t = (struct _cpspirs_t*)malloc(sizeof(struct _cpspirs_t));
    (*spirs_t)->rs_dev = rsdev;
    memcpy(&(*spirs_t)->rs_dcb_original, &rsdcb_original, sizeof(cp_rs_dcb));
    memcpy(&(*spirs_t)->rs_dcb_current, &rsdcb_current, sizeof(cp_rs_dcb));
    return CPDL_SUCCESS;
}


/*! \brief 设置串口参数
    \param spirs_t - 串口实例句柄指针
    \param br - 波特率
    \param parity - 校验
    \param flow - 流控制
    \param bits - 数据位
    \param stopbits - 停止位
    \return CPDL_SUCCESS - 存在, CPDL_ERROR - 失败
    \sa ::cpspirs_open, ::cpspirs_read, ::cpspirs_write
*/
int cpspirs_set(cpspirs_t spirs_t, const CPSPIRS_BR br,
                   const CPSPIRS_PARITY parity, const CPSPIRS_FLOW flow,
                   const CPSPIRS_DBITS bits, const CPSPIRS_SBITS stopbits)
{
#ifndef _NWCP_WIN32
    cfsetispeed(&spirs_t->rs_dcb_current, CPSPIRS_BR_MAP[br]);
    cfsetospeed(&spirs_t->rs_dcb_current, CPSPIRS_BR_MAP[br]);
    spirs_t->rs_dcb_current.c_cflag &= ~(PARENB | PARODD | CSIZE | CRTSCTS | CSTOPB);
    spirs_t->rs_dcb_current.c_iflag &= ~(IXON | IXANY | IXOFF);
    spirs_t->rs_dcb_current.c_cflag |= CPSPIRS_DBITS_MAP[bits];
    switch(parity)
    {
    case RS_PARITY_EVEN:
    case RS_PARITY_ODD:
        spirs_t->rs_dcb_current.c_cflag |= CPSPIRS_PARITY_MAP[parity];
        break;
    case RS_PARITY_NONE:
    default:
        break;
    }
    switch(flow)
    {
    case RS_FLOW_SOFT:
        spirs_t->rs_dcb_current.c_iflag |= (IXON | IXANY | IXOFF);
        break;
    case RS_FLOW_BOTH:
        spirs_t->rs_dcb_current.c_iflag |= (IXON | IXANY | IXOFF);
    case RS_FLOW_HARD:
        spirs_t->rs_dcb_current.c_cflag |= CRTSCTS;
        break;
    case RS_FLOW_NONE:
    default:
        break;
    }
    switch(stopbits)
    {
    case  RS_STOPBITS_2:
        spirs_t->rs_dcb_current.c_cflag |= CSTOPB;
        break;
    case  RS_STOPBITS_1:
    default:
        break;
    }
    if(tcsetattr(spirs_t->rs_dev, TCSANOW, &spirs_t->rs_dcb_current)==-1)
        return CPDL_ERROR;
#else
#define ASCII_XON       0x11
#define ASCII_XOFF      0x13
    spirs_t->rs_dcb_current.BaudRate = CPSPIRS_BR_MAP[br];
    spirs_t->rs_dcb_current.Parity = CPSPIRS_PARITY_MAP[parity];
    spirs_t->rs_dcb_current.XonChar = ASCII_XON;
    spirs_t->rs_dcb_current.XoffChar = ASCII_XOFF;
    spirs_t->rs_dcb_current.XonLim = 100;
    spirs_t->rs_dcb_current.XoffLim = 100;
    spirs_t->rs_dcb_current.ByteSize = CPSPIRS_DBITS_MAP[bits];
    switch(flow)
    {
    case RS_FLOW_SOFT:
        spirs_t->rs_dcb_current.fInX = 1;
        spirs_t->rs_dcb_current.fOutX = 1;
        break;
    case RS_FLOW_BOTH:
        spirs_t->rs_dcb_current.fInX = 1;
        spirs_t->rs_dcb_current.fOutX = 1;
    case RS_FLOW_HARD:
        spirs_t->rs_dcb_current.fOutxCtsFlow = 1;
        spirs_t->rs_dcb_current.fRtsControl = RTS_CONTROL_HANDSHAKE;
        break;
    case RS_FLOW_NONE:
    default:
        break;
    }
    switch(stopbits)
    {
    case RS_STOPBITS_2:
        spirs_t->rs_dcb_current.StopBits = TWOSTOPBITS;
        break;
    case RS_STOPBITS_1:
    default:
        spirs_t->rs_dcb_current.StopBits = ONESTOPBIT;
        break;
    }
    if(FALSE==SetCommState(spirs_t->rs_dev, &spirs_t->rs_dcb_current))
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}
int cpspirs_setbaudrate(cpspirs_t spirs_t, const CPSPIRS_BR br)
{
#ifndef _NWCP_WIN32
    cfsetispeed(&spirs_t->rs_dcb_current, CPSPIRS_BR_MAP[br]);
    cfsetospeed(&spirs_t->rs_dcb_current, CPSPIRS_BR_MAP[br]);
    if(tcsetattr(spirs_t->rs_dev, TCSANOW, &spirs_t->rs_dcb_current)==-1)
        return CPDL_ERROR;
#else
#define ASCII_XON       0x11
#define ASCII_XOFF      0x13
    spirs_t->rs_dcb_current.BaudRate = CPSPIRS_BR_MAP[br];
    if(FALSE==SetCommState(spirs_t->rs_dev, &spirs_t->rs_dcb_current))
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

int cpspirs_setparity(cpspirs_t spirs_t, const CPSPIRS_PARITY parity)
{
#ifndef _NWCP_WIN32
    spirs_t->rs_dcb_current.c_cflag &= ~(PARENB | PARODD);
    switch(parity)
    {
    case RS_PARITY_EVEN:
    case RS_PARITY_ODD:
        spirs_t->rs_dcb_current.c_cflag |= CPSPIRS_PARITY_MAP[parity];
        break;
    case RS_PARITY_NONE:
    default:
        break;
    }
    if(tcsetattr(spirs_t->rs_dev, TCSANOW, &spirs_t->rs_dcb_current)==-1)
        return CPDL_ERROR;
#else
    spirs_t->rs_dcb_current.Parity = CPSPIRS_PARITY_MAP[parity];
    if(FALSE==SetCommState(spirs_t->rs_dev, &spirs_t->rs_dcb_current))
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

int cpspirs_setflow(cpspirs_t spirs_t, const CPSPIRS_FLOW flow)
{
#ifndef _NWCP_WIN32
    spirs_t->rs_dcb_current.c_cflag &= ~CRTSCTS;
    spirs_t->rs_dcb_current.c_iflag &= ~(IXON | IXANY | IXOFF);
//    spirs_t->rs_dcb_current.c_cflag |= CPSPIRS_DBITS_MAP[bits];
    switch(flow)
    {
    case RS_FLOW_SOFT:
        spirs_t->rs_dcb_current.c_iflag |= (IXON | IXANY | IXOFF);
        break;
    case RS_FLOW_BOTH:
        spirs_t->rs_dcb_current.c_iflag |= (IXON | IXANY | IXOFF);
    case RS_FLOW_HARD:
        spirs_t->rs_dcb_current.c_cflag |= CRTSCTS;
        break;
    case RS_FLOW_NONE:
    default:
        break;
    }
    if(tcsetattr(spirs_t->rs_dev, TCSANOW, &spirs_t->rs_dcb_current)==-1)
        return CPDL_ERROR;
#else
#define ASCII_XON       0x11
#define ASCII_XOFF      0x13
    spirs_t->rs_dcb_current.XonChar = ASCII_XON;
    spirs_t->rs_dcb_current.XoffChar = ASCII_XOFF;
    spirs_t->rs_dcb_current.XonLim = 100;
    spirs_t->rs_dcb_current.XoffLim = 100;
    switch(flow)
    {
    case RS_FLOW_SOFT:
        spirs_t->rs_dcb_current.fInX = 1;
        spirs_t->rs_dcb_current.fOutX = 1;
        break;
    case RS_FLOW_BOTH:
        spirs_t->rs_dcb_current.fInX = 1;
        spirs_t->rs_dcb_current.fOutX = 1;
    case RS_FLOW_HARD:
        spirs_t->rs_dcb_current.fOutxCtsFlow = 1;
        spirs_t->rs_dcb_current.fRtsControl = RTS_CONTROL_HANDSHAKE;
        break;
    case RS_FLOW_NONE:
    default:
        break;
    }
    if(FALSE==SetCommState(spirs_t->rs_dev, &spirs_t->rs_dcb_current))
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

int cpspirs_setdatabits(cpspirs_t spirs_t, const CPSPIRS_DBITS bits)
{
#ifndef _NWCP_WIN32
    spirs_t->rs_dcb_current.c_cflag &= ~CSIZE;
    spirs_t->rs_dcb_current.c_cflag |= CPSPIRS_DBITS_MAP[bits];
    if(tcsetattr(spirs_t->rs_dev, TCSANOW, &spirs_t->rs_dcb_current)==-1)
        return CPDL_ERROR;
#else
    spirs_t->rs_dcb_current.ByteSize = CPSPIRS_DBITS_MAP[bits];
    if(FALSE==SetCommState(spirs_t->rs_dev, &spirs_t->rs_dcb_current))
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}

int cpspirs_setstopbits(cpspirs_t spirs_t, const CPSPIRS_SBITS stopbits)
{
#ifndef _NWCP_WIN32
    spirs_t->rs_dcb_current.c_cflag &= ~CSTOPB;
    switch(stopbits)
    {
    case RS_STOPBITS_2:
        spirs_t->rs_dcb_current.c_cflag |= CSTOPB;
        break;
    case RS_STOPBITS_1:
    default:
        break;
    }
    if(tcsetattr(spirs_t->rs_dev, TCSANOW, &spirs_t->rs_dcb_current)==-1)
        return CPDL_ERROR;
#else
    switch(stopbits)
    {
    case RS_STOPBITS_2:
        spirs_t->rs_dcb_current.StopBits = TWOSTOPBITS;
        break;
    case RS_STOPBITS_1:
    default:
        spirs_t->rs_dcb_current.StopBits = ONESTOPBIT;
        break;
    }
    if(FALSE==SetCommState(spirs_t->rs_dev, &spirs_t->rs_dcb_current))
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}


/*! \brief 关闭串口
    \param spirs_t - 串口实例句柄指针
    \return CPDL_SUCCESS - 成功   CPDL_ERROR - 失败
    \sa ::cpspirs_open
*/
int cpspirs_close(cpspirs_t *spirs_t)
{
#ifndef	_NWCP_WIN32
    tcsetattr((*spirs_t)->rs_dev, TCSANOW, &(*spirs_t)->rs_dcb_original);
#else
    DWORD dwError;
    COMSTAT cs;
    SetCommState((*spirs_t)->rs_dev, &(*spirs_t)->rs_dcb_original);
    PurgeComm((*spirs_t)->rs_dev, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
    SetCommMask((*spirs_t)->rs_dev, 0);
    if(GetLastError())// asynchronous
    {
        ClearCommError((*spirs_t)->rs_dev, &dwError, &cs);
    }
    CloseHandle((*spirs_t)->rs_dev);
#endif
    free(*spirs_t);
    *spirs_t = 0;
    return CPDL_SUCCESS;
}

/*! \brief 检测串口状态
    \param spirs_t - 串口实例句柄指针
    \return pending - 参看CPIOPending的定义
    \return CPDL_SUCCESS - 成功, CPDL_EINVAL - 参数错误, CPDL_ERROR - 失败
    \sa ::cpspirs_open, ::cpspirs_read, ::cpspirs_write
*/
int cpspirs_state(cpspirs_t spirs_t, const CPIOPending pending, const int timeout)
{
#ifndef _NWCP_WIN32
    int status;
    struct timeval tv;
    fd_set grp;
    struct timeval *tvp = &tv;

    if(timeout == 0)
        tvp = NULL;
    else
    {
        tv.tv_usec = (timeout % 1000) * 1000;
        tv.tv_sec = timeout / 1000;
    }

    FD_ZERO(&grp);
    FD_SET(spirs_t->rs_dev, &grp);
    switch(pending)
    {
    case pendingInput:
        status = select(spirs_t->rs_dev + 1, &grp, NULL, NULL, tvp);
        break;
    case pendingOutput:
        status = select(spirs_t->rs_dev + 1, NULL, &grp, NULL, tvp);
        break;
    case pendingError:
        status = select(spirs_t->rs_dev + 1, NULL, NULL, &grp, tvp);
        break;
    case pendingNone:
    default:
        return pendingNone;
    }
    if(status < 0)
        return CPDL_ERROR;

    if(0!=status && FD_ISSET(spirs_t->rs_dev, &grp))
        return pending;
#else
    BOOL ret;
    DWORD	dwError;
    COMSTAT	cs;
    OVERLAPPED ol;
    DWORD dwMask = EV_ERR;
    ret = ClearCommError(spirs_t->rs_dev, &dwError, &cs);
    if(timeout == 0 || pending == pendingError)
    {
        if(FALSE==ret)
            return CPDL_ERROR;
        switch(pending)
        {
        case pendingInput:
            return cs.cbInQue != 0 ? pendingInput : pendingNone;
        case pendingOutput:
            return cs.cbOutQue == 0 ? pendingOutput : pendingNone;
        case pendingError:
            return dwError !=0 ? pendingError : pendingNone;
        case pendingNone:
            break;
        }
    }
    else
    {
        if(ret)
        {
            if(pending == pendingInput)
            {
                if(cs.cbInQue != 0)
                    return pendingInput;
            }
            else if(pending == pendingOutput)
            {
                if(cs.cbOutQue == 0)
                    return pendingOutput;
            }
        }

        memset(&ol, 0, sizeof(OVERLAPPED));
        ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if(ol.hEvent==NULL)
            return CPDL_ERROR;

        if(pending == pendingInput)
            dwMask = (EV_RXCHAR | EV_RXFLAG);
        else if(pending == pendingOutput)
            dwMask = EV_TXEMPTY;
        if(!SetCommMask(spirs_t->rs_dev, dwMask))
        {
            CloseHandle(ol.hEvent);
            return CPDL_ERROR;
        }

        // let's wait for event or timeout
        ret = WaitCommEvent(spirs_t->rs_dev, &dwMask, &ol);
        if(ret == FALSE)
        {
            if(GetLastError() == ERROR_IO_PENDING)
            {
                dwError = WaitForSingleObject(ol.hEvent, timeout);
                ret = (dwError == WAIT_OBJECT_0);
            }
            else
            {
                ClearCommError(spirs_t->rs_dev, &dwError, &cs);
            }
        }
        SetCommMask(spirs_t->rs_dev, 0);
        CloseHandle(ol.hEvent);
        if(ret == FALSE)
        {
            return CPDL_ERROR;
        }

        if(pending == pendingInput)
        {
            if(dwMask & (EV_RXCHAR | EV_RXFLAG))
                return pendingInput;
        }
        else if(pending == pendingOutput)
        {
            if(dwMask & (EV_TXEMPTY))
                return pendingOutput;
        }
    }
#endif
    return pendingNone;
}

/*! \brief 读串口
    \param spirs_t - 串口实例句柄指针
    \param buf  - 读取数据的存放缓冲区
    \param len   - 读取数据大小
    \return CPDL_ERROR - 出错, >=0 - 实际读取的数据字节数
    \sa ::cpspirs_open, ::cpspirs_status
*/
int cpspirs_read(cpspirs_t spirs_t, char *buf, const int len)
{
#ifndef _NWCP_WIN32
    return read(spirs_t->rs_dev, buf, len);
#else
    DWORD dwLength = 0;
    DWORD dwError = 0;
    DWORD  dwReadLength;
    COMSTAT	cs;
    OVERLAPPED ol;

    // Read max length or only what is available
    if(FALSE==ClearCommError(spirs_t->rs_dev, &dwError, &cs))
        return CPDL_ERROR;
    // If not requiring an exact byte count, get whatever is available
    if((DWORD)(len) > cs.cbInQue)
        dwReadLength = cs.cbInQue;
    else
        dwReadLength = len;

    if(dwReadLength > 0)
    {
        memset(&ol, 0, sizeof(OVERLAPPED));
        ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if(ol.hEvent==NULL)
            return CPDL_ERROR;
        if(FALSE==ReadFile(spirs_t->rs_dev, buf, dwReadLength, &dwLength, &ol))
        {
            if(GetLastError() == ERROR_IO_PENDING)
            {
                WaitForSingleObject(ol.hEvent, INFINITE);
                GetOverlappedResult(spirs_t->rs_dev, &ol, &dwLength, TRUE);
            }
            else
                ClearCommError(spirs_t->rs_dev, &dwError, &cs);
        }
        CloseHandle(ol.hEvent);
    }
    return dwLength;
#endif
}

/*! \brief 写串口
    \param spirs_t - 串口实例句柄指针
    \param buf  - 待写数据的存放缓冲区
    \param len   - 待写数据大小
    \return CPDL_ERROR - 出错, >=0 - 实际写的数据字节数
    \sa ::cpspirs_open, ::cpspirs_status
*/
int cpspirs_write(cpspirs_t spirs_t, const char *buf, const int len)
{
#ifndef _NWCP_WIN32
    return write(spirs_t->rs_dev, buf, len);
#else
    DWORD retSize = 0;
    DWORD dwError = 0;
    COMSTAT	cs;
    OVERLAPPED ol;
    memset(&ol, 0, sizeof(OVERLAPPED));
    // Clear the com port of any error condition prior to read
    if(FALSE==ClearCommError(spirs_t->rs_dev, &dwError, &cs))
        return CPDL_ERROR;
    ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(ol.hEvent==NULL)
        return CPDL_ERROR;
    if(FALSE==WriteFile(spirs_t->rs_dev, buf, len, &retSize, &ol))
    {
        if(GetLastError() == ERROR_IO_PENDING)
        {
            WaitForSingleObject(ol.hEvent, INFINITE);
            GetOverlappedResult(spirs_t->rs_dev, &ol, &retSize, TRUE);
        }
        else
            ClearCommError(spirs_t->rs_dev, &dwError, &cs);
    }
    CloseHandle(ol.hEvent);
    return retSize;
#endif
}

