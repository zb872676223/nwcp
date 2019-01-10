// test_rwlock.cxx
//
#include "rwlock_test.cxx"
#include "../cprwlock.h"

cprwlock_t       g_srwLock;
static int write_lock( int timeout)
{
    return cprwlock_wlock(g_srwLock, timeout);
}
static int read_lock( int timeout)
{
    return cprwlock_rlock(g_srwLock, timeout);
}
static int write_release()
{
    return cprwlock_wunlock(g_srwLock);
}
static int read_release()
{
    return cprwlock_runlock(g_srwLock);
}

int main()
{
    int ret = 0, shareddata;
    int i;
    cpthread_t hThread[4];
    RWLT_DATA g_data = {write_lock, read_lock, write_release, read_release, &shareddata, (int*)0, 0, 0};
    RWLT_TDATA td[4] = { {&g_data, 0, 3}, {&g_data, 1, 4}, {&g_data, 2, 5},
                         {&g_data, 3, 6} };

    printf("  Test ... read-write-lock\n\n");

    cprwlock_create(&g_srwLock);
    init_test();

    shareddata = 10;
    for (i = 1 ; i <= 4; i++)
        ret = cpthread_create(&hThread[i-1], i % 2 ? read_thread : write_thread, &td[i-1], 1);
    for (i = 0; i < 4; i++)
    {
        cpthread_join(hThread[i], &ret);
    }

    uninit_test();
    cprwlock_close(&g_srwLock);

    getchar();
    return 0;
}

