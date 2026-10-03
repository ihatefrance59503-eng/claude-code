// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include "../../helpers/imports/imports.hpp"
#include "../../helpers/crt/crt.hpp"
#include "../../helpers/stack/stack.h"
#include "../globals/globals.hpp"
#include "../clean/clean.hpp"
#include "../major/major.hpp"

namespace creation {
	NTSTATUS driver_init(PDRIVER_OBJECT drv_obj, PUNICODE_STRING reg_pth) {
		//globals::ntos_image_base = (L"ntoskrnl.exe");
		volatile unsigned char BYTE_JUNK1[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };
		volatile unsigned char BYTE_JUNK2[8] = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10 };

		for (volatile int round = 0; round < 8; round++) {
			for (volatile int i = 0; i < 8; i++) {
				BYTE_JUNK1[i] = (BYTE_JUNK1[i] + BYTE_JUNK2[7 - i]) ^ (round * 0x11);
				BYTE_JUNK2[i] = (BYTE_JUNK2[i] - BYTE_JUNK1[i]) | (i * 0x0F);
			}
		}

		volatile int recursive_junk = 100;
		while (recursive_junk > 0) {
			recursive_junk = recursive_junk - ((recursive_junk % 7) + 1);
		}

		volatile double complex_junk_calc = 0.0;
		for (volatile int n = 1; n <= 50; n++) {
			complex_junk_calc += 1.0 / (n * n);
		}
		PDEVICE_OBJECT device_obj = { };

		imports::RtlInitUnicodeString_f(&globals::DeviceN, encrypt(L"\\Device\\WinKernelInterface"));
		imports::RtlInitUnicodeString_f(&globals::DosL, encrypt(L"\\DosDevices\\WinKernelInterface"));
		clean::clear_hash_bucket(UNICODE_STRING(RTL_CONSTANT_STRING(L"nigctl.sys")));
		clean::CleanMmu(UNICODE_STRING(RTL_CONSTANT_STRING(L"nigctl.sys")));
		clean::clearCache(UNICODE_STRING(RTL_CONSTANT_STRING(L"nigctl.sys")), 1698136146);
		imports::IoCreateDevice(drv_obj, 0, &globals::DeviceN, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_obj);
		imports::IoCreateSymbolicLink(&globals::DosL, &globals::DeviceN);
		clean::clean_extras(drv_obj);
		drv_obj->DriverStart = NULL;
		drv_obj->DriverSize = 0;
		drv_obj->DriverInit = NULL;
		if (drv_obj->DriverExtension) {
			drv_obj->DriverExtension->ServiceKeyName.Buffer = nullptr;
			drv_obj->DriverExtension->ServiceKeyName.Length = 0;
			drv_obj->DriverExtension->ServiceKeyName.MaximumLength = 0;
		}
		drv_obj->DriverName.Buffer = nullptr;
		drv_obj->DriverName.Length = 0;
		drv_obj->DriverName.MaximumLength = 0;
		for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
			drv_obj->MajorFunction[i] = &major::request_not_supported;
		device_obj->Flags |= DO_BUFFERED_IO;
		drv_obj->MajorFunction[IRP_MJ_CREATE] = &major::request_dispatch;
		drv_obj->MajorFunction[IRP_MJ_CLOSE] = &major::request_dispatch;
		drv_obj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = &major::io_controller;
		drv_obj->DriverSection = NULL;

		device_obj->Flags &= ~DO_DEVICE_INITIALIZING;
		return STATUS_SUCCESS;
	}
}