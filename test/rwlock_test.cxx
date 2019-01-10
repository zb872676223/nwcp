// rwlock_test.cxx
//
#include "../cpthread.h"
#include <stdio.h>
#include <stdarg.h>
typedef struct tagRWLOCK_TEST_DATA
{
    int (*write_lock)( int);
    int (*read_lock)( int);
    int (*write_release)(void);
    int (*read_release)(void);
    int *data;
    int *pcount;
    int reads;
    int writes;
}RWLT_DATA;
typedef struct tagRWLOCK_TEST_TDATA
{
    RWLT_DATA *pdata;
    int id;
    int slps;
}RWLT_TDATA;

static bool g_uselog = true;
static cpthread_mutex_t g_cs;
void init_test()
{
    cpthread_mutex_init(&g_cs);
}
void uninit_test()
{
    cpthread_mutex_destroy(&g_cs);
}

#ifdef _NWCP_WIN32
BOOL SetConsoleColor(const WORD wAttributes)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE)
        return FALSE;
    return SetConsoleTextAttribute(hConsole, wAttributes);
}

void WriterPrintf(const char *pszFormat, ...)
{
    if(!g_uselog)
        return;
    va_list   pArgList;
    va_start(pArgList, pszFormat);
    SetConsoleColor(FOREGROUND_GREEN);
    vfprintf(stdout, pszFormat, pArgList);
    SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    va_end(pArgList);
}

void ReaderPrintf(const char *pszFormat, ...)
{
    if(!g_uselog)
        return;
    va_list   pArgList;
    va_start(pArgList, pszFormat);
//    cpthread_mutex_lock(&g_cs);
    vfprintf(stdout, pszFormat, pArgList);
//    cpthread_mutex_unlock(&g_cs);
    va_end(pArgList);
}
#else
void WriterPrintf(const char *pszFormat, ...)
{
    if(!g_uselog)
        return;
    va_list   pArgList;
    va_start(pArgList, pszFormat);
    //SetConsoleColor(FOREGROUND_GREEN);
    vfprintf(stdout, pszFormat, pArgList);
    //SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    va_end(pArgList);
}

void ReaderPrintf(const char *pszFormat, ...)
{
    if(!g_uselog)
        return;
    va_list   pArgList;
    va_start(pArgList, pszFormat);
//    cpthread_mutex_lock(&g_cs);
    vfprintf(stdout, pszFormat, pArgList);
//    cpthread_mutex_unlock(&g_cs);
    va_end(pArgList);
}
#endif

#ifdef _NWCP_WIN32
void read_thread(void *pM)
#else
void* read_thread(void *pM)
#endif
{
    RWLT_TDATA *data = (RWLT_TDATA*)pM;
    int running = 1;
    while(running)
    {
        if(CPDL_SUCCESS != data->pdata->read_lock(1000))
        {
            ReaderPrintf("%d. read failed!\n", data->id);
            continue;
        }
        running = *(data->pdata->data);
        ++data->pdata->reads;
        ReaderPrintf("%d. read OK running = %d.\n", data->id, running);
        data->pdata->read_release();
        cpthread_sleep(data->slps); /* release the cpu */
    }
    cpthread_exit(123);
#ifndef _NWCP_WIN32
    return ((void*)0);
#endif
}

#ifdef _NWCP_WIN32
static void write_thread(void *pM)
#else
static void* write_thread(void *pM)
#endif
{
    RWLT_TDATA *data = (RWLT_TDATA*)pM;
    int running = 1;
    while(running)
    {
        if(CPDL_SUCCESS != data->pdata->write_lock(1000))
        {
            WriterPrintf("%d. write failed!\n", data->id);
            continue;
        }
        ++data->pdata->writes;
        if(*(data->pdata->data))
        {
            *(data->pdata->data) -= 1;
            running = *(data->pdata->data);
            WriterPrintf("%d. write OK --running = %d.\n", data->id, running);
        }
        else
        {
            running = 0;
            WriterPrintf("%d. write OK   running = %d.\n", data->id, running);
        }
        data->pdata->write_release();
        cpthread_sleep(data->slps); /* release the cpu */
        //cpthread_sleep(0); /* release the cpu */
    }
    cpthread_exit(321);
#ifndef _NWCP_WIN32
    return 0;
#endif
}

