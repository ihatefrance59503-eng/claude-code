#pragma once
#include "helpers.hpp"
#define MM_UNLOADED_DRIVERS_SIZE 50

/* 1903, 1909, 2004, 20H2, 21H1, 22H2 */
#define MmuPattern "\x4C\x8B\x15\x00\x00\x00\x00\x4C\x8B\xC9"
#define MmuMask    "xxx????xxx"

/* 1903, 1909, 2004, 20H2, 21H1, 22H2 */
#define MmlPattern "\x8B\x05\x00\x00\x00\x00\x83\xF8\x32"
#define MmlMask    "xx????xxx"

namespace clean {

    // ------------------------------------------------------------------
    // clear_hash_bucket: wipe PiDDBCacheTable entry for DriverName
    // ------------------------------------------------------------------
    BOOL clear_hash_bucket(UNICODE_STRING DriverName) {
        char* CIDLLString = encrypt("ci.dll");
        CONST PVOID CIDLLBase = GetKernelModuleBase(CIDLLString);
        if (!CIDLLBase)
            return 1;

        char* pKernelBucketHashPattern_21H1 = encrypt(KernelBucketHashPattern_21H1);
        char* pKernelBucketHashMask_21H1    = encrypt(KernelBucketHashMask_21H1);
        char* pKernelBucketHashPattern_22H2 = encrypt(KernelBucketHashPattern_22H2);
        char* pKernelBucketHashMask_22H2    = encrypt(KernelBucketHashMask_22H2);

        PVOID SignatureAddress = FindPatternImage((PCHAR)CIDLLBase,
            pKernelBucketHashPattern_21H1, pKernelBucketHashMask_21H1);
        if (!SignatureAddress) {
            SignatureAddress = FindPatternImage((PCHAR)CIDLLBase,
                pKernelBucketHashPattern_22H2, pKernelBucketHashMask_22H2);
            if (!SignatureAddress)
                return 1;
        }

        CONST ULONGLONG* g_KernelHashBucketList =
            (ULONGLONG*)ResolveRelativeAddress(SignatureAddress, 3, 7);
        if (!g_KernelHashBucketList)
            return 1;

        LARGE_INTEGER Time{};
        imports::KeQuerySystemTimePrecise(&Time);

        BOOL Status = FALSE;
        for (ULONGLONG i = *g_KernelHashBucketList; i; i = *(ULONGLONG*)i) {
            CONST PWCHAR wsName = PWCH(i + 0x48);
            if (crt::wcscmp(wsName, DriverName.Buffer)) {
                PUCHAR Hash = PUCHAR(i + 0x18);
                for (UINT j = 0; j < 20; j++)
                    Hash[j] = UCHAR(imports::RtlRandomEx(&Time.LowPart) % 255);
                Status = TRUE;
            }
        }
        return Status ? 0 : 1;
    }

    // ------------------------------------------------------------------
    // MmUnloadedDrivers helpers (pattern resolve + validation)
    // ------------------------------------------------------------------
    PMM_UNLOADED_DRIVER GetMmuAddress() {
        PCHAR base = (PCHAR)GetKernelBase2();
        char* pMmuPattern = encrypt(MmuPattern);
        char* pMmuMask    = encrypt(MmuMask);
        PVOID MmUnloadedDriversInstr = FindPatternImage(base, pMmuPattern, pMmuMask);
        if (!MmUnloadedDriversInstr)
            return nullptr;
        return *(PMM_UNLOADED_DRIVER*)ResolveRelativeAddress(MmUnloadedDriversInstr, 3, 7);
    }

    PULONG GetMmlAddress() {
        PCHAR Base = (PCHAR)GetKernelBase2();
        char* pMmlPattern = encrypt(MmlPattern);
        char* pMmlMask    = encrypt(MmlMask);
        PVOID mmlastunloadeddriverinst = FindPatternImage(Base, pMmlPattern, pMmlMask);
        if (!mmlastunloadeddriverinst)
            return nullptr;
        return (PULONG)ResolveRelativeAddress(mmlastunloadeddriverinst, 2, 6);
    }

    BOOL VerifyMmu() {
        return (GetMmuAddress() != nullptr && GetMmlAddress() != nullptr);
    }

    BOOL IsUnloadEmpty(PMM_UNLOADED_DRIVER Entry) {
        return (Entry->Name.MaximumLength == 0 ||
                Entry->Name.Length == 0 ||
                Entry->Name.Buffer == NULL);
    }

