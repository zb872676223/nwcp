
#ifndef _CPSOCKET_H
#define _CPSOCKET_H

#include "cpdltype.h"

/*! \file  cpsocket.h
    \brief SOCKET接口
*/

/*! \addtogroup CPSOCKET SOCKET接口
    \remarks UNIX与Windows下的SOCKET接口基本相同, 主要区别是UNIX下关闭SOCKET的函数是
    close, 而Windows是closesocket, CPDL定义UNIX下的closesocket函数, 将两者统一*/
/*@{*/

#ifdef _NWCP_WIN32
typedef SOCKET cpsocket_t;
#   define CPSOCKET_T_INVALID INVALID_SOCKET
#else
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <netdb.h>

typedef int cpsocket_t;
#   define CPSOCKET_T_INVALID  -1
#   ifndef closesocket
#       include <unistd.h>
#       define closesocket close
#   endif
#endif

#if !defined(_SOCKLEN_T) && !defined(__SOCKLEN_T) && !defined(__socklen_t_defined) && !defined(HAVE_SOCKLEN_T)
typedef int cpsocklen_t;
#define HAVE_SOCKLEN_T
#else
typedef socklen_t cpsocklen_t;
#endif

/*@}*/
#endif  /* not _CPSOCKET_H */
