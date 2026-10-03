#pragma once
#include "../../helpers/imports/imports.hpp"
#include "../../helpers/crt/crt.hpp"
#include "../../helpers/stack/stack.h"
#include "../globals/globals.hpp"
#include "../functions/functions.hpp"
#include "../mouse/mouse.hpp"

// PsGetProcessImageFileName is semi-documented; declare if your imports/
// namespace doesn't already wrap it. Returns short name (15 chars max).
extern "C" PCHAR NTAPI PsGetProcessImageFileName(IN PEPROCESS Process);

namespace major {

    // ------------------------------------------------------------------
    // Caller validation: only accept IOCTLs from safe-rage.exe.
    // The old static magic 0x1E2E3F was decorative — anyone with the
    // device handle + that constant (trivially extracted from a dump)
    // owned the kernel. This runs before any dispatch work.
    // ------------------------------------------------------------------
    static bool caller_is_allowed() {
        PEPROCESS proc = PsGetCurrentProcess();
        if (!proc) return false;

        PCHAR image_name = PsGetProcessImageFileName(proc);
        if (!image_name) return false;

        // EPROCESS ImageFileName is a 15-char fixed buffer.
        // "safe-rage.exe" is 13 chars so it fits in the compare.
        char expected[] = { 's','a','f','e','-','r','a','g','e','.','e','x','e', 0 };
        for (int i = 0; i < 14; i++) {
            if (image_name[i] != expected[i])
                return false;
            if (expected[i] == 0)
                break;
        }
        return true;
    }

    extern "C" NTSTATUS request_not_supported(PDEVICE_OBJECT devObj, PIRP irpRequest) {
        UNREFERENCED_PARAMETER(devObj);
        irpRequest->IoStatus.Status = STATUS_NOT_SUPPORTED;
        imports::IofCompleteRequest(irpRequest, IO_NO_INCREMENT);
        return irpRequest->IoStatus.Status;
    }

    extern "C" NTSTATUS request_dispatch(PDEVICE_OBJECT devObj, PIRP irpRequest) {
        UNREFERENCED_PARAMETER(devObj);
        irpRequest->IoStatus.Status = STATUS_SUCCESS;
        imports::IofCompleteRequest(irpRequest, IO_NO_INCREMENT);
        return irpRequest->IoStatus.Status;
    }

    extern "C" NTSTATUS io_controller(PDEVICE_OBJECT device_obj, PIRP irp) {
        UNREFERENCED_PARAMETER(device_obj);

        NTSTATUS status = STATUS_UNSUCCESSFUL;
        ULONG bytes = 0;

        PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(irp);
        ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
        ULONG size = stack->Parameters.DeviceIoControl.InputBufferLength;

        // ------------------------------------------------------------------
        // Caller validation first — reject anyone who isn't safe-rage.exe.
        // ------------------------------------------------------------------
        if (!caller_is_allowed()) {
            irp->IoStatus.Status = STATUS_ACCESS_DENIED;
            irp->IoStatus.Information = 0;
            imports::IofCompleteRequest(irp, IO_NO_INCREMENT);
            return STATUS_ACCESS_DENIED;
        }

        // SystemBuffer is a kernel-mode copy of the user input (DO_BUFFERED_IO)
        // so struct fields are safe to read. But USER POINTERS inside it —
        // Address, Buffer — still need probing before kernel deref/write.
        __try {
            if (code == 0x1889) {
                if (size != sizeof(Image)) {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else {
                    Image_ Req = (Image_)irp->AssociatedIrp.SystemBuffer;
                    if (!Req) {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else {
                        ProbeForWrite((PVOID)Req->Address, sizeof(ULONGLONG), __alignof(ULONGLONG));
                        status = functions::get_image(Req);
                        bytes  = NT_SUCCESS(status) ? sizeof(Image) : 0;
                    }
                }
            }
            else if (code == 0x1999) {
                if (size != sizeof(ReadWrite)) {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else {
                    ReadWrite_ Req = (ReadWrite_)irp->AssociatedIrp.SystemBuffer;
                    if (Req->Buffer && Req->Size) {
                        // probe the direction of transfer
                        if (Req->Write) {
                            ProbeForRead((PVOID)Req->Buffer, Req->Size, 1);
                        } else {
                            ProbeForWrite((PVOID)Req->Buffer, Req->Size, 1);
                        }
                    }
                    status = functions::read_handler(Req);
                    bytes  = sizeof(ReadWrite);
                }
            }
            else if (code == 0x1299) {
                if (size != sizeof(Dirbase)) {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else {
                    Dirbase_ Req = (Dirbase_)irp->AssociatedIrp.SystemBuffer;
                    status = functions::get_cr3(Req);
                    bytes  = sizeof(Dirbase);
                }
            }
            else if (code == 0x4299) {
                if (size != sizeof(DTB)) {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else {
                    DTB_ Req = (DTB_)irp->AssociatedIrp.SystemBuffer;
                    if (Req->Address) {
                        ProbeForWrite((PVOID)Req->Address, sizeof(uint64_t), __alignof(uint64_t));
                    }
                    status = functions::get_peb(Req);
                    bytes  = sizeof(DTB);
                }
            }
            else {
                status = STATUS_INVALID_DEVICE_REQUEST;
                bytes  = 0;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = STATUS_ACCESS_VIOLATION;
            bytes  = 0;
        }

        irp->IoStatus.Status      = status;
        irp->IoStatus.Information = bytes;
        imports::IofCompleteRequest(irp, IO_NO_INCREMENT);
        return status;
    }
}