    BOOL IsMmuFilled() {
        PMM_UNLOADED_DRIVER base = GetMmuAddress();
        if (!base) return FALSE;
        for (ULONG Idx = 0; Idx < MM_UNLOADED_DRIVERS_SIZE; ++Idx) {
            if (IsUnloadEmpty(&base[Idx]))
                return FALSE;
        }
        return TRUE;
    }

    // ------------------------------------------------------------------
    // CleanMmu: iterative replacement of the previously tail-recursive
    // implementation. The old code called CleanMmu(DriverName) at the
    // end of the Modified branch with NO termination guard; a second
    // match on the next pass would recurse again, risking stack overflow
    // on repeated loads. This version loops until a full pass finds no
    // more matches.
    // ------------------------------------------------------------------
    BOOL CleanMmu(UNICODE_STRING DriverName) {
        auto ps_loaded = GetPsLoaded();
        if (!ps_loaded)
            return 1;

        imports::ExAcquireResourceExclusiveLite(ps_loaded, TRUE);

        PMM_UNLOADED_DRIVER MmuBase = GetMmuAddress();
        if (!MmuBase) {
            imports::ExReleaseResourceLite(ps_loaded);
            return 1;
        }

        BOOLEAN AnyModified  = FALSE;
        BOOLEAN PassModified;

        do {
            PassModified = FALSE;
            BOOLEAN Filled = IsMmuFilled();

            for (ULONG Index = 0; Index < MM_UNLOADED_DRIVERS_SIZE; ++Index) {
                PMM_UNLOADED_DRIVER Entry = &MmuBase[Index];

                if (IsUnloadEmpty(Entry))
                    continue;

                if (PassModified) {
                    // we already removed one this pass; shift subsequent
                    // entries down by one to close the hole, then zero the tail
                    PMM_UNLOADED_DRIVER PrevEntry = &MmuBase[Index - 1];
                    RtlCopyMemory(PrevEntry, Entry, sizeof(MM_UNLOADED_DRIVER));
                    if (Index == MM_UNLOADED_DRIVERS_SIZE - 1)
                        RtlFillMemory(Entry, sizeof(MM_UNLOADED_DRIVER), 0);
                    continue;
                }

                if (!imports::RtlEqualUnicodeString(&DriverName, &Entry->Name, TRUE))
                    continue;

                PVOID BufferPool = Entry->Name.Buffer;
                RtlFillMemory(Entry, sizeof(MM_UNLOADED_DRIVER), 0);
                if (BufferPool)
                    imports::ExFreePoolWithTag(BufferPool, 'TDmM');

                if (ULONG* Mml = GetMmlAddress()) {
                    *Mml = (Filled ? MM_UNLOADED_DRIVERS_SIZE : *Mml) - 1;
                }

                PassModified = TRUE;
                AnyModified  = TRUE;
            }

            if (PassModified) {
                // fix up unload timestamps so the compacted array still
                // looks monotonic to any scanner that validates order
                ULONG64 PreviousTime = 0;
                for (LONG Index = MM_UNLOADED_DRIVERS_SIZE - 2; Index >= 0; --Index) {
                    PMM_UNLOADED_DRIVER Entry = &MmuBase[Index];
                    if (IsUnloadEmpty(Entry))
                        continue;
                    if (PreviousTime != 0 && Entry->UnloadTime > PreviousTime)
                        Entry->UnloadTime = PreviousTime - RandomNumber();
                    PreviousTime = Entry->UnloadTime;
                }
            }
        } while (PassModified);

        imports::ExReleaseResourceLite(ps_loaded);
        return AnyModified ? 0 : 1;
    }

    // ------------------------------------------------------------------
    // Pattern scan helpers used by LocatePiDDB (unchanged from original)
    // ------------------------------------------------------------------
    NTSTATUS BBSearchPattern(IN PCUCHAR pattern, IN UCHAR wildcard, IN ULONG_PTR len,
        IN const VOID* base, IN ULONG_PTR size, OUT PVOID* ppFound, int index = 0)
    {
        ASSERT(ppFound != NULL && pattern != NULL && base != NULL);
        if (ppFound == NULL || pattern == NULL || base == NULL)
            return STATUS_ACCESS_DENIED;
        int cIndex = 0;
        for (ULONG_PTR i = 0; i < size - len; i++) {
            BOOLEAN found = TRUE;
            for (ULONG_PTR j = 0; j < len; j++) {
                if (pattern[j] != wildcard && pattern[j] != ((PCUCHAR)base)[i + j]) {
                    found = FALSE;
                    break;
                }
            }
            if (found != FALSE && cIndex++ == index) {
                *ppFound = (PUCHAR)base + i;
                return STATUS_SUCCESS;
            }
        }
        return STATUS_NOT_FOUND;
    }

