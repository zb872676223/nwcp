
#include "cpmisc.h"

#ifndef _NWCP_WIN32
#include <stdio.h>
#if defined(__osf__) && defined(__alpha)
#define OSF_ALPHA
#include <sys/table.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif

#ifdef HAVE_SYS_TIMEB_H
#include <sys/timeb.h>
#endif

#ifdef HAVE_FSTAB_H
#include <fstab.h>
#endif

#ifdef HAVE_KSTAT_H
#include <kstat.h>
#endif

#ifdef HAVE_SYS_MNTTAB_H
#include <sys/mnttab.h>
#endif

#endif

#ifdef _NWCP_WIN32

#define SystemBasicInformation       0
#define SystemPerformanceInformation 2
#define SystemTimeInformation        3

#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))

typedef struct structSYSTEM_BASIC_INFORMATION
{
    DWORD   dwUnknown1;
    ULONG   uKeMaximumIncrement;
    ULONG   uPageSize;
    ULONG   uMmNumberOfPhysicalPages;
    ULONG   uMmLowestPhysicalPage;
    ULONG   uMmHighestPhysicalPage;
    ULONG   uAllocationGranularity;
    PVOID   pLowestUserAddress;
    PVOID   pMmHighestUserAddress;
    ULONG   uKeActiveProcessors;
    BYTE    bKeNumberProcessors;
    BYTE    bUnknown2;
    WORD    wUnknown3;
} SYSTEM_BASIC_INFORMATION;

typedef struct structSYSTEM_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER   liIdleTime;
    DWORD           dwSpare[76];
} SYSTEM_PERFORMANCE_INFORMATION;

typedef struct structSYSTEM_TIME_INFORMATION
{
    LARGE_INTEGER liKeBootTime;
    LARGE_INTEGER liKeSystemTime;
    LARGE_INTEGER liExpTimeZoneBias;
    ULONG         uCurrentTimeZoneId;
    DWORD         dwReserved;
} SYSTEM_TIME_INFORMATION;


/* ntdll!NtQuerySystemInformation (NT specific!)

 The function copies the system information of the
 specified type into a buffer

 NTSYSAPI
 NTSTATUS
 NTAPI
 NtQuerySystemInformation(
    IN UINT SystemInformationClass,    // information type
    OUT PVOID SystemInformation,       // pointer to buffer
    IN ULONG SystemInformationLength,  // buffer size in bytes
    OUT PULONG ReturnLength OPTIONAL   // pointer to a 32-bit
                                       // variable that receives
                                       // the number of bytes
                                       // written to the buffer 
 ); */
typedef LONG (WINAPI *PROCNTQSI)(UINT,PVOID,ULONG,PULONG);

PROCNTQSI NtQuerySystemInformation;

#endif /* _NWCP_WIN32 */

#ifdef HAVE_KSTAT_H
static double __get_kstat_val(char *devname, char *class, kstat_named_t *knp)  
{  
     switch(knp->data_type) 
     {  
     case KSTAT_DATA_CHAR:  
         return 0;
     case KSTAT_DATA_INT32:  
         return knp->value.i32;
     case KSTAT_DATA_UINT32:  
         return knp->value.ui32;
     case KSTAT_DATA_INT64:  
         return knp->value.i64;
     case KSTAT_DATA_UINT64:  
         return knp->value.ui64;
     }
     return 0;
}
#endif

