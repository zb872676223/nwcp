
#ifndef _CPSTRING_H
#define _CPSTRING_H

#include "cpdltype.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _NWCP_WIN32
extern CPDLAPI char* strupr(char* str);
extern CPDLAPI char* strlwr(char* str);
#endif /* not _NWCP_WIN32 */

#ifndef _NWCP_SUNOS
extern CPDLAPI size_t strlcpy(char* dst, const char* src, size_t siz);
extern CPDLAPI size_t strlcat(char* dst, const char* src, size_t siz);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* not _CPSTRING_H */