    PVOID g_KernelBase = NULL;
    ULONG g_KernelSize = 0;

    PVOID GetKernelBase(OUT PULONG pSize) {
        NTSTATUS status = STATUS_SUCCESS;
        ULONG bytes = 0;
        PRTL_PROCESS_MODULES pMods = NULL;
        PVOID checkPtr = NULL;
        UNICODE_STRING routineName;

        if (g_KernelBase != NULL) {
            if (pSize) *pSize = g_KernelSize;
            return g_KernelBase;
        }

        RtlUnicodeStringInit(&routineName, encrypt(L"NtOpenFile"));
        checkPtr = imports::MmGetSystemRoutineAddress(&routineName);
        if (checkPtr == NULL)
            return NULL;

        status = imports::ZwQuerySystemInformation(SystemModuleInformation, 0, bytes, &bytes);
        if (bytes == 0)
            return NULL;

        pMods = (PRTL_PROCESS_MODULES)imports::ExAllocatePoolWithTag(NonPagedPool, bytes, BB_POOL_TAG);
        if (!pMods)
            return NULL;
        crt::memset(pMods, 0, bytes);

        status = imports::ZwQuerySystemInformation(SystemModuleInformation, pMods, bytes, &bytes);
        if (NT_SUCCESS(status)) {
            PRTL_PROCESS_MODULE_INFORMATION pMod = pMods->Modules;
            for (ULONG i = 0; i < pMods->NumberOfModules; i++) {
                if (checkPtr >= pMod[i].ImageBase &&
                    checkPtr < (PVOID)((PUCHAR)pMod[i].ImageBase + pMod[i].ImageSize)) {
                    g_KernelBase = pMod[i].ImageBase;
                    g_KernelSize = pMod[i].ImageSize;
                    if (pSize) *pSize = g_KernelSize;
                    break;
                }
            }
        }

        imports::ExFreePoolWithTag(pMods, BB_POOL_TAG);
        return g_KernelBase;
    }

    NTSTATUS BBScanSection(IN PCCHAR section, IN PCUCHAR pattern, IN UCHAR wildcard,
        IN ULONG_PTR len, OUT PVOID* ppFound, PVOID base = nullptr)
    {
        if (ppFound == NULL)
            return STATUS_ACCESS_DENIED;
        if (base == nullptr)
            base = GetKernelBase(&g_KernelSize);
        if (base == nullptr)
            return STATUS_ACCESS_DENIED;

        PIMAGE_NT_HEADERS64 pHdr = imports::RtlImageNtHeader(base);
        if (!pHdr)
            return STATUS_ACCESS_DENIED;

        PIMAGE_SECTION_HEADER pFirstSection = (PIMAGE_SECTION_HEADER)(
            (uintptr_t)&pHdr->FileHeader +
            pHdr->FileHeader.SizeOfOptionalHeader +
            sizeof(IMAGE_FILE_HEADER));

        for (PIMAGE_SECTION_HEADER pSection = pFirstSection;
             pSection < pFirstSection + pHdr->FileHeader.NumberOfSections; pSection++)
        {
            ANSI_STRING s1, s2;
            imports::RtlInitAnsiString(&s1, section);
            imports::RtlInitAnsiString(&s2, (PCCHAR)pSection->Name);
            if (imports::RtlCompareString(&s1, &s2, TRUE) == 0) {
                PVOID ptr = NULL;
                NTSTATUS status = BBSearchPattern(pattern, wildcard, len,
                    (PUCHAR)base + pSection->VirtualAddress,
                    pSection->Misc.VirtualSize, &ptr);
                if (NT_SUCCESS(status)) {
                    *(PULONG64)ppFound = (ULONG_PTR)ptr;
                    return status;
                }
            }
        }
        return STATUS_ACCESS_DENIED;
    }

