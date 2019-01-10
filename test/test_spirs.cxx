#include "../cpspibus.h"
#include "../cpthread.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void send_msg(cpspirs_t rsc, char *bufw, int sendlen)
{
    char *psend = bufw;
    int ret = 0;
    while(sendlen > 0)
    {
        ret = cpspirs_state(rsc, pendingOutput, 1000);
        if(ret == pendingOutput)
        {
            ret = cpspirs_write(rsc, psend, sendlen);
            if(ret > 0)
            {
                psend += ret;
                sendlen -= ret;
                printf("\t send: len = %d left = %d\n",
                        ret, sendlen);
            }
            else
            {
                break;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        printf("Input serial port please : \n"
               "\tCOM? for windows, /dev/ttyS? for linux\n");
        return -1;
    }
    srand(time(0));
    int rcount = 0;
    int wcount = 0;
    int rwlen = 0;
    char bufrw[512];
    cpspirs_t rsc = 0;
    int ret = cpspirs_open(argv[1], &rsc);
    if(ret != CPDL_SUCCESS)
        return -1;

    if(cpspirs_set(rsc, RS_BR9600, RS_PARITY_NONE, RS_FLOW_NONE,
                   RS_DATABITS_8, RS_STOPBITS_1) != CPDL_SUCCESS)
    {
        cpspirs_close(&rsc);
        return -2;
    }

    while(wcount < 99900)
    {
        if(cpspirs_state(rsc, pendingInput, 500) == pendingInput)
        {
            rwlen = cpspirs_read(rsc, bufrw, 511);
            if(rwlen > 0)
            {
                rcount += rwlen;
                bufrw[rwlen] = 0;
                printf("R%5d read form %s: len = %d buf = %s\n",
                        rcount, argv[1], rwlen,  bufrw);
            }
        }
        else if(pendingOutput==cpspirs_state(rsc, pendingOutput, 2000))
        {
            char sc = abs(rand()) % (0x7E - 0x21) + 0x21;
            rwlen = (sc - 0x21 + 1) * 2;
            for(int i = 0; i < rwlen / 2; ++i)
            {
                bufrw[i] = 0x21 + i;
                bufrw[rwlen - 1 - i] = 0x7E - i;
            }
            bufrw[rwlen] = 0;
            wcount += rwlen;
            printf("S%5d send to %s: len = %d buf = %s\n",
                   wcount, argv[1], rwlen, bufrw);
            send_msg(rsc, bufrw, rwlen);
            cpthread_sleep(10);
        }
    }

    ret = cpspirs_close(&rsc);

#ifdef _NWCP_WIN32
    ::system("pause");
#endif
    return 0;
}

