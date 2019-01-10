// test_prwlock.cxx
//
#include "rwlock_test.cxx"
#include "../cpshm.h"
#include "../cpprocsem.h"
#include "../cprwlock.h"
#include "../cptime.h"

cpprwlock_t       g_srwLock;
static int write_lock( int timeout)
{    return cpprwlock_wlock(g_srwLock, timeout);}
static int read_lock( int timeout)
{    return cpprwlock_rlock(g_srwLock, timeout);}
static int write_release()
{    return cpprwlock_wunlock(g_srwLock);}
static int read_release()
{    return cpprwlock_runlock(g_srwLock);}

const char *shmname = "test_prwlock_shm";
const char *semname = "test_prwlock_sem";
const char *rwlname = "test_prwlock_rwl";

int main(int argc, char *argv[])
{
    int ret = 0, i = 0;
    cpthread_t hThread[3];
    cpsem_t sem = 0;
    cpshm_t shm = 0;
    bool writer = false;
    RWLT_DATA g_data = { write_lock, read_lock, write_release, read_release, (int*)0, (int*)0, 0, 0 };

    if(argc > 1)
        g_uselog = false;

    if(CPDL_SUCCESS != cpshm_map(shmname, &shm))
    {
        if(CPDL_SUCCESS != cpshm_create(shmname, sizeof(int) * 2, &shm))
        {
            printf("Create shared memory failed!\n");
            return -1;
        }
        writer = true;
    }
    else
    {
        cpthread_sleep(500);
    }

    printf("  Test ... read-write-lock %s\n\n",writer ? "server" : "slave");

    if(CPDL_SUCCESS != cpshm_data(shm, (cpshm_d*)&g_data.data, 0))
    {
        printf("Map shm-data failed!\n");
        cpshm_close(&shm);
        return -1;
    }
    g_data.pcount = g_data.data + sizeof(int);

    if(CPDL_SUCCESS != cpsem_open(semname, &sem))
    {
        if(CPDL_SUCCESS != cpsem_create(semname, 0, 1, &sem))
        {
            printf("Create shared semaphore failed!\n");
            cpshm_close(&shm);
            return -2;
        }
    }

    if(CPDL_SUCCESS != cpprwlock_open(rwlname,&g_srwLock))
    {
        if(CPDL_SUCCESS != cpprwlock_create(rwlname, &g_srwLock))
        {
            printf("Create shared rwlock failed!\n");
            cpsem_close(&sem);
            cpshm_close(&shm);
            return -3;
        }
    }

    init_test();

    if(writer)
    {
        *g_data.pcount = 0;
        *g_data.data = 1000;
        cpsem_wait(sem, -1);
    }
    else
    {
         if(0 == *g_data.pcount)
            cpsem_post(sem);
         *g_data.pcount += 1;
    }

    RWLT_TDATA td[4] = { {&g_data, 0, 1}, {&g_data, 1, 0}, {&g_data, 2, 0} , {&g_data, 3, 0} };
    cp_tm tm_b, tm_e;
    cpgettime(&tm_b);
    for (i = 0 ; i < 4; i++)
    {
        ret = cpthread_create(&hThread[i], !(i % 4) ? write_thread : read_thread, &td[i], 1);
    }
    for (i = 0; i < 4; i++)
    {
        cpthread_join(hThread[i], &ret);
    }
    cpgettime(&tm_e);
    if(writer)
    {
        while(*g_data.pcount)
            cpsem_wait(sem, -1);
    }
    else
    {
        *g_data.pcount -= 1;
        cpsem_post(sem);
    }

    uninit_test();
    cpprwlock_close(&g_srwLock);
    cpsem_close(&sem);
    cpshm_close(&shm);
    printf("Time: %f reads: %d writes: %d \n", cpdifftime(&tm_e, &tm_b),
           g_data.reads, g_data.writes);
    getchar();
    return 0;
}

