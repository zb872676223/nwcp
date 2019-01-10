// test_thread.cxx : 定义控制台应用程序的入口点。
//
#include "../cpthread.h"
#include <stdio.h>

struct mc_signal
{
    cpthread_mutex_t mutex;
    cpthread_cond_t  cond;
    int  runflag;
    char testmsg[128];
};

#ifdef _NWCP_WIN32
void test_thread(void *para)
#else
void * test_thread(void *para)
#endif
{
    mc_signal &mcs = *((mc_signal*)para);
    //生产者
    int tc = 0;
    while(--mcs.runflag)
    {
        int ret = cpthread_mutex_lock(&mcs.mutex);
        if(ret != CPDL_SUCCESS)
        {
            printf("test cpthread_mutex_lock failed!\n");
            cpthread_sleep(1000);
            continue;
        }

        ret = cpthread_cond_signal(&mcs.cond);
        if(ret != CPDL_SUCCESS)
        {
            printf("test cpthread_cond_signal failed!\n");
        }

        snprintf(mcs.testmsg, 127, "Wei post a signal. index: %04d", tc++);
        printf("test Send msg { %s }\n", mcs.testmsg);

        ret = cpthread_mutex_unlock(&mcs.mutex);
        if(ret != CPDL_SUCCESS)
        {
            printf("test cpthread_mutex_unlock failed!\n");
        }
        cpthread_sleep(10);
    }

    cpthread_exit(123456);

#ifndef _NWCP_WIN32
    return (void*)123;
#else

#endif
}


int main(int /*argc*/, char* /*argv*/[])
{
    mc_signal mcs;
    mcs.runflag = 100;
    int ret = cpthread_mutex_init(&mcs.mutex);
    if(ret!=CPDL_SUCCESS)
    {
        printf("main cpthread_mutex_init failed!\n");
    }

    ret = cpthread_cond_init(&mcs.cond);
    if(ret!=CPDL_SUCCESS)
    {
        printf("main cpthread_cond_init failed!\n");
    }

    cpthread_t tid;
    ret = cpthread_create(&tid, test_thread, &mcs, 1);
    if(ret!=CPDL_SUCCESS)
    {
        printf("main cpthread_create failed!\n");
        cpthread_cond_destroy(&mcs.cond);
        cpthread_mutex_destroy(&mcs.mutex);
    }

    while(mcs.runflag)
    {
        ret = cpthread_mutex_lock(&mcs.mutex);
        if(ret != CPDL_SUCCESS)
        {
            printf("main cpthread_mutex_lock failed!\n");
            cpthread_sleep(10);
            continue;
        }

        ret = cpthread_cond_timedwait(&mcs.cond, &mcs.mutex, 1000);
        if(ret == CPDL_SUCCESS)
        {
            printf("main Recv msg { %s }\n", mcs.testmsg);
        }
        else
        {
            printf("main cpthread_cond_timedwait overtime!\n");
        }

        ret = cpthread_mutex_unlock(&mcs.mutex);
        if(ret != CPDL_SUCCESS)
        {
            printf("main cpthread_mutex_unlock failed!\n");
        }
    }


    cpthread_join(tid, &ret);

    ret = cpthread_cond_destroy(&mcs.cond);
    if(ret!=CPDL_SUCCESS)
    {
        printf("main cpthread_cond_destroy failed!\n");
    }

    ret = cpthread_mutex_destroy(&mcs.mutex);
    if(ret!=CPDL_SUCCESS)
    {
        printf("main cpthread_mutex_destroy failed!\n");
    }

#ifdef _NWCP_WIN32
    ::system("pause");
#endif
    return 0;
}
