#pragma once
#include "../../helpers/imports/imports.hpp"
#include "../../helpers/crt/crt.hpp"
#include "../../helpers/stack/stack.h"
#include "../globals/globals.hpp"

// ----------------------------------------------------------------------
// Physical memory read via MmCopyMemory + MM_COPY_MEMORY_PHYSICAL.
// Kept as a free function (not in a namespace) because clean.hpp also
// uses it on the PFN-walking paths.
// ----------------------------------------------------------------------
extern "C" NTSTATUS read_physical(PVOID target_address, PVOID buffer, SIZE_T size, SIZE_T* bytes_read) {
    if (!target_address || !buffer || !bytes_read)
        return STATUS_INVALID_PARAMETER;
    if (size == 0) {
        *bytes_read = 0;
        return STATUS_SUCCESS;
    }

    MM_COPY_ADDRESS to_read = { 0 };
    to_read.PhysicalAddress.QuadPart = (LONGLONG)target_address;
    NTSTATUS status = imports::MmCopyMemory(buffer, to_read, size, MM_COPY_MEMORY_PHYSICAL, bytes_read);
    if (!NT_SUCCESS(status))
        return status;
    if (*bytes_read > size)
        *bytes_read = size;
    return STATUS_SUCCESS;
}

namespace pml
{
    extern "C" auto FindPatternInMemory(PVOID startAddress, SIZE_T memorySize, const void* pattern, SIZE_T patternSize) -> PVOID {
        const auto* memStart   = static_cast<const UCHAR*>(startAddress);
        const auto* memPattern = static_cast<const UCHAR*>(pattern);

        for (SIZE_T i = 0; i <= memorySize - patternSize; ++i) {
            SIZE_T j = 0;
            while (j < patternSize && memStart[i + j] == memPattern[j]) ++j;
            if (j == patternSize)
                return const_cast<UCHAR*>(&memStart[i]);
        }
        return nullptr;
    }

    void* KernelDatabase = nullptr;

    extern "C" auto InitializePfnDatabase() -> NTSTATUS {
        struct PfnDatabasePattern {
            const UCHAR* bytePattern;
            SIZE_T       byteSize;
            bool         isHardcoded;
        };

        static const UCHAR win10x64Pattern[] = {
            0x48, 0x8B, 0xC1, 0x48, 0xC1, 0xE8, 0x0C, 0x48, 0x8D, 0x14, 0x40,
            0x48, 0x03, 0xD2, 0x48, 0xB8
        };
        static const UCHAR win11x64Pattern[] = {
            0x48, 0x8B, 0xC1, 0x48, 0xC1, 0xE8, 0x0C, 0x48, 0x8D, 0x0D, 0x00,
            0x00, 0x00, 0x00, 0x48, 0x8D, 0x14, 0x40, 0x48, 0x03, 0xD1
        };
        static const UCHAR win11_22H2Pattern[] = {
            0x48, 0x8B, 0xC1, 0x48, 0xC1, 0xE8, 0x0C, 0x48, 0x8D, 0x15, 0x00,
            0x00, 0x00, 0x00, 0x48, 0x8D, 0x0C, 0x40, 0x48, 0x03, 0xCA
        };

        PfnDatabasePattern pfnSearchConfig = {
            win10x64Pattern, sizeof(win10x64Pattern), true
        };

        auto getVirtualFn = reinterpret_cast<UCHAR*>(imports::MmGetVirtualForPhysical);
        if (!getVirtualFn)
            return STATUS_PROCEDURE_NOT_FOUND;

        auto resultAddr = reinterpret_cast<UCHAR*>(
            FindPatternInMemory(getVirtualFn, 0x20, pfnSearchConfig.bytePattern, pfnSearchConfig.byteSize));

        if (!resultAddr) {
            pfnSearchConfig.bytePattern = win11_22H2Pattern;
            pfnSearchConfig.byteSize    = sizeof(win11_22H2Pattern);
            resultAddr = reinterpret_cast<UCHAR*>(
                FindPatternInMemory(getVirtualFn, 0x20, pfnSearchConfig.bytePattern, pfnSearchConfig.byteSize));
        }
        if (!resultAddr)
            return STATUS_UNSUCCESSFUL;

        resultAddr += pfnSearchConfig.byteSize;
        if (pfnSearchConfig.isHardcoded) {
            KernelDatabase = *reinterpret_cast<void**>(resultAddr);
        }
        else {
            auto pfnAddr = *reinterpret_cast<ULONG_PTR*>(resultAddr);
            KernelDatabase = *reinterpret_cast<void**>(pfnAddr);
        }
        KernelDatabase = PAGE_ALIGN(KernelDatabase);
        return STATUS_SUCCESS;
    }

