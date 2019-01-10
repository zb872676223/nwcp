#ifndef _CPSPIBUS_H
#define _CPSPIBUS_H
#include "cpdltype.h"

/*@ 定义串行外设接口（Serial Peripheral Interface Bus，SPI）通信接口
*/

#ifdef __cplusplus
extern "C" {
#endif

extern int CPDLAPI cpspirs_open(const char* spirs, cpspirs_t* spirs_t);
extern int CPDLAPI cpspirs_close(cpspirs_t* spirs_t);
extern int CPDLAPI cpspirs_set(cpspirs_t spirs_t, const CPSPIRS_BR br,
                               const CPSPIRS_PARITY parity, const CPSPIRS_FLOW flow,
                               const CPSPIRS_DBITS databits, const CPSPIRS_SBITS stopbits);
extern int CPDLAPI cpspirs_setbaudrate(cpspirs_t spirs_t, const CPSPIRS_BR br);
extern int CPDLAPI cpspirs_setparity(cpspirs_t spirs_t, const CPSPIRS_PARITY parity);
extern int CPDLAPI cpspirs_setflow(cpspirs_t spirs_t, const CPSPIRS_FLOW flow);
extern int CPDLAPI cpspirs_setdatabits(cpspirs_t spirs_t, const CPSPIRS_DBITS bits);
extern int CPDLAPI cpspirs_setstopbits(cpspirs_t spirs_t, const CPSPIRS_SBITS stopbits);
extern int CPDLAPI cpspirs_state(cpspirs_t spirs_t, const CPIOPending pending, const int timeout);
extern int CPDLAPI cpspirs_read(cpspirs_t spirs_t, char* buf, const int len);
extern int CPDLAPI cpspirs_write(cpspirs_t spirs_t, const char* buf, const int len);

#ifdef __cplusplus
}
#endif


#endif
