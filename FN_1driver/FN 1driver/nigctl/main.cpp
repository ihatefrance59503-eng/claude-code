// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include "core/creation/creation.hpp"

extern "C" int _fltused = 0;


//\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\x8D\x8C\x24\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00\x48\x85\xFF xxx????xxxx?xxxx????xx????xxx
// \x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\xFF\x15\x00\x00\x00\x00\x85\xC0\x78\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\xFF\x15\x00\x00\x00\x00\x8B\xD8\x4C\x39\x25 xxx????xxxx?xx????xxx?xxx????xxxx?xx????xxxxx

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT drv_obj, PUNICODE_STRING reg_pth) {
    // KEVLAR-patch: strip all obfuscation junk from DriverEntry.
    // The prime-testing loop at `for (i = 2; i * i <= PRIME_JUNK1; i++)`
    // relies on signed-int-overflow UB to terminate on real hardware
    // under /O2 optimization. In Unicorn the raw arithmetic runs
    // literally and the comparison is always true after overflow,
    // so the loop never terminates in emulation.
    imports::resolve_imports();
    UNREFERENCED_PARAMETER(reg_pth);
    UNREFERENCED_PARAMETER(drv_obj);
    return (imports::IoCreateDriver)(0, &creation::driver_init);
}
