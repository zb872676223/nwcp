#include "../cpshm.h"
#include "cpstring.h"
#include <stdlib.h>

/*! \file  cpshm.c
    \brief 共享内存管理
*/

/*! \addtogroup CPSHM 共享内存管理
    \remarks  */
/*@{*/

#ifdef _NWCP_WIN32
typedef HANDLE  cpshm_handle;
#else
#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
typedef int    cpshm_handle;
extern void __get_ipc_name(const char *name, char *full_name, const char *op);
static const char *cpshm_ipc_op = "cpdl_shm_";
extern key_t __tok_ipc_file(const char *ipcname, int id);
#endif


#define SHM_T_FILENAME_LEN 256
struct _cpshm_privatedata
{
    unsigned int len;
};

struct _cpshm_t
{
    cpshm_handle handler;
    struct _cpshm_privatedata *raw_addr;
};

/*! \brief 创建共享内存并映射共享内存到用户空间
    \param shm_id - 共享内存关键字
    \param len - 共享内存长度
    \param shm_t - 共享内存句柄指针(当确定不再使用时需要调用::cpshm_close销毁)
    \return CPDL_SUCCESS - 成功    CPDL_ERROR - 失败
    \sa ::cpmem_exist, ::cpshm_map, ::cpshm_mapex, ::cpshm_close
*/
int cpshm_create(const char *shm_id, const unsigned int len,  cpshm_t *shm_t)
{
    void *addr = 0;
#ifdef _NWCP_WIN32
    cpshm_handle shm = CreateFileMapping(INVALID_HANDLE_VALUE,
                                         NULL, PAGE_READWRITE, 0,
                                         len + sizeof(struct _cpshm_privatedata),
                                         shm_id);
    if(0==shm )
        return CPDL_ERROR;

    addr = (void*)MapViewOfFile(shm, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#else
#define PERM S_IRUSR | S_IWUSR | IPC_CREAT
    cpshm_handle shm = -1;
    char ipcname[256];
    key_t key;
    __get_ipc_name(shm_id, ipcname, cpshm_ipc_op);
    key = __tok_ipc_file(ipcname, 0);
    if (key < 0 )
        return CPDL_ERROR;
    shm = shmget(key, len + sizeof(struct _cpshm_privatedata), PERM | 0666 );
    if( shm < 0)
        return CPDL_ERROR;
    addr = (void *)shmat(shm, (char*)0, 0666);
#endif
    *shm_t = (struct _cpshm_t*)malloc(sizeof(struct _cpshm_t));
    (*shm_t)->raw_addr = (struct _cpshm_privatedata*)(addr);
    (*shm_t)->raw_addr->len = len;
    (*shm_t)->handler = shm;
  return CPDL_SUCCESS;
}


/*! \brief 检测共享内存是否存在
    \param shm_id - 共享内存关键字
    \return CPDL_SUCCESS - 存在, CPDL_ERROR - 不存在
    \sa ::cpshm_create
*/
int cpshm_exist(const char *shm_id)
{
#ifdef _NWCP_WIN32
    cpshm_handle shm = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, shm_id);
    if(0==shm)
        return CPDL_ERROR;
    CloseHandle(shm);
#else
    char ipcname[256];
    key_t key = -1;
    __get_ipc_name(shm_id, ipcname, cpshm_ipc_op);
     key = ftok(ipcname, 0);
    if(key < 0)
        return CPDL_ERROR;
    if ( shmget(key, 0, 0) < 0 )
        return CPDL_ERROR;
#endif
    return CPDL_SUCCESS;
}


