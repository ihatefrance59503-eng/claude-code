// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "extras.hpp"

float fortnite::extras::get_cpu( )
{
    static ULARGE_INTEGER lastIdleTime = { 0 } , lastKernelTime = { 0 } , lastUserTime = { 0 };

    FILETIME idleTime , kernelTime , userTime;
    if ( !GetSystemTimes( &idleTime , &kernelTime , &userTime ) )
        return 0.0f;

    ULARGE_INTEGER idle , kernel , user;

    idle.LowPart = idleTime.dwLowDateTime;
    idle.HighPart = idleTime.dwHighDateTime;

    kernel.LowPart = kernelTime.dwLowDateTime;
    kernel.HighPart = kernelTime.dwHighDateTime;

    user.LowPart = userTime.dwLowDateTime;
    user.HighPart = userTime.dwHighDateTime;

    ULONGLONG sysIdle = idle.QuadPart - lastIdleTime.QuadPart;
    ULONGLONG sysKernel = kernel.QuadPart - lastKernelTime.QuadPart;
    ULONGLONG sysUser = user.QuadPart - lastUserTime.QuadPart;

    ULONGLONG sysTotal = sysKernel + sysUser;
    float cpu = 0.0f;

    if ( sysTotal > 0 )
        cpu = ( float ) ( ( sysTotal - sysIdle ) * 100.0 / sysTotal );

    lastIdleTime = idle;
    lastKernelTime = kernel;
    lastUserTime = user;

    return cpu;
}
