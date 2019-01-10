
#ifndef _CPFLOAT_H
#define _CPFLOAT_H

#include "cpdltype.h"

#ifdef _NWCP_WIN32
#include <float.h>
#define cpfinite(x) _finite(x)
#define cpisnan(x) _isnan(x)
#else
#include <math.h>
#define cpfinite(x) finite(x)
#define cpisnan(x) isnan(x)
#endif /* not _NWCP_WIN32 */

#endif /* not _CPFLOAT_H */