    extern "C" auto dirbase_from_base_address(void* processBase) -> uintptr_t {
        if (!NT_SUCCESS(pml::InitializePfnDatabase()))
            return 0;

        virt_addr_t virtualBase{};
        virtualBase.value = processBase;

        SIZE_T bytesRead = 0;
        auto physicalMemory = imports::MmGetPhysicalMemoryRanges();
        for (int i = 0; physicalMemory[i].BaseAddress.QuadPart; ++i) {
            auto& currentRange = physicalMemory[i];
            UINT64 physicalAddr = currentRange.BaseAddress.QuadPart;

            for (UINT64 offset = 0;
                 offset < currentRange.NumberOfBytes.QuadPart;
                 offset += 0x1000, physicalAddr += 0x1000)
            {
                auto pfn = reinterpret_cast<_MMPFN*>(
                    (uintptr_t)pml::KernelDatabase + ((physicalAddr >> 12) * sizeof(_MMPFN)));

                if (pfn->u4.PteFrame != (physicalAddr >> 12))
                    continue;

                MMPTE pml4Entry{};
                if (!NT_SUCCESS(read_physical(PVOID(physicalAddr + 8 * virtualBase.pml4_index),
                        &pml4Entry, 8, &bytesRead)))
                    continue;
                if (!pml4Entry.u.Hard.Valid)
                    continue;

                MMPTE pdptEntry{};
                if (!NT_SUCCESS(read_physical(PVOID((pml4Entry.u.Hard.PageFrameNumber << 12) + 8 * virtualBase.pdpt_index),
                        &pdptEntry, 8, &bytesRead)))
                    continue;
                if (!pdptEntry.u.Hard.Valid)
                    continue;

                MMPTE pdeEntry{};
                if (!NT_SUCCESS(read_physical(PVOID((pdptEntry.u.Hard.PageFrameNumber << 12) + 8 * virtualBase.pd_index),
                        &pdeEntry, 8, &bytesRead)))
                    continue;
                if (!pdeEntry.u.Hard.Valid)
                    continue;

                MMPTE pteEntry{};
                if (!NT_SUCCESS(read_physical(PVOID((pdeEntry.u.Hard.PageFrameNumber << 12) + 8 * virtualBase.pt_index),
                        &pteEntry, 8, &bytesRead)))
                    continue;
                if (!pteEntry.u.Hard.Valid)
                    continue;

                return physicalAddr;
            }
        }
        return 0;
    }
}

namespace functions
{
    extern "C" NTSTATUS get_image(Image_ X) {
        if (X->Security != SECURITY_FLAG)
            return STATUS_UNSUCCESSFUL;
        if (!X->ProcessID)
            return STATUS_UNSUCCESSFUL;

        PEPROCESS Process = NULL;
        imports::PsLookupProcessByProcessId((HANDLE)X->ProcessID, &Process);
        if (!Process)
            return STATUS_UNSUCCESSFUL;

        ULONGLONG ImageBase = (ULONGLONG)imports::PsGetProcessSectionBaseAddress(Process);
        if (!ImageBase) {
            imports::ObfDereferenceObject(Process);
            return STATUS_UNSUCCESSFUL;
        }

        crt::memcpy(X->Address, &ImageBase, sizeof(ImageBase));
        imports::ObfDereferenceObject(Process);
        return STATUS_SUCCESS;
    }

    extern "C" ULONG64 find_min(INT32 g, SIZE_T f) {
        INT32 h = (INT32)f;
        return (g < h) ? (ULONG64)g : (ULONG64)h;
    }

    extern "C" NTSTATUS get_peb(DTB_ data) {
        uint64_t pid         = data->ProcessID;
        uint64_t output_addr = data->Address;

        PEPROCESS process = nullptr;
        NTSTATUS status = imports::PsLookupProcessByProcessId((HANDLE)pid, &process);
        if (!NT_SUCCESS(status) || !process)
            return STATUS_UNSUCCESSFUL;

        PPEB peb = imports::PsGetProcessPeb(process);
        uint64_t peb_addr = (uint64_t)peb;
        crt::memcpy((void*)output_addr, &peb_addr, sizeof(uint64_t));

        imports::ObfDereferenceObject(process);
        return STATUS_SUCCESS;
    }

    extern "C" UINT64 translate_linear(UINT64 directoryTableBase, UINT64 virtualAddress) {
        directoryTableBase &= ~0xf;

        UINT64 pageOffset = virtualAddress & ~(~0ul << PAGE_OFFSET_SIZE);
        UINT64 pte = ((virtualAddress >> 12) & (0x1ffll));
        UINT64 pt  = ((virtualAddress >> 21) & (0x1ffll));
        UINT64 pd  = ((virtualAddress >> 30) & (0x1ffll));
        UINT64 pdp = ((virtualAddress >> 39) & (0x1ffll));

        SIZE_T readsize = 0;

        UINT64 pdpe = cached_pml4e1[pdp];
        if (~pdpe & 1)
            return 0;

        UINT64 pde = 0;
        read_physical(PVOID((pdpe & PMASK) + 8 * pd), &pde, sizeof(pde), &readsize);
        if (~pde & 1)
            return 0;

        if (pde & 0x80)
            return (pde & (~0ull << 42 >> 12)) + (virtualAddress & ~(~0ull << 30));

        UINT64 pteAddr = 0;
        read_physical(PVOID((pde & PMASK) + 8 * pt), &pteAddr, sizeof(pteAddr), &readsize);
        if (~pteAddr & 1)
            return 0;

        if (pteAddr & 0x80)
            return (pteAddr & PMASK) + (virtualAddress & ~(~0ull << 21));

        virtualAddress = 0;
        read_physical(PVOID((pteAddr & PMASK) + 8 * pte), &virtualAddress, sizeof(virtualAddress), &readsize);
        virtualAddress &= PMASK;
        if (!virtualAddress)
            return 0;

        return virtualAddress + pageOffset;
    }

