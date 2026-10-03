// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include "core/creation/creation.hpp"

extern "C" int _fltused = 0;


//\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\x8D\x8C\x24\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00\x48\x85\xFF xxx????xxxx?xxxx????xx????xxx
// \x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\xFF\x15\x00\x00\x00\x00\x85\xC0\x78\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\xFF\x15\x00\x00\x00\x00\x8B\xD8\x4C\x39\x25 xxx????xxxx?xx????xxx?xxx????xxxx?xx????xxxxx

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT drv_obj, PUNICODE_STRING reg_pth) {
    // KEVLAR-patch: strip all obfuscation junk from DriverEntry (see git log).
    imports::resolve_imports();

    // KEVLAR-patch: bypass IoCreateDriver, call driver_init directly.
    // The real driver uses IoCreateDriver(0, init) to synthesize a fresh
    // DRIVER_OBJECT for anti-detection (hides which driver file is
    // registered). KEVLAR's generic auto-stub returns 0 without ever
    // invoking the init function in RDX, so driver_init never runs.
    // Call it directly with the DriverEntry args instead.
    return creation::driver_init(drv_obj, reg_pth);
}
