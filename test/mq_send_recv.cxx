#include "cpshmmq.h"
#include "cpthread.h"
#include "cptime.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

const char* g_mq_name = "test_nwcp_mq";
bool g_running = true;
const int mg_max = 100;
unsigned long allsend = 0;
unsigned long allread = 0;

/*
 * show the status of message queue
 */
int msgQShow(cpshmmq_t msgQId)
{
    char file[CPSHMMQ_NAME_LEN] = {0,};
    unsigned int msgNum, recvTimes, sendTimes, maxMsgs, maxMsgLength;
    int rc = cpshmmq_status(msgQId, &msgNum, &sendTimes, &recvTimes, &maxMsgs, &maxMsgLength, file);
    if (rc != CPDL_SUCCESS)
    {
        printf("Get the message queue state failed in %s.\n", "msgQSender");
        return -1;
    }

    printf("cpshmmq_t.file         = %s\n",  file);
    printf("cpshmmq_t.maxMsg       = %d\n",  maxMsgs);
    printf("cpshmmq_t.maxMsgLength = %d\n",  maxMsgLength);
    printf("cpshmmq_t.msgNum       = %d\n",  msgNum);
    printf("cpshmmq_t.recvTimes    = %d\n",  recvTimes);
    printf("cpshmmq_t.sendTimes    = %d\n",  sendTimes);

    return CPDL_SUCCESS;
}

void sender_task(void* param, int timeout)
{
    int i = 0;
    int priority = 0;
    int rc = 0;
    cpshmmq_t msgQTest = (cpshmmq_t)param;
    char buf[256] = {0};
    unsigned int buflen = 0;

    if (msgQShow(msgQTest) != CPDL_SUCCESS)
    {
        printf("Get the message queue state failed in %s.\n", "msgQSender");
        cpthread_exit(-1);
        return;
    }

    srand((unsigned)cptime(NULL));
    int mymax = mg_max + rand() % (2000 - mg_max) ;
    rc = 0;
    unsigned int recvc, sendc;
    while (g_running)
    {
        if (rc > 20 || i >  mymax)
        {
            msgQShow(msgQTest);
            break;
        }
        sprintf(buf, "data transfered index - %4d", i);
        if (0 == (rand() % 2))
            priority = CPSHMMQ_PRI_NORMAL;
        else
            priority = CPSHMMQ_PRI_URGENT;

        buflen = strlen(buf) + 1;
        if (CPDL_SUCCESS != cpshmmq_push(msgQTest, buf, &buflen, timeout, priority))
        {
            ++rc;
            printf("send message failed in %s.\n", "msgQSender");
            msgQShow(msgQTest);
            cpthread_sleep(20);
            continue;
        }
        rc = 0;

        if (CPDL_SUCCESS == cpshmmq_status(msgQTest, 0, &sendc, &recvc, 0, 0, 0))
        {
            printf("S2R {%s} <r%4d s%4d> ----- %ld \n", buf, recvc, sendc, ++allsend);
        }
        else
        {
            printf("S2R {%s} ----- %ld \n", buf, ++allsend);
        }
        i++;
        cpthread_sleep(1);
    }
}

void msgQSender(void* param)
{
    sender_task(param, 1000);
    cpthread_exit(1);
}

void receiver_task(void* param, int timeout)
{
    char buf[64] = {0};
    unsigned int buflen = 64;
    int i = 0;
    int rc = 0;

    cpshmmq_t msgQTest = (cpshmmq_t)param;
    if (msgQShow(msgQTest) != CPDL_SUCCESS)
    {
        printf("Get the message queue state failed in %s.\n", "msgQReceiver");
        cpthread_exit(-2);
        return;
    }

    srand((unsigned)cptime(NULL));
    int mymax = mg_max + rand();
    rc = 0;
    unsigned int recvc, sendc;
    while (g_running)
    {
        if (rc > 10 || i > mymax)
        {
            msgQShow(msgQTest);
            break;
        }
        memset(buf, 0, sizeof(buf));
        buflen = 64;
        if (CPDL_SUCCESS != cpshmmq_pop(msgQTest, buf, &buflen, timeout))
        {
            ++rc;
            printf("receive message failed in %s.\n", "msgQReceiver");
            msgQShow(msgQTest);
            cpthread_sleep(20);
            continue;
        }
        rc = 0;
        if (CPDL_SUCCESS == cpshmmq_status(msgQTest, 0, &sendc, &recvc, 0, 0, 0))
        {
            printf("RfS i%4d {%s} <r%4d s%4d> ----- %ld \n", i, buf, recvc, sendc, ++allread);
        }
        else
        {
            printf("RfS i%4d {%s} ----- %ld \n", i, buf, ++allread);
        }
        i++;
        //cpthread_sleep(1);
    }
}

void msgQReceiver(void* param)
{
    receiver_task(param, 1000);
    cpthread_exit(2);
}

void test_sender()
{
    cpthread_t hSender = 0;
    int tSender = 0;
    cpshmmq_t msgQTest = 0;
    int maxMsgs = mg_max;

    if (CPDL_SUCCESS != cpshmmq_create(g_mq_name, maxMsgs, 100, &msgQTest))
    {
        if (CPDL_SUCCESS != cpshmmq_open(g_mq_name, &msgQTest, 0, 0))
        {
            printf("%s: unable to create message queue.\n", "test_sender");
            return;
        }
        printf("%s: message queue created.\n", "test_sender");
    }
    else
    {
        printf("%s: message queue opened.\n", "test_sender");
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

void test_recver()
{
    cpthread_t hReceiver = 0;
    int tReceiver = 0;
    cpshmmq_t msgQTest = 0;
    int maxMsgs = mg_max;

    if (CPDL_SUCCESS != cpshmmq_open(g_mq_name, &msgQTest, 0, 0))
    {
        if (CPDL_SUCCESS != cpshmmq_create(g_mq_name, maxMsgs, 100, &msgQTest))
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
