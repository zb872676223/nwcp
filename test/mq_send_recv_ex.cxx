#include "mq_send_recv.cxx"

#include "cpshm.h"

cpshm_t shm_t = 0;

int get_shm(cpshm_d& data, unsigned int memSize)
{
    //char strName[64];
    unsigned int mapLen = 0;
    //snprintf(strName,  63, "%s%s", g_mq_name, "nwcp");
    if (0 == shm_t && CPDL_SUCCESS != cpshm_map(g_mq_name, &shm_t))
    {
        if (CPDL_SUCCESS != cpshm_create(g_mq_name, memSize, &shm_t))
            return CPDL_ERROR;
        if (CPDL_SUCCESS != cpshm_data(shm_t, &data, 0))
        {
            cpshm_close(&shm_t);
            return CPDL_ERROR;
        }

        return 1;
    }
    else if (CPDL_SUCCESS != cpshm_data(shm_t, &data, &mapLen) || mapLen != memSize)
    {
        cpshm_close(&shm_t);
        return CPDL_ERROR;
    }
    return 0;
}

void test_sender_ex()
{
    cpthread_t hSender = 0;
    int tSender = 0;
    cpshmmq_t msgQTest = 0;
    int maxMsgs = mg_max;
    cpshm_d data;
    size_t memSize = cpshmmq_shm_size(maxMsgs, 100);
    int cc = get_shm(data, memSize);
    if (cc < 0)

        return;
    if (cc == 1 || CPDL_SUCCESS != cpshmmq_open_ex(g_mq_name, data, &msgQTest, 0, 0))
    {
        if (CPDL_SUCCESS != cpshmmq_create_ex(g_mq_name, maxMsgs, 100, data, &msgQTest))
        {
            printf("%s: unable to open message queue.\n", "test_sender");
            return;
        }
        printf("%s: message queue opened.\n", "test_sender");
    }
    else
    {
        printf("%s: message queue created.\n", "test_recver");
    }

    if (CPDL_SUCCESS == cpthread_create(&hSender, (CPTHREAD_ROUTINE)msgQSender, msgQTest, 1))
    {
        cpthread_join(hSender, &tSender);
        msgQShow(msgQTest);
        printf("%s: sender's thread exit %d.\n", "test_sender", tSender);
    }
    else
    {
        printf("%s: create sender's thread failed.\n", "test_sender");
    }

    cpshmmq_close(&msgQTest);
}

void test_recver_ex()
{
    cpthread_t hReceiver = 0;
    int tReceiver = 0;
    cpshmmq_t msgQTest = 0;
    int maxMsgs = mg_max;

    cpshm_d data;
    size_t memSize = cpshmmq_shm_size(maxMsgs, 100);
    int cc = get_shm(data, memSize);
    if (cc < 0)
        return;

    if (cc == 1 || CPDL_SUCCESS != cpshmmq_open_ex(g_mq_name, data, &msgQTest, 0, 0))
    {
        if (CPDL_SUCCESS != cpshmmq_create_ex(g_mq_name, maxMsgs, 100, data, &msgQTest))
        {
            printf("%s: unable to open message queue.\n", "test_recver");
            return;
        }
        printf("%s: message queue opened.\n", "test_recver");
    }
    else
    {
        printf("%s: message queue created.\n", "test_recver");
    }

    if (CPDL_SUCCESS == cpthread_create(&hReceiver, (CPTHREAD_ROUTINE)msgQReceiver, msgQTest, 1))
    {
        cpthread_join(hReceiver, &tReceiver);
        msgQShow(msgQTest);
        printf("%s: recviver's thread exit %d.\n", "test_recver", tReceiver);
    }
    else
    {
        printf("%s: create recviver's thread failed.\n", "test_recver");
    }


    cpshmmq_close(&msgQTest);
}