/*! \brief 映射已创建好的共享内存到用户空间
    \param shm_id - 共享内存关键字
    \param shm_t - 共享内存句柄(当确定不再使用时需要调用::cpshm_unmap进行反映射)
    \return CPDL_SUCCESS - 成功, CPDL_EINVAL - 参数错误, CPDL_ERROR - 失败
    \sa ::cpshm_create, ::cpshm_unmap
*/
int cpshm_map(const char *shm_id, cpshm_t *shm_t)
{
#ifdef _NWCP_WIN32
    cpshm_handle shm = INVALID_HANDLE_VALUE;
#else
    cpshm_handle shm = -1;
    key_t key = -1;
    char ipcname[256];
#endif
    if(0==shm_t || 0==shm_id || 0==strlen(shm_id) )
        return CPDL_EINVAL;
#ifdef _NWCP_WIN32
    shm = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, shm_id);
    if(0==shm)
        return CPDL_ERROR;
    *shm_t = (struct _cpshm_t*)malloc(sizeof(struct _cpshm_t));
    (*shm_t)->raw_addr = (struct _cpshm_privatedata *)MapViewOfFile(shm, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#else
    __get_ipc_name(shm_id, ipcname, cpshm_ipc_op);
    key = ftok(ipcname, 0);
    if(key < 0)
        return CPDL_ERROR;
    shm = shmget(key, 0, 0);
    if (shm < 0)
        return CPDL_ERROR;
    *shm_t = (struct _cpshm_t*)malloc(sizeof(struct _cpshm_t));
    (*shm_t)->raw_addr = (struct _cpshm_privatedata *)shmat(shm, (char*)0, 0666);
#endif
    (*shm_t)->handler = shm;
    return CPDL_SUCCESS;
}

/*! \brief 释放共享内存映射地址
    \param shm_t - 共享内存句柄
    \return CPDL_SUCCESS - 成功, CPDL_EINVAL - 参数错误, CPDL_ERROR - 失败
    \sa ::cpshm_create, ::cpshm_map, ::cpshm_mapex
*/
int cpshm_unmap(cpshm_t shm_t)
{
    if(0==shm_t || 0==shm_t->raw_addr)
        return CPDL_EINVAL;
#ifdef _NWCP_WIN32
    if (!UnmapViewOfFile(shm_t->raw_addr))
        return CPDL_ERROR;
#else
    if( -1 == shmdt(shm_t->raw_addr) )
        return CPDL_ERROR;
#endif
    shm_t->raw_addr = 0;
    return CPDL_SUCCESS;
}

/*! \brief 获取用户数据
    \param shm_t - 共享内存句柄
    \param pdata  - 共享内存用户区指针
    \param len   - 共享内存用户区大小
    \return CPDL_SUCCESS - 成功, CPDL_EINVAL - 参数错误, CPDL_ERROR - 失败
    \sa ::cpshm_map, ::cpshm_mapex
*/
int cpshm_data( const cpshm_t shm_t, cpshm_d *pdata, unsigned int *len)
{
    if( 0==shm_t || 0==shm_t->raw_addr)
        return CPDL_EINVAL;
    if(pdata)
        (*pdata) = ((char*)shm_t->raw_addr) + sizeof(struct _cpshm_privatedata);
    if(len)
        *len = *((unsigned int*)shm_t->raw_addr);
    return CPDL_SUCCESS;
}

/*! \brief 释放共享内存同时销毁共享内存句柄,如有必要会释放共享内存映射的地址
    \param shm_t - 共享内存句柄地址
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cpshm_create, ::cpshm_mapex, ::cpshm_close
*/
int cpshm_close(cpshm_t *shm_t)
{
#ifndef _NWCP_WIN32
    struct shmid_ds shmds;
#endif
    if(0==shm_t || 0==(*shm_t) )
        return CPDL_EINVAL;
    cpshm_unmap(*shm_t);
#ifdef _NWCP_WIN32
     CloseHandle((*shm_t)->handler);
#else
    memset (&shmds, 0, sizeof(struct shmid_ds));
    if(shmctl((*shm_t)->handler, IPC_STAT, &shmds) < 0 || 0==shmds.shm_nattch)
    {
        shmctl((*shm_t)->handler, IPC_RMID, NULL);
    }
    else
    {
        free((void*)(*shm_t));
        (*shm_t) = 0;
        return CPDL_AEXIST;
    }
#endif
    free((void*)(*shm_t));
    (*shm_t) = 0;
    return CPDL_SUCCESS;
}


/*! \brief 强制删除共享内存同时销毁共享内存句柄,如有必要会释放共享内存映射的地址
    \param shm_t - 共享内存句柄地址
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cpshm_create, ::cpshm_mapex, ::cpshm_close
*/
int cpshm_delete(cpshm_t *shm_t)
{
#ifdef _NWCP_WIN32
    return cpshm_close(shm_t);
#else
    if(0==shm_t || 0==(*shm_t) )
        return CPDL_EINVAL;
    cpshm_unmap(*shm_t);
    shmctl((*shm_t)->handler, IPC_RMID, NULL);
    free((void*)(*shm_t));
    (*shm_t) = 0;
#endif
    return CPDL_SUCCESS;
}