    extern "C" bool LocatePiDDB(PERESOURCE* lock, PRTL_AVL_TABLE* table) {
        PVOID PiDDBLockPtr = nullptr, PiDDBCacheTablePtr = nullptr;

        if (NT_SUCCESS(BBScanSection(encrypt("PAGE"), PiDDBLockPtr_sig_win10, 0,
            sizeof(PiDDBLockPtr_sig_win10) - 1,
            reinterpret_cast<PVOID*>(&PiDDBLockPtr))))
        {
            PiDDBLockPtr = PVOID((uintptr_t)PiDDBLockPtr + 28);
        }
        else {
            if (NT_SUCCESS(BBScanSection(encrypt("PAGE"), PiDDBLockPtr_sig_win11, 0,
                sizeof(PiDDBLockPtr_sig_win11) - 1,
                reinterpret_cast<PVOID*>(&PiDDBLockPtr))))
            {
                PiDDBLockPtr = PVOID((uintptr_t)PiDDBLockPtr + 16);
            }
            else {
                return false;
            }
        }

        if (!NT_SUCCESS(BBScanSection(encrypt("PAGE"), PiDDBCacheTablePtr_sig, 0,
            sizeof(PiDDBCacheTablePtr_sig) - 1,
            reinterpret_cast<PVOID*>(&PiDDBCacheTablePtr))))
        {
            return false;
        }

        PiDDBCacheTablePtr = PVOID((uintptr_t)PiDDBCacheTablePtr + 3);

        *lock  = (PERESOURCE)(ResolveRelativeAddress(PiDDBLockPtr, 3, 7));
        *table = (PRTL_AVL_TABLE)(ResolveRelativeAddress(PiDDBCacheTablePtr, 3, 7));
        return true;
    }

    // ------------------------------------------------------------------
    // clearCache: remove our driver's entry from PiDDBCacheTable
    // ------------------------------------------------------------------
    BOOL clearCache(UNICODE_STRING DriverName, ULONG timeDateStamp) {
        PERESOURCE PiDDBLock;
        PRTL_AVL_TABLE PiDDBCacheTable;
        if (!LocatePiDDB(&PiDDBLock, &PiDDBCacheTable))
            return 1;

        PiDDBCacheEntry lookupEntry = { };
        lookupEntry.DriverName    = DriverName;
        lookupEntry.TimeDateStamp = timeDateStamp;

        imports::ExAcquireResourceExclusiveLite(PiDDBLock, TRUE);
        auto pFoundEntry = (PiDDBCacheEntry*)imports::RtlLookupElementGenericTableAvl(
            PiDDBCacheTable, &lookupEntry);
        if (pFoundEntry == nullptr) {
            imports::ExReleaseResourceLite(PiDDBLock);
            return 1;
        }
        RemoveEntryList(&pFoundEntry->List);
        if (!imports::RtlDeleteElementGenericTableAvl(PiDDBCacheTable, pFoundEntry)) {
            imports::ExReleaseResourceLite(PiDDBLock);
            return 1;
        }
        imports::ExReleaseResourceLite(PiDDBLock);
        return 0;
    }

    // ------------------------------------------------------------------
    // clean_extras: unlink our driver from PsLoadedModuleList and
    // overwrite its LDR_DATA_TABLE_ENTRY with a kernel-module template
    // ------------------------------------------------------------------
    extern "C" LIST_ENTRY PsLoadedModuleList;

    typedef struct _LDR_DATA_TABLE_ENTRY {
        LIST_ENTRY InLoadOrderLinks;
        LIST_ENTRY InMemoryOrderLinks;
        LIST_ENTRY InInitializationOrderLinks;
        PVOID DllBase;
        PVOID EntryPoint;
        ULONG SizeOfImage;
        UNICODE_STRING FullDllName;
        UNICODE_STRING BaseDllName;
        ULONG Flags;
        USHORT LoadCount;
        USHORT TlsIndex;
        LIST_ENTRY HashLinks;
        PVOID SectionPointer;
        ULONG CheckSum;
        ULONG TimeDateStamp;
        PVOID LoadedImports;
        PVOID EntryPointActivationContext;
        PVOID PatchInformation;
        LIST_ENTRY ForwarderLinks;
        LIST_ENTRY ServiceTagLinks;
        LIST_ENTRY StaticLinks;
        PVOID ContextInformation;
        ULONG OriginalBase;
        LARGE_INTEGER LoadTime;
    } LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

