#include "cpshm.h"
#include <stdio.h>
#include "cpstring.h"


const char* shm_id = "test_nwdl_shm";

struct TEST_SHM_DATA
{
    char str[128];
    int len;
};

void print_data(TEST_SHM_DATA* data)
{
    printf("TEST_SHM_DATA::str: %s \nTEST_SHM_DATA::len: %d\n\n",
           data->str, data->len);

}

int test_1()
{
    cpshm_t shm_t = 0;
    int ret = -1;

    ret = cpshm_exist(shm_id);
    if (CPDL_SUCCESS != ret)
    {
        ret = cpshm_create(shm_id, sizeof(TEST_SHM_DATA), &shm_t);
        if (CPDL_SUCCESS != ret)
        {
            return -1;
        }

        ret = cpshm_exist(shm_id);
        if (CPDL_SUCCESS != ret)
        {
            ret = cpshm_close(&shm_t);
            return -2;
        }
    }
    else
    {
        ret = cpshm_map(shm_id, &shm_t);
        if (CPDL_SUCCESS != ret)
        {
            return -1;
        }
    }

    TEST_SHM_DATA* data = 0;
    unsigned int len = 0;
    ret = cpshm_data(shm_t, (cpshm_d*)&data, &len);
    if (CPDL_SUCCESS != ret)
    {
        ret = cpshm_close(&shm_t);;
        return -4;
    }
    strlcpy(data->str, "test_1 shm by nonwill", 128);
    data->len = strlen(data->str);
    print_data(data);

    ret = cpshm_unmap(shm_t);
    data = 0;

    ret = cpshm_exist(shm_id);
    if (CPDL_SUCCESS != ret)
    {
        ret = cpshm_close(&shm_t);
        return -2;
    }

    cpshm_t shm_t2 = 0;
    ret = cpshm_map(shm_id, &shm_t2);
    ret = cpshm_data(shm_t2, (cpshm_d*)&data, &len);
    if (CPDL_SUCCESS != ret)
    {
        ret = cpshm_close(&shm_t2);
        ret = cpshm_close(&shm_t);
        return -5;
    }
    print_data(data);

    ret = cpshm_close(&shm_t2);
    ret = cpshm_close(&shm_t);

    return 1;
}


int test_2()
{
    cpshm_t shm_t = 0;
    int ret = -1;

    ret = cpshm_create(shm_id, sizeof(TEST_SHM_DATA), &shm_t);
    if (CPDL_SUCCESS != ret)
    {
        return -1;
    }

    TEST_SHM_DATA* data = 0;
    unsigned int len = 0;
    ret = cpshm_data(shm_t, (cpshm_d*)&data, &len);
    if (CPDL_SUCCESS != ret)
    {
        ret = cpshm_close(&shm_t);
        return -5;
    }

    strlcpy(data->str, "test_2 shm by nonwill", 128);
    data->len = strlen(data->str);
    print_data(data);

    ret = cpshm_close(&shm_t);

    shm_t = 0;
    data = 0;

    ret = cpshm_exist(shm_id);
    if (CPDL_SUCCESS == ret)
    {
        //ret = cpshm_close(&shm_t);
        return -2;
    }

    ret = cpshm_map(shm_id, &shm_t);
    if (CPDL_SUCCESS == ret)
    {
        ret = cpshm_close(&shm_t);
        return -3;
    }

    ret = cpshm_data(shm_t, (cpshm_d*)&data, &len);
    if (CPDL_SUCCESS == ret)
    {
        print_data(data);
        ret = cpshm_close(&shm_t);
        return -4;
    }

    return 1;
}


int test_3()
{
    cpshm_t shm_t = 0;
    int ret = -1;

    ret = cpshm_exist(shm_id);
    if (CPDL_SUCCESS != ret)
    {
        ret = cpshm_create(shm_id, sizeof(TEST_SHM_DATA), &shm_t);
        if (CPDL_SUCCESS != ret)
        {
            return -1;
        }

        ret = cpshm_exist(shm_id);
        if (CPDL_SUCCESS != ret)
        {
            ret = cpshm_close(&shm_t);
            return -2;
        }
    }

    TEST_SHM_DATA* data = 0;
    unsigned int len = 0;
    ret = cpshm_data(shm_t, (cpshm_d*)&data, &len);
    if (CPDL_SUCCESS != ret)
    {
        ret = cpshm_close(&shm_t);;
        return -4;
    }
    strlcpy(data->str, "test_3 shm by nonwill", 128);
    data->len = strlen(data->str);
    print_data(data);

    cpshm_t shm_t2 = 0;
    ret = cpshm_map(shm_id, &shm_t2);
    if (CPDL_SUCCESS != ret)
    {
        ret = cpshm_close(&shm_t);
        return -5;
    }
    ret = cpshm_close(&shm_t);

    ret = cpshm_data(shm_t2, (cpshm_d*)&data, &len);
    if (CPDL_SUCCESS != ret)
    {
        ret = cpshm_close(&shm_t2);
        return -5;
    }
    print_data(data);

    ret = cpshm_close(&shm_t2);

    return 1;
}

void test_4()
{
    cpshm_t shm_t = 0;
    int ret = -1;

    ret = cpshm_exist(shm_id);
    if (CPDL_SUCCESS != ret)
    {
        ret = cpshm_create(shm_id, sizeof(TEST_SHM_DATA), &shm_t);
        if (CPDL_SUCCESS != ret)
        {
            return ;
        }

        ret = cpshm_exist(shm_id);
        if (CPDL_SUCCESS != ret)
        {
            ret = cpshm_close(&shm_t);
            return ;
        }
    }

    cpshm_t shm_t2 = 0;
    cpshm_t shm_t3 = 0;
    ret = cpshm_map(shm_id, &shm_t2);
    if (CPDL_SUCCESS != ret)
    {
        ret = cpshm_close(&shm_t);
        return ;
    }

    ret = cpshm_close(&shm_t2);

    ret = cpshm_map(shm_id, &shm_t3);
    if (CPDL_SUCCESS != ret)
    {
        ret = cpshm_close(&shm_t);
        return ;
    }
    else
    {
        ret = cpshm_close(&shm_t3);
    }

    ret = cpshm_exist(shm_id);
    if (CPDL_SUCCESS != ret)
    {
        printf("test_4 cpshm_exist : no shm exist\n");
    }

    ret = cpshm_close(&shm_t);
}

int main(int /*argc*/, char* /*argv*/[])
{
    test_1();
    test_2();
    test_3();
    test_4();
#ifdef _NWCP_WIN32
    system("pause");
#endif
    return 1;
}