double cpcpuusage(double *last_idletime, double *last_systemtime)
{
#ifdef _NWCP_WIN32
    SYSTEM_PERFORMANCE_INFORMATION SysPerfInfo;
    SYSTEM_TIME_INFORMATION        SysTimeInfo;
    SYSTEM_BASIC_INFORMATION       SysBaseInfo;
    double                         dbIdleTime;
    double                         dbSystemTime;
    LONG                           status;
    double                         dbUsage = 0;

    NtQuerySystemInformation = (PROCNTQSI)GetProcAddress(
                                          GetModuleHandle("ntdll"),
                                         "NtQuerySystemInformation"
                                         );
    if(!NtQuerySystemInformation)
        return -1;

    status = NtQuerySystemInformation(SystemBasicInformation,&SysBaseInfo,sizeof(SysBaseInfo),NULL);
    if (status != NO_ERROR)
        return -1;
    
    status = NtQuerySystemInformation(SystemTimeInformation, 
        &SysTimeInfo, sizeof(SysTimeInfo), 0);
    if (status != NO_ERROR)
        return -1;
    
    status = NtQuerySystemInformation(SystemPerformanceInformation,
        &SysPerfInfo, sizeof(SysPerfInfo), NULL);
    if (status != NO_ERROR)
        return -1;
    
    if (*last_idletime != 0)
    {
        dbIdleTime = Li2Double(SysPerfInfo.liIdleTime) - *last_idletime;
        dbSystemTime = Li2Double(SysTimeInfo.liKeSystemTime) - *last_systemtime;
        
        dbIdleTime = dbIdleTime / dbSystemTime;
 
        dbUsage = 1 - dbIdleTime / (double)SysBaseInfo.bKeNumberProcessors;
    }
    
    *last_idletime = Li2Double(SysPerfInfo.liIdleTime);
    *last_systemtime = Li2Double(SysTimeInfo.liKeSystemTime);

    return dbUsage;
#elif defined(OSF_ALPHA)
    int num;
    struct tbl_sysinfo sysinfo;
    double idle_time, sycptime, usage;

    num = table(TBL_SYSINFO, 0, &sysinfo, 1, sizeof(sysinfo));
    if(num != 1)
        return -1;

    usage = 0;
    if(*last_idletime != 0)
    {
        idle_time = sysinfo.si_idle - *last_idletime;
        sycptime = (sysinfo.si_user + sysinfo.si_sys) - *last_systemtime;
        usage = 1 - (idle_time) / (sycptime+idle_time);
    }

    *last_idletime = sysinfo.si_idle;
    *last_systemtime = sysinfo.si_user + sysinfo.si_sys;

    return usage;
#elif defined(HAVE_KSTAT_H)  /* use kstat */
    kstat_ctl_t *kc = 0;
    kstat_t       *ksp;  
    kstat_io_t     kio;  
    kstat_named_t *knp;  
    kid_t kid;
    double idle_time, kernel_time, user_time, wait_time;
    double tmp_idle_time, tmp_sycptime;
    double usage;

    kc = kstat_open();
    if(!kc)
        return -1;
    ksp = kstat_lookup(kc, "cpu", -1, NULL);
    if(!ksp)
    {
        kstat_close(kc);
        return -1;
    }
    kid = kstat_read(kc, ksp, NULL);
    if(kid < 0)
    {
        kstat_close(kc);
        return -1;
    }
    knp = kstat_data_lookup(ksp, "cpu_ticks_idle");
    if(!knp)
    {
        kstat_close(kc);
        return -1;
    }
    idle_time = __get_kstat_val(ksp->ks_name, ksp->ks_class, knp);
    knp = kstat_data_lookup(ksp, "cpu_ticks_kernel");
    if(!knp)
    {
        kstat_close(kc);
        return -1;
    }
    kernel_time = __get_kstat_val(ksp->ks_name, ksp->ks_class, knp);
    knp = kstat_data_lookup(ksp, "cpu_ticks_user");
    if(!knp)
    {
        kstat_close(kc);
        return -1;
    }
    user_time = __get_kstat_val(ksp->ks_name, ksp->ks_class, knp);
    knp = kstat_data_lookup(ksp, "cpu_ticks_wait");
    if(!knp)
    {
        kstat_close(kc);
        return -1;
    }
    wait_time = __get_kstat_val(ksp->ks_name, ksp->ks_class, knp);
    /*printf("idle %.2f kernel %.2f user %.2f wait %.2f\n",
            idle_time, kernel_time, user_time, wait_time);*/
    kstat_close(kc);
    usage = 0;
    if(*last_idletime != 0)
    {
        tmp_idle_time = idle_time - *last_idletime;
        tmp_sycptime = (user_time+kernel_time+wait_time) - *last_systemtime;
        usage = 1 - (tmp_idle_time) / (tmp_sycptime+tmp_idle_time);
    }

    *last_idletime = idle_time;
    *last_systemtime = kernel_time + user_time + wait_time;

    return usage;
#else
    return -1;
#endif /* _NWCP_WIN32 */
}

double cpdiskusage()
{
#ifdef _NWCP_WIN32
    DWORD dwLogicalDriverBitmask;
    int i;
    char strDriverRootName[20];
    ULARGE_INTEGER liFreeBytesAvailable = {0, 0};
    double dbTotalNumberOfBytes = 0;
    double dbTotalNumberOfFreeBytes = 0;
    double dbDiskUsage;

    ULARGE_INTEGER liTempFreeBytes, liTempTotalBytes;

    dwLogicalDriverBitmask = GetLogicalDrives();

    for(i=0; i<32; i++)
    {
        if(dwLogicalDriverBitmask & (1 << i))
        {
            snprintf(strDriverRootName, 20, "%c:\\", 'A' + i);
            if(GetDriveType(strDriverRootName) != DRIVE_FIXED)
                continue;

            if(GetDiskFreeSpaceEx(strDriverRootName, &liFreeBytesAvailable,
                &liTempTotalBytes, &liTempFreeBytes))
            {
                dbTotalNumberOfBytes += Li2Double(liTempTotalBytes);
                dbTotalNumberOfFreeBytes += Li2Double(liTempFreeBytes);
            }
        }
    }

    if(dbTotalNumberOfBytes == 0)
        return -1;
    
    dbDiskUsage = 1 - dbTotalNumberOfFreeBytes / dbTotalNumberOfBytes;

    return dbDiskUsage;
/* #elif defined(OSF_ALPHA) */
#elif defined(HAVE_GETFSENT)
    int ret;
    struct statvfs vfs;
    double usage;
    int f_blocks, f_bfree;

    f_blocks = 0;
    f_bfree = 0;

    setfsent();

    while(1)
    {
        struct fstab *p_tab = getfsent();
        if(!p_tab)
            break;
        ret = statvfs(p_tab->fs_file, &vfs);
        if(ret)
            continue;
        
        f_blocks += vfs.f_blocks;
        f_bfree += vfs.f_bfree;
    }

    if(f_blocks)
        usage = ((double)(f_blocks - f_bfree)) / ((double)f_blocks);
    else
        usage = -1;

    return usage;
#elif defined(HAVE_GETMNTENT)
#define MNTTAB "/etc/mnttab"
    struct statvfs fsstat;
	struct mnttab my_mnttab;
	FILE *fp;
    double usage;

	fp = fopen(MNTTAB, "r");
 	if (fp == NULL) 
    {
 		printf("Unable to open mount table.\n");
 		return -1;
 	}

    int total_size=0, total_free=0;
    while (getmntent(fp, &my_mnttab) == 0)
    {
	    if (strncmp(my_mnttab.mnt_fstype, "ufs", 3) != 0)
		    continue;

        memset(&fsstat, 0, sizeof(struct statvfs));
        if (statvfs(my_mnttab.mnt_mountp, &fsstat) == 0) 
        {
            total_size += fsstat.f_blocks;
            total_free += fsstat.f_bavail;
        }

    }
    fclose(fp);

    if(!total_size)
        return -1;
    usage = (double)(total_size - total_free) / (double)total_size;
    return usage;
#else
    return -1;
#endif /* _NWCP_WIN32 */
}
