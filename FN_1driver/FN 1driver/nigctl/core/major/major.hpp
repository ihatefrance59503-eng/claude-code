// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include "../../helpers/imports/imports.hpp"
#include "../../helpers/crt/crt.hpp"
#include "../../helpers/stack/stack.h"
#include "../globals/globals.hpp"
#include "../functions/functions.hpp"
#include "../mouse/mouse.hpp"

namespace major {
	extern "C" NTSTATUS request_not_supported(PDEVICE_OBJECT devObj, PIRP irpRequest) {
		UNREFERENCED_PARAMETER(devObj);
		irpRequest->IoStatus.Status = STATUS_NOT_SUPPORTED;
		imports::IofCompleteRequest(irpRequest, IO_NO_INCREMENT);
		return irpRequest->IoStatus.Status;
	}
	extern "C" NTSTATUS request_dispatch(PDEVICE_OBJECT devObj, PIRP irpRequest) {
		UNREFERENCED_PARAMETER(devObj);
		PIO_STACK_LOCATION stackLocation = IoGetCurrentIrpStackLocation(irpRequest);
		switch (stackLocation->MajorFunction) {
		case IRP_MJ_CREATE:
			break;
		case IRP_MJ_CLOSE:
			break;
		default:
			break;
		}
		imports::IofCompleteRequest(irpRequest, IO_NO_INCREMENT);
		return irpRequest->IoStatus.Status;
	}
	extern "C" NTSTATUS io_controller(PDEVICE_OBJECT device_obj, PIRP irp) {
		UNREFERENCED_PARAMETER(device_obj);
		NTSTATUS status = { };
		ULONG bytes = { };
		PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(irp);
		ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
		ULONG size = stack->Parameters.DeviceIoControl.InputBufferLength;
		if (code == 0x1889)
{
    if (size == sizeof(Image))
    {
        Image_ Req = (Image_)irp->AssociatedIrp.SystemBuffer;
        if (!Req)
        {
            status = STATUS_INVALID_PARAMETER;
            bytes = 0;
        }
        else
        {
            status = functions::get_image(Req);

            if (NT_SUCCESS(status))
            {
                bytes = sizeof(Image);
            }
            else
            {
                bytes = 0;
            }
        }
    }
    else
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
        bytes = 0;
    }
}
		else if (code == 0x1999)
		{
			if (size == sizeof(ReadWrite))
			{
				ReadWrite_ Req = (ReadWrite_)irp->AssociatedIrp.SystemBuffer;
				status = functions::read_handler(Req);
				bytes = sizeof(ReadWrite);

			}
			else
			{
				status = STATUS_INFO_LENGTH_MISMATCH;
				bytes = 0;
			}
		}
		else if (code == 0x1299)
		{
			if (size == sizeof(Dirbase))
			{
				Dirbase_ Req = (Dirbase_)irp->AssociatedIrp.SystemBuffer;
				status = functions::get_cr3(Req);
				bytes = sizeof(Dirbase);

			}
			else
			{
				status = STATUS_INFO_LENGTH_MISMATCH;
				bytes = 0;
			}
		}
		else if (code == 0x4299)
		{
			if (size == sizeof(DTB))
			{
				DTB_ Req = (DTB_)irp->AssociatedIrp.SystemBuffer;
				status = functions::get_peb(Req);
				bytes = sizeof(DTB);

			}
			else
			{
				status = STATUS_INFO_LENGTH_MISMATCH;
				bytes = 0;
			}
		}

		//if (code == ReadMem) {
		//	if (size == sizeof(_rw)) {
		//		prw req = (prw)(irp->AssociatedIrp.SystemBuffer);

		//		status = ReadFunc(req);
		//		bytes = sizeof(_rw);
		//	}
		//	else
		//	{
		//		status = STATUS_INFO_LENGTH_MISMATCH;
		//		bytes = 0;
		//	}
		//}

		//else if (code == Pml4) {
		//	DTBStruct req = (DTBStruct)(irp->AssociatedIrp.SystemBuffer);

		//	status = STATUS_SUCCESS;
		//	DTBBFIX((HANDLE)req->process_id);
		//	bytes = sizeof(DTBStruct);
		//}

		//else if (code == BaseAdd) {
		//	if (size == sizeof(_ba)) {
		//		pba req = (pba)(irp->AssociatedIrp.SystemBuffer);
		//		status = BaseAddy(req);
		//		bytes = sizeof(_ba);
		//	}
		//	else
		//	{
		//		status = STATUS_INFO_LENGTH_MISMATCH;
		//		bytes = 0;
		//	}
		//}
		irp->IoStatus.Status = status;
		irp->IoStatus.Information = bytes;
		imports::IofCompleteRequest(irp, IO_NO_INCREMENT);
		return status;

	}

}