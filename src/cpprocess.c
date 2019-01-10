
#include "cpprocess.h"
#include "cpthread.h"
#include <string.h>

#ifdef _NWCP_WIN32
#include <process.h>
#else   /* UNIX */
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#endif  /* _NWCP_WIN32 */

/*! \file  cpprocess.h 
    \brief 进程管理
*/

void _create_cmdline(char *const argv[], char *cmdline)
{
    int i = 0;
    cmdline[0] = 0;
    while(argv[i])
    {
        strcat(cmdline, argv[i]);
        strcat(cmdline, " ");
        i ++;
    }
}

static int _create_argv(const char *cmd_line, const char *delim, char *** argvp)
{
    int i, token_num;
    const char *s;
    char *t;

    *argvp = 0;
    s = cmd_line + strspn(cmd_line, delim);
    t = malloc(strlen(s)+1);
    if(!t)
        return 0;
    strcpy(t, s);
    token_num = 0;
    if(strtok(t, delim))
        for(token_num = 1; strtok(0, delim); token_num++);
    *argvp = malloc((token_num+1)*sizeof(char *));
    if(!argvp)
    {
        free(t);
        return 0;
    }

    if(!token_num)
        free(t);
    else
        strcpy(t, s);

    **argvp = (char*)strtok(t, delim);
    for(i=1; i<token_num; i++)
        *((*argvp)+i) = (char*)strtok(0, delim);

    *((*argvp)+token_num) = 0;
    return token_num;
}

void _free_argv(char **argv)
{
    if(!argv)
        return;
    if(*argv)
        free(*argv);
    free(argv);
}

/*! \addtogroup CPROCESS 进程管理*/
/*@{*/

/*! \brief 创建进程
    \param cmd_line - 命令行(包括可执行文件路径及其命令行参数)
    \param pid - 返回进程PID
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cpprocess_wait, ::cpprocess_kill, ::sprocess_stop
    \remark 注意!!!此函数不是线程安全的!!!
*/
int cpprocess_create2(const char *cmd_line, cppid_t *pid)
{
    char **argv;
    int ret;

    if(!_create_argv(cmd_line, " ", &argv))
        return CPDL_ERROR;
    ret = cpprocess_create(argv, pid);
    _free_argv(argv);
    return ret;
}

/*! \addtogroup CPROCESS 进程管理*/
/*@{*/

/*! \brief 创建进程
    \param argv - 命令行参数指针数组,以0(空指针)结尾
    \param pid - 返回进程PID
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cpprocess_wait, ::cpprocess_kill, ::cpprocess_stop
*/
int cpprocess_create(char *const argv[], cppid_t *pid)
{
#ifdef _NWCP_WIN32
    char cmdline[512];
    BOOL success = 0;
    STARTUPINFOA startupInfo = { sizeof( STARTUPINFOA ), 0, 0, 0,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                                  };
    PROCESS_INFORMATION pinfo;
#else
    pid_t child_pid;
    int i;
#endif
    if(!argv || !pid)
        return CPDL_ERROR;
#ifdef _NWCP_WIN32
    _create_cmdline(argv, cmdline);
    success = CreateProcess(0, cmdline, 0, 0, FALSE, 0/*CREATE_NEW_CONSOLE*/, 0, 0,
        &startupInfo, &pinfo);
    if(success)
    {
        CloseHandle(pinfo.hThread);
        CloseHandle(pinfo.hProcess);
        *pid = pinfo.dwProcessId;
        //pid->tid = pinfo.dwThreadId;
        return CPDL_SUCCESS;
    }
    else
        return CPDL_ERROR;
#else
    *pid = -1;
    child_pid = fork();
    if(child_pid == -1)
        return CPDL_ERROR;
    else if(child_pid == 0)
    {
        for(i=1; i<256; i++)
            close(i);
        execvp(argv[0], argv);
        exit(1);
    }
    else
    {
        *pid = child_pid;
        return CPDL_SUCCESS;
    }
#endif
}

/*
#ifdef _NWCP_WIN32
static BOOL CALLBACK _terminate_app(HWND hwnd, LPARAM pid)
{
    DWORD current_pid = 0;
    GetWindowThreadProcessId(hwnd, &current_pid);
    if (current_pid == (DWORD)pid)
	    PostMessage(hwnd, WM_CLOSE, 0, 0);

    return TRUE;
}
#endif
*/

