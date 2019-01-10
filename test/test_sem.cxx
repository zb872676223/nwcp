// test_nwdl.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include "../cpprocsem.h"
#include "../cpthread.h"

const char* sem_name_mu = "test_nwdl_sem_mu";

const char* sem_name_ip = "test_nwdl_sem_ip";
const char* sem_name_op = "test_nwdl_sem_op";
#ifdef _NWCP_WIN32
void sem_test_thread(void* para)
#else
void* sem_test_thread(void* para)
#endif
{
    int& ic = *((int*)para);
    int tc = 0;

    cpmutex_t mutex = 0;
    int ret = cpmutex_open(sem_name_mu, &mutex);
    if (ret == CPDL_ERROR)
    {
        printf("sem_test_thread cpthread_open failed!\n");
    }

    while (ic > 1)
    {
        cpsem_t semt;
        ret = cpsem_open(sem_name_ip, &semt);
        if (ret == CPDL_SUCCESS)
        {
            ret = cpsem_post(semt);
            if (ret == CPDL_SUCCESS)
            {
                ret = cpmutex_lock(mutex);
                printf("We post a sem signal! - s %02d\n", ++tc);
                if (ret != CPDL_SUCCESS)
                {
                    printf("sem_test_thread cpmutex_lock failed!\n");
                }
                else
                {
                    ret = cpmutex_unlock(mutex);
                    if (ret != CPDL_SUCCESS)
                    {
                        printf("sem_test_thread cpmutex_unlock failed!\n");
                    }
                }

            }
            cpsem_close(&semt);
        }

        cpthread_sleep(100);
    }

    ret = cpmutex_close(&mutex);
    if (ret == CPDL_ERROR)
    {
        printf("sem_test_thread cpmutex_close failed!\n");

    }

    cpthread_exit(123);

#ifndef _NWCP_WIN32
    return (void*)123;
#endif
}

int test_inproc()
{
    printf("test_inproc\n");
    cpsem_t semt = 0;
    int ret = cpsem_create(sem_name_ip, 0, 1, &semt);
    if (ret == CPDL_ERROR)
    {
        printf("test_inproc cpsem_create failed!\n");
        cpsem_close(&semt);
    }

    cpmutex_t mutex = 0;
    ret = cpmutex_create(sem_name_mu, &mutex);
    if (ret == CPDL_ERROR)
    {
        printf("test_inproc cpmutex_create failed!\n");
        cpsem_close(&semt);
    }

    int ic = 10;
    cpthread_t tid;
    ret = cpthread_create(&tid, sem_test_thread, &ic, 1);
    if (ret == CPDL_ERROR)
    {
        printf("test_inproc cpthread_create failed!\n");
        cpmutex_close(&mutex);
        cpsem_close(&semt);
    }


    int tc = 0;
    while (ic--)
    {
        ret = cpsem_wait(semt, 1000);
        if (ret == CPDL_SUCCESS)
        {
            ret = cpmutex_lock(mutex);
            printf("We wait a sem signal! - w %02d\n", ++tc);
            if (ret != CPDL_SUCCESS)
            {
                printf("test_inproc cpmutex_lock failed!\n");
            }
            else
            {
                ret = cpmutex_unlock(mutex);
                if (ret != CPDL_SUCCESS)
                {
                    printf("test_inproc cpmutex_unlock failed!\n");
                }
            }
        }
        else
        {
            printf("cpsem_wait timeout!\n");
        }
    }


    cpthread_join(tid, &ret);

    ret = cpmutex_close(&mutex);
    if (ret == CPDL_ERROR)
    {
        printf("test_inproc cpmutex_close failed!\n");
        cpsem_close(&semt);
    }

    cpsem_close(&semt);
    if (ret == CPDL_ERROR)
    {
        printf("test_inproc cpsem_close failed!\n");
        cpsem_close(&semt);
    }

    return 0;
}

int test_oproc()
{
    printf("test_oproc\n");
    cpsem_t semt;
    int ret = cpsem_open(sem_name_op, &semt);
    if (ret != CPDL_SUCCESS)
    {
        ret = cpsem_create(sem_name_op, 0, 1, &semt);
        if (ret == CPDL_ERROR)
            return -1;
    }


    int ic = 20;

    while (ic--)
    {
        ret = ic % 2 ? cpsem_wait(semt, 1000) : cpsem_post(semt);
        if (ret == CPDL_SUCCESS)
        {
            printf(ic % 2 ? "We got a sem signal!\n" : "We sent a sem signal!\n");
        }
    }

    cpsem_close(&semt);

    return 0;
}

int test_trywait()
{
    printf("test_trywait\n");
    cpsem_t semt;
    int ret = cpsem_open(sem_name_op, &semt);
    if (ret != CPDL_SUCCESS)
    {
        ret = cpsem_create(sem_name_op, 0, 1, &semt);
        if (ret == CPDL_ERROR)
            return -1;
    }

    int ic = 30;

    while (ic--)
    {
        ret = ic % 3 ? cpsem_trywait(semt) : cpsem_post(semt);
        if (ret == CPDL_SUCCESS)
        {
            printf(ic % 3 ? "We got a sem signal! OK\n" : "We sent a sem signal! OK\n");
        }
        else
        {
            printf(ic % 3 ? "We got no signal!\n" : "We sent a sem failed!\n");
            //cpthread_sleep(1000);
        }
    }

    cpsem_close(&semt);

    return 0;
}

int main(int /*argc*/, char* /*argv*/[])
{
    while (1)
    {
        test_trywait();
        test_inproc();
        test_oproc();
        cpthread_sleep(1500);
    }
    return 0;
}
