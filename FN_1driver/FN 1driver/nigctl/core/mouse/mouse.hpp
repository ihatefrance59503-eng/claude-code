// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include "../../helpers/imports/imports.hpp"
#include "../../helpers/crt/crt.hpp"
#include "../../helpers/stack/stack.h"
#include "../globals/globals.hpp"

namespace mouse {
	/*extern "C" POBJECT_TYPE* IoDriverObjectType;

	extern "C" NTSTATUS init_mouse(PMOUSE_OBJECT MouseObj) {
		UNICODE_STRING ClassString;
		imports::RtlInitUnicodeString(&ClassString, encrypt(L"\\Driver\\MouClass"));

		PDRIVER_OBJECT ClassDriverObject = NULL;
		NTSTATUS Status = imports::ObReferenceObjectByName(&ClassString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&ClassDriverObject);
		if (!NT_SUCCESS(Status)) { return Status; }

		UNICODE_STRING HidString;
		RtlInitUnicodeString(&HidString, encrypt(L"\\Driver\\MouHID"));

		PDRIVER_OBJECT HidDriverObject = NULL;
		Status = imports::ObReferenceObjectByName(&HidString, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&HidDriverObject);

		if (!NT_SUCCESS(Status))
		{
			if (ClassDriverObject)
			{
				imports::ObfDereferenceObject(ClassDriverObject);
			}

			return Status;
		}

		PVOID ClassDriverBase = NULL;

		PDEVICE_OBJECT HidDeviceObject = HidDriverObject->DeviceObject;
		while (HidDeviceObject && !MouseObj->ServiceCallback)
		{
			PDEVICE_OBJECT ClassDeviceObject = ClassDriverObject->DeviceObject;
			while (ClassDeviceObject && !MouseObj->ServiceCallback)
			{
				if (!ClassDeviceObject->NextDevice && !MouseObj->MouseDevice)
				{
					MouseObj->MouseDevice = ClassDeviceObject;
				}

				PULONG_PTR DeviceExtension = (PULONG_PTR)HidDeviceObject->DeviceExtension;
				ULONG_PTR DeviceExternalSize = ((ULONG_PTR)HidDeviceObject->DeviceObjectExtension - (ULONG_PTR)HidDeviceObject->DeviceExtension) / 4;
				ClassDriverBase = ClassDriverObject->DriverStart;
				for (ULONG_PTR i = 0; i < DeviceExternalSize; i++)
				{
					if (DeviceExtension[i] == (ULONG_PTR)ClassDeviceObject && DeviceExtension[i + 1] > (ULONG_PTR)ClassDriverObject)
					{
						MouseObj->ServiceCallback = (MouseClassServiceCallback)(DeviceExtension[i + 1]);
						break;
					}
				}
				ClassDeviceObject = ClassDeviceObject->NextDevice;
			}

			HidDeviceObject = HidDeviceObject->AttachedDevice;
		}

		if (!MouseObj->MouseDevice)
		{
			PDEVICE_OBJECT TargetDeviceObject = ClassDriverObject->DeviceObject;

			while (TargetDeviceObject)
			{
				if (!TargetDeviceObject->NextDevice)
				{
					MouseObj->MouseDevice = TargetDeviceObject;
					break;
				}
				TargetDeviceObject = TargetDeviceObject->NextDevice;
			}
		}

		imports::ObfDereferenceObject(ClassDriverObject);
		imports::ObfDereferenceObject(HidDriverObject);

		return STATUS_SUCCESS;
	}
	extern "C" NTSTATUS move_mouse(MOUSE_OBJECT MouseObj, long x, long y, unsigned short button_flags) {
		init_mouse(&MouseObj);
		ULONG InputData;
		KIRQL IRQL = 0;
		MOUSE_INPUT_DATA mid = { 0 };

		mid.LastX = x;
		mid.LastY = y;
		mid.ButtonFlags = button_flags;

		imports::KfRaiseIrql(IRQL);
		MouseObj.ServiceCallback(MouseObj.MouseDevice, &mid, (PMOUSE_INPUT_DATA)&mid + 1, &InputData);
		imports::KeLowerIrql(IRQL);
		return STATUS_SUCCESS;
	}*/
}