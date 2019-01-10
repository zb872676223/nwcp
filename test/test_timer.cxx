#include <stdio.h>
#include "../cptimer.h"
#include "../cptime.h"
#include "../cpthread.h"

static void timer_proc00(int id, void *arg)
{
    printf("------timer_proc00 id<%d> arg< %04d >\n", id, ++(*((int*)arg)));
}

static void timer_proc99(int id, void *arg)
{
    printf("------timer_proc99 id< %d > arg< %s >\n", id, (char*)arg);
}

int test_timer(int iid = 1234, int inv = 200)
{
    printf("\ntest_timer begin\n");
    cptimer_t id;
    char str[128];
    int ret = cptimer_create(iid, &id);
    if(ret!=CPDL_SUCCESS)
    {
        printf("--test_timer cptimer_create(%d, &id) failed\n", iid);
        return 1;
    }
    printf("--test_timer cptimer_create(%d, &id) OK\n", iid);

    ret = cptimer_set(id, inv, timer_proc99, str);
    if(ret!=CPDL_SUCCESS)
    {
        cptimer_destroy(&id);
        printf("--test_timer cptimer_set(id, %d, %p, %p) failed\n", inv, timer_proc99, str);
        return 1;
    }
    printf("--test_timer cptimer_set(id, %d, %p, %p) OK\n", inv, timer_proc99, str);

    int i = 0;
    while(i < 100)
    {
        sprintf(str, "timer arg data i = %08d", ++i);
        cpthread_sleep(inv);
    }

    if(cptimer_destroy(&id)!=CPDL_SUCCESS)
    {
        printf("--test_timer cptimer_destroy(&id) failed \n");
    }

    printf("--test_timer cptimer_destroy(&id) OK \n");

    cpthread_sleep(inv);
    printf("test_timer end\n");
    return 0;
}

int test(int iid = 2345, int inv = 1000)
{
    cptimer_t id;
    int i = 0;
    int ret = cptimer_create(iid, &id);
    if(ret!=CPDL_SUCCESS)
    {
        printf("--test cptimer_create(%d, &cptimer_t) failed\n", iid);
        return 1;
    }
    printf("--test cptimer_create(%d, &cptimer_t) OK\n", iid);

    ret = cptimer_set(id, inv, timer_proc00, &i);
    if(ret!=CPDL_SUCCESS)
    {
        cptimer_destroy(&id);
        printf("--test cptimer_set(cptimer_t, %d, %p, %p) failed\n", inv, timer_proc00, &i);
        return 1;
    }
    printf("--test cptimer_set(cptimer_t, %d, %p, %p) OK\n", inv, timer_proc00, &i);

    test_timer();

    if(cptimer_destroy(&id)!=CPDL_SUCCESS)
    {
        printf("--main cptimer_destroy(&cptimer_t) failed \n");
    }
    printf("--test cptimer_destroy(&cptimer_t) OK \n");

    cpthread_sleep(inv);
    printf("test end\n");
    return 0;
}

int test_noprogreg_timer(int iid = 123456, int inv = 2000)
{
    static int pinc = 0;
    ++pinc;
    if(pinc > 2)
    {
        pinc = 0;
        return 1;
    }

    printf("\ntest_noprogreg_timer begin\n");

    cptimer_t id;
    cptimer_proc proc = pinc==1 ? timer_proc00 : 0;
    int i = 0;

    int ret = cptimer_create(iid, &id);
    if(ret!=CPDL_SUCCESS)
    {
        printf("--test_noprogreg_timer cptimer_create(%d, &cptimer_t) failed\n", iid);
        return 1;
    }
    printf("--test_noprogreg_timer cptimer_create(%d, &cptimer_t) OK\n", iid);
    ret = cptimer_set(id, inv, proc, &i);
    if(ret!=CPDL_SUCCESS)
    {
        cptimer_destroy(&id);
        printf("--test_noprogreg_timer cptimer_set(cptimer_t, %d, %p, %p) failed\n", inv, proc, &i);
        return 1;
    }
    printf("--test_noprogreg_timer cptimer_set(cptimer_t, %d, %p, %p) OK\n", inv, proc, &i);

    test_noprogreg_timer(23456, 400);
    int iit = 0;
    if(0==proc)
    {
        while(iit++ < 100)
        {
            if(cptimer_wait(id)!=CPDL_SUCCESS)
            {
                printf("--test_noprogreg_timer cptimer_wait(cptimer_t) failed \n");
            }
            printf("--test_noprogreg_timer cptimer_wait(cptimer_t) OK < %08d >\n", ++i);
        }
    }

    if(cptimer_destroy(&id)!=CPDL_SUCCESS)
    {
        printf("--test_noprogreg_timer cptimer_destroy(&cptimer_t) failed \n");
    }

    printf("--test_noprogreg_timer cptimer_destroy(&cptimer_t) OK \n");

    printf("test_noprogreg_timer end\n");
    return 0;
}

int main(int /*argc*/, char* /*argv*/[])
{
    printf("main begin\n");
    int i = 0;
    while(++i)
    {
        test_noprogreg_timer();
        test();
        printf("\tmain while %d\n", i);
        cpthread_sleep(1500);
    }
    printf("main end\n");
#ifdef _NWCP_WIN32
    ::system("pause");
#endif
    return 0;
}

