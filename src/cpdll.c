
#include "cpdll.h"
#ifndef _NWCP_WIN32
#include <dlfcn.h>
#endif

/*! \file  cpdll.h
    \brief 共享库的动态加载
*/

/*! \addtogroup CPDLL 共享库动态加载
    \remarks 在UNIX下, 使用dlopen；在WINDOWS下，使用LoadLibrary
*/
/*@{*/

/*! \brief 打开共享库
    \param filename - 共享库的文件名
    \return 成功返回库句柄，否则返回NULL
    \sa ::cpdll_close, ::cpdll_sym
*/
cpdll_t cpdll_open(const char *filename)
{
#ifdef _NWCP_WIN32
    return LoadLibrary(filename);
#else
    return dlopen(filename, RTLD_GLOBAL|RTLD_NOW);
#endif
}

/*! \brief 获取函数指针
   \param handle - 库的句柄(由cpdll_open()获得)
   \param sym_name - 函数的名称
   \return 成功返回函数指针，否则返回NULL
   \sa ::cpdll_open, ::cpdll_close
*/
cpdlsym_t cpdll_sym(cpdll_t dll_t, const char *sym_name)
{
#ifdef _NWCP_WIN32
    return GetProcAddress(dll_t, sym_name);
#else
    return dlsym(dll_t, sym_name);
#endif
}

/*! \brief 关闭共享库
   \param handle - 库的句柄(由cpdll_open()获得)
   \return 无
   \sa ::cpdll_open, ::cpdll_sym
*/
void cpdll_close(cpdll_t dll_t)
{
#ifdef _NWCP_WIN32
    FreeLibrary(dll_t);
#else
    dlclose(dll_t);
#endif
}

/*@}*/