    PLDR_DATA_TABLE_ENTRY kernelModule = NULL;

    void clean_extras(PDRIVER_OBJECT drv_obj) {
        if (!drv_obj)
            return;

        PLDR_DATA_TABLE_ENTRY entry = (PLDR_DATA_TABLE_ENTRY)drv_obj->DriverSection;
        if (!entry)
            return;

        // find a legit kernel-mode driver entry to steal metadata from.
        // the heuristic 'name starts with N' is kept from the original —
        // ntoskrnl.exe / ndis.sys / netio.sys all qualify.
        if (!kernelModule) {
            PLIST_ENTRY moduleList = entry->InLoadOrderLinks.Flink;
            PLDR_DATA_TABLE_ENTRY currentModule =
                CONTAINING_RECORD(moduleList, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

            while (moduleList != &entry->InLoadOrderLinks) {
                if (currentModule->BaseDllName.Buffer) {
                    WCHAR c = currentModule->BaseDllName.Buffer[0];
                    if (c == L'n' || c == L'N') {
                        kernelModule = currentModule;
                        break;
                    }
                }
                moduleList = moduleList->Flink;
                currentModule = CONTAINING_RECORD(moduleList, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
            }
        }

        // unlink from all three order lists
        RemoveEntryList(&entry->InLoadOrderLinks);
        RemoveEntryList(&entry->InMemoryOrderLinks);
        RemoveEntryList(&entry->InInitializationOrderLinks);
        InitializeListHead(&entry->InLoadOrderLinks);
        InitializeListHead(&entry->InMemoryOrderLinks);
        InitializeListHead(&entry->InInitializationOrderLinks);

        // zero the name buffers (survives dumps that walk DriverName)
        if (entry->FullDllName.Buffer) {
            crt::memset(entry->FullDllName.Buffer, 0, entry->FullDllName.Length);
            entry->FullDllName.Length        = 0;
            entry->FullDllName.MaximumLength = 0;
        }
        if (entry->BaseDllName.Buffer) {
            crt::memset(entry->BaseDllName.Buffer, 0, entry->BaseDllName.Length);
            entry->BaseDllName.Length        = 0;
            entry->BaseDllName.MaximumLength = 0;
        }

        entry->Flags         = 0;
        entry->LoadCount     = 0;
        entry->TlsIndex      = 0;
        entry->CheckSum      = 0;
        entry->TimeDateStamp = 0;
        entry->LoadedImports = NULL;
        entry->EntryPointActivationContext = NULL;
        entry->PatchInformation = NULL;
        entry->OriginalBase = 0;
        crt::memset(&entry->LoadTime, 0, sizeof(entry->LoadTime));

        RemoveEntryList(&entry->HashLinks);
        InitializeListHead(&entry->HashLinks);

#ifdef _HAS_EXTENDED_ENTRIES
        RemoveEntryList(&entry->ServiceTagLinks);
        RemoveEntryList(&entry->StaticLinks);
        RemoveEntryList(&entry->ForwarderLinks);
        InitializeListHead(&entry->ServiceTagLinks);
        InitializeListHead(&entry->StaticLinks);
        InitializeListHead(&entry->ForwarderLinks);
#endif

        // steal camouflage from the N-prefixed kernel driver we found
        if (kernelModule) {
            entry->Flags         = kernelModule->Flags;
            entry->CheckSum      = kernelModule->CheckSum;
            entry->TimeDateStamp = kernelModule->TimeDateStamp;
            entry->LoadedImports = kernelModule->LoadedImports;
            entry->LoadCount     = kernelModule->LoadCount;
            entry->EntryPointActivationContext = kernelModule->EntryPointActivationContext;
            entry->PatchInformation = kernelModule->PatchInformation;
            entry->OriginalBase  = kernelModule->OriginalBase;

            if (kernelModule->BaseDllName.Buffer &&
                kernelModule->BaseDllName.Length > 0 &&
                entry->BaseDllName.Buffer)
            {
                entry->BaseDllName.Length        = kernelModule->BaseDllName.Length;
                entry->BaseDllName.MaximumLength = kernelModule->BaseDllName.MaximumLength;
                RtlCopyMemory(entry->BaseDllName.Buffer,
                              kernelModule->BaseDllName.Buffer,
                              min(entry->BaseDllName.MaximumLength,
                                  kernelModule->BaseDllName.Length));
            }
        }
    }

}
