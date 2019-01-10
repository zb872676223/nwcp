#include "cpthread.h"
#include <string.h>

extern bool g_running;
extern void test_recver();
extern void test_sender();

int main(int argc, char* argv[])
{
    while (1)
    {
        g_running = true;
        if (argc > 1)
        {
            test_recver();
        }
        else
        {
            test_sender();
            cpthread_sleep(2000);
        }
    }
    return 0;
}
