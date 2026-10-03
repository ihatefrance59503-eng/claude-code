#pragma once
#include "../../helpers/imports/imports.hpp"
#include "../../helpers/crt/crt.hpp"
#include "../../helpers/stack/stack.h"
#include "../globals/globals.hpp"
#include "../clean/clean.hpp"
#include "../major/major.hpp"

// KEVLAR_BUILD define is injected by VS2022 preprocessor on the emu config
// (Project > Properties > C/C++ > Preprocessor > Preprocessor Definitions).
// Release builds OMIT it so the four clean::* calls run on real hardware.
// This replaces the manual commit-37d40fc comment/uncomment dance.

namespace creation {
    NTSTATUS driver_init(PDRIVER_OBJECT drv_obj, PUNICODE_STRING reg_pth) {
        UNREFERENCED_PARAMETER(reg_pth);

        PDEVICE_OBJECT device_obj = nullptr;

        imports::RtlInitUnicodeString_f(&globals::DeviceN, encrypt(L"\\Device\\WinKernelInterface"));
        imports::RtlInitUnicodeString_f(&globals::DosL,    encrypt(L"\\DosDevices\\WinKernelInterface"));

#ifndef KEVLAR_BUILD
        // real-hardware stealth: scrub PiDDBCacheTable, MmUnloadedDrivers, and
        // their caches. these ALL pattern-scan into unmapped ntoskrnl/ci.dll
        // regions inside KEVLAR so they're gated out of the emu build.
        clean::clear_hash_bucket(UNICODE_STRING(RTL_CONSTANT_STRING(L"nigctl.sys")));
        clean::CleanMmu(UNICODE_STRING(RTL_CONSTANT_STRING(L"nigctl.sys")));
        clean::clearCache(UNICODE_STRING(RTL_CONSTANT_STRING(L"nigctl.sys")), 1698136146);
#endif

        NTSTATUS st = imports::IoCreateDevice(
            drv_obj, 0, &globals::DeviceN,
            FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE,
            &device_obj);
        if (!NT_SUCCESS(st) || !device_obj)
            return NT_SUCCESS(st) ? STATUS_UNSUCCESSFUL : st;

        st = imports::IoCreateSymbolicLink(&globals::DosL, &globals::DeviceN);
        if (!NT_SUCCESS(st)) {
            imports::IoDeleteDevice(device_obj);
            return st;
        }

#ifndef KEVLAR_BUILD
        // clean_extras must run AFTER the device/symlink are live but BEFORE
        // we null out drv_obj->DriverSection below (clean_extras walks it).
        clean::clean_extras(drv_obj);
#endif

        drv_obj->DriverStart = NULL;
        drv_obj->DriverSize  = 0;
        drv_obj->DriverInit  = NULL;
        if (drv_obj->DriverExtension) {
            drv_obj->DriverExtension->ServiceKeyName.Buffer        = nullptr;
            drv_obj->DriverExtension->ServiceKeyName.Length        = 0;
            drv_obj->DriverExtension->ServiceKeyName.MaximumLength = 0;
        }
        drv_obj->DriverName.Buffer        = nullptr;
        drv_obj->DriverName.Length        = 0;
        drv_obj->DriverName.MaximumLength = 0;

        for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
            drv_obj->MajorFunction[i] = &major::request_not_supported;

        device_obj->Flags |= DO_BUFFERED_IO;
        drv_obj->MajorFunction[IRP_MJ_CREATE]         = &major::request_dispatch;
        drv_obj->MajorFunction[IRP_MJ_CLOSE]          = &major::request_dispatch;
        drv_obj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = &major::io_controller;
        drv_obj->DriverSection = NULL;

        device_obj->Flags &= ~DO_DEVICE_INITIALIZING;
        return STATUS_SUCCESS;
    }
}
