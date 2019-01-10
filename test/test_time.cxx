#include "../cptime.h"
#include "../cpthread.h"
#include <stdlib.h>
#include <stdio.h>

int main(int /*argc*/, char* /*argv*/[])
{
    printf("main begin\n\n");

    int i = 100;
    int sle = 0;
    int diff = 0;
    cp_tm b, e;
    char strtime[128];

    srand(cptime(0));
    while(i--)
    {
        sle = rand() % 5000;
        printf("\tsleep %d ms\n", sle);
        cpgettime(&b);
        cpthread_sleep(sle);
        cpgettime(&e);
        diff = cpdifftime_ms(&e, &b);
        printf("\tbegin %s\n", cptime2cstr(strtime, &b));
        printf("\tend   %s\n", cptime2cstr(strtime, &e));
        printf("\tdiff  %d ms\n", diff);
        printf("\tcpthread_sleep precision   %d ms\n\n", sle - diff);
    }

    printf("main end\n");
#ifdef _NWCP_WIN32
    ::system("pause");
#endif
    return 0;
}