    extern "C" NTSTATUS write_physical(PVOID destBuf, PVOID srcPhys, SIZE_T size, SIZE_T* copiedSize) {
        PHYSICAL_ADDRESS AddrToWrite = { 0 };
        AddrToWrite.QuadPart = LONGLONG(destBuf);
        PVOID pmapped_mem = imports::MmMapIoSpaceEx(AddrToWrite, size, PAGE_READWRITE);
        if (!pmapped_mem) return STATUS_UNSUCCESSFUL;

        if (!imports::MmIsAddressValid(pmapped_mem)) {
            imports::MmUnmapIoSpace(pmapped_mem, size);
            return STATUS_ACCESS_VIOLATION;
        }

        crt::memcpy(pmapped_mem, srcPhys, size);
        *copiedSize = size;
        imports::MmUnmapIoSpace(pmapped_mem, size);
        return STATUS_SUCCESS;
    }

    extern "C" NTSTATUS read_memory(PVOID target_address, PVOID buffer, SIZE_T size, SIZE_T* bytes_read) {
        return read_physical(target_address, buffer, size, bytes_read);
    }

    // ------------------------------------------------------------------
    // read_handler: FIXED to loop across page boundaries.
    //
    // Old code did ONE translate_linear + ONE chunk_size = min(remaining
    // in page, total), then returned STATUS_SUCCESS without advancing
    // this_offset. A read of 2KB starting 3KB into a page = partial read
    // of 1KB with no error returned. For a struct crossing a page the
    // caller got garbage past the boundary.
    //
    // New code loops, re-translating each chunk since consecutive virtual
    // pages may be at non-consecutive physical pages.
    // ------------------------------------------------------------------
    extern "C" NTSTATUS read_handler(ReadWrite_ X) {
        if (X->Security != SECURITY_FLAG)
            return STATUS_UNSUCCESSFUL;
        if (!X->ProcessID)
            return STATUS_UNSUCCESSFUL;
        if (!X->Address || !X->Buffer)
            return STATUS_INVALID_PARAMETER;
        if (X->Size == 0 || X->Size > 0x10000000)
            return STATUS_INVALID_PARAMETER;

        SIZE_T this_offset = 0;
        SIZE_T total_size  = X->Size;

        while (total_size > 0) {
            UINT64 physical_address = translate_linear(m_stored_dtb,
                (ULONG64)X->Address + this_offset);
            if (physical_address == 0)
                return STATUS_UNSUCCESSFUL;

            ULONG64 chunk_size = find_min(
                (INT32)(PAGE_SIZE - (physical_address & 0xFFF)),
                total_size);
            if (chunk_size == 0)
                return STATUS_UNSUCCESSFUL;

            SIZE_T bytes_through = 0;
            NTSTATUS st;
            if (X->Write) {
                st = write_physical(
                    PVOID(physical_address),
                    (PVOID)((ULONG64)X->Buffer + this_offset),
                    chunk_size, &bytes_through);
            } else {
                st = read_memory(
                    PVOID(physical_address),
                    (PVOID)((ULONG64)X->Buffer + this_offset),
                    chunk_size, &bytes_through);
            }

            if (!NT_SUCCESS(st))
                return st;
            if (bytes_through != chunk_size)
                return STATUS_UNSUCCESSFUL;

            this_offset += chunk_size;
            total_size  -= chunk_size;
        }
        return STATUS_SUCCESS;
    }

    // ------------------------------------------------------------------
    // get_cr3: now actually returns a status. Old code fell off the end
    // of the function; MSVC inserted an implicit zero return (which just
    // happens to be STATUS_SUCCESS) but it was UB-adjacent.
    // ------------------------------------------------------------------
    extern "C" NTSTATUS get_cr3(Dirbase_ X) {
        if (X->Security != SECURITY_FLAG)
            return STATUS_UNSUCCESSFUL;
        if (!X->ProcessID)
            return STATUS_UNSUCCESSFUL;

        size_t bytes{};
        PEPROCESS DTB{};
        NTSTATUS st = imports::PsLookupProcessByProcessId((HANDLE)X->ProcessID, &DTB);
        if (!NT_SUCCESS(st) || !DTB)
            return STATUS_UNSUCCESSFUL;

        m_stored_dtb = pml::dirbase_from_base_address(
            imports::PsGetProcessSectionBaseAddress(DTB));

        if (m_stored_dtb == 0) {
            imports::ObfDereferenceObject(DTB);
            return STATUS_UNSUCCESSFUL;
        }

        for (int i = 0; i < 512; i++) {
            read_physical(PVOID(m_stored_dtb + 8 * i),
                &cached_pml4e1[i], 8, &bytes);
        }
        imports::ObfDereferenceObject(DTB);
        return STATUS_SUCCESS;
    }
}
