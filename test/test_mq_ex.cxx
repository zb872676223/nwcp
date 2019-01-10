#include "cpthread.h"
#include "cpshm.h"
#include <string.h>

extern bool g_running;
extern void test_recver_ex();
extern void test_sender_ex();
extern cpshm_t shm_t;

int main(int argc, char* argv[])
{
    while (1)
    {
        g_running = true;
        if (argc > 1)
        {
            test_recver_ex();
        }
        else
        {
            test_sender_ex();
            cpthread_sleep(2000);
        }
    }


    if (0 != shm_t)
        cpshm_close(&shm_t);

    return 0;
}