/*! \brief 等待子进程结束
    \param pid - 子进程PID
    \param timeout - 等待超时时间(毫秒)，如果为-1则永远等待
    \param exit_code - 子进程的返回代码
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \remarks 在UNIX系统下,父进程必须等待子进程结束,否则子进程会成为
        僵尸进程(zombie),一直驻留在系统中
    \sa ::cpprocess_wait_all, ::cpprocess_create
*/
int cpprocess_wait(cppid_t pid, int timeout, int *exit_code)
{
#ifdef _NWCP_WIN32
    HANDLE h_proc;
    DWORD ret;

    h_proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if(h_proc == NULL)
        return CPDL_SUCCESS;
    
    if(timeout == -1)
        ret = WaitForSingleObject(h_proc, INFINITE);
    else
        ret = WaitForSingleObject(h_proc, timeout);
    if(ret == WAIT_TIMEOUT)
    {
        CloseHandle(h_proc);
        return CPDL_ERROR;
    }

    if(exit_code)
    {
        GetExitCodeProcess(h_proc, &ret);
        *exit_code = (int)(ret);
    }
    CloseHandle(h_proc);
    return CPDL_SUCCESS;
#else
    int count = 0;
    int ret;
    if(!exit_code)
        return CPDL_ERROR;
    if(timeout == -1)
    {
        if((ret = waitpid(pid, exit_code, 0)) > 0)
            return CPDL_SUCCESS;
        /*char tmp[512];
        tmp[0] = 0;
        perror(tmp);
        printf("ret=%d errno=%d pid=%d %s code=%d\n", ret, errno, pid, tmp,
                *exit_code);*/
        return CPDL_ERROR;
    }
    while(!(ret=waitpid(pid, exit_code, WNOHANG)) && ++count <= timeout)
        cpthread_sleep(1);
    if(ret <= 0)
        return CPDL_ERROR;
    /*printf("ret=%d pid=%d exit_code=%d count=%d\n", 
            ret, pid, *exit_code, count);*/
    return CPDL_SUCCESS;
#endif
}

/*! \brief 等待任意子进程退出
    \param timeout - 等待超时时间(毫秒)
    \return CPDL_SUCCESS - 成功, CPDL_ERROR - 失败
    \sa ::cpprocess_wait, ::cpprocess_create
*/
int cpprocess_wait_all(int timeout)
{
#ifdef _NWCP_WIN32
    return CPDL_SUCCESS;
#else
    int exit_code;
    while(cpprocess_wait(-1, timeout, &exit_code) == CPDL_SUCCESS);
    return CPDL_SUCCESS;
#endif
}

/*! \brief 强行终止子进程
    \param pid - 进程PID
    \return 无
    \sa ::sprocess_stop, ::cpprocess_create
*/
void cpprocess_kill(cppid_t pid)
{
#ifdef _NWCP_WIN32
    HANDLE h_proc;

    h_proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if(h_proc == NULL)
        return;
    TerminateProcess(h_proc, 1);
    CloseHandle(h_proc);
#else
    kill(pid, SIGKILL);
#endif
}

/*! \brief 判断子进程是否在运行
    \param pid - 进程PID
    \return 1 - 运行， 0 - 不在运行
    \remarks 如果cpprocess_alive返回0，不需要调用cpprocess_wait
*/
int cpprocess_alive(cppid_t pid)
{
#ifdef _NWCP_WIN32
    HANDLE h_proc;
    BOOL ret;
    DWORD exit_code;

    h_proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if(h_proc == NULL)
        return 0;
    ret = GetExitCodeProcess(h_proc, &exit_code);
    CloseHandle(h_proc);
    if(!ret)
        return 0;
    if(exit_code == STILL_ACTIVE)
        return 1;
    return 0;
#else
    int ret;
    ret = kill(pid, 0);
    if(ret < 0)
        return 0;
    return 1;
#endif
}

/*! \brief 停止子进程
    \param pid - 进程PID
    \return 无
    \sa ::cpprocess_kill, ::cpprocess_create
*/
/*
void sprocess_stop(cppid_t pid)
{
#ifdef _NWCP_WIN32
    HANDLE h_proc;

    h_proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid.pid);
    if(h_proc == NULL)
        return;
    EnumWindows(_terminate_app, (LPARAM)(h_proc));
    PostThreadMessage(pid.tid, WM_CLOSE, 0, 0);
#else
    kill(pid, SIGSTOP);
#endif
}
*/

/*@}*/
