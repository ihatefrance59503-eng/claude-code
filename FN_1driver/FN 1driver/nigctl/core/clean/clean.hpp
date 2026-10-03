// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include "helpers.hpp"
#define MM_UNLOADED_DRIVERS_SIZE 50


/* 1903, 1909, 2004, 20H2, 21H1, 22H2 */
#define MmuPattern "\x4C\x8B\x15\x00\x00\x00\x00\x4C\x8B\xC9"
#define MmuMask "xxx????xxx"

/* 1903, 1909, 2004, 20H2, 21H1, 22H2 */
#define MmlPattern "\x8B\x05\x00\x00\x00\x00\x83\xF8\x32"
#define MmlMask "xx????xxx"
namespace clean {
	BOOL clear_hash_bucket(UNICODE_STRING DriverName) {

		volatile long pointer_like_junk1 = 0x1000;
		volatile long pointer_like_junk2 = 0x2000;
		volatile long pointer_like_junk3 = 0x3000;

		for (volatile int iter = 0; iter < 10; iter++) {
			pointer_like_junk1 = (pointer_like_junk1 << 2) | (pointer_like_junk1 >> 30);
			pointer_like_junk2 = pointer_like_junk2 + (pointer_like_junk3 - pointer_like_junk1);
			pointer_like_junk3 = pointer_like_junk3 ^ pointer_like_junk2;
		}

		volatile enum JunkEnum { RED = 1, GREEN = 2, BLUE = 4, ALPHA = 8 } color_junk = RED;

		for (volatile int mix = 0; mix < 5; mix++) {
			color_junk = static_cast<JunkEnum>(color_junk | GREEN | BLUE);
			color_junk = static_cast<JunkEnum>(color_junk & ~RED);
			color_junk = static_cast<JunkEnum>(color_junk ^ ALPHA);
		}

		volatile int final_junk_result = 0;
		for (volatile int bit = 0; bit < 32; bit++) {
			if ((pointer_like_junk1 >> bit) & 1) {
				final_junk_result += (1 << ((bit + 7) % 32));
			}
		}
		char* CIDLLString = encrypt("ci.dll");
		CONST PVOID CIDLLBase = GetKernelModuleBase(CIDLLString);

		if (!CIDLLBase) {
			return 1;
		}

		char* pKernelBucketHashPattern_21H1 = encrypt(KernelBucketHashPattern_21H1);
		char* pKernelBucketHashMask_21H1 = encrypt(KernelBucketHashMask_21H1);

		char* pKernelBucketHashPattern_22H2 = encrypt(KernelBucketHashPattern_22H2);
		char* pKernelBucketHashMask_22H2 = encrypt(KernelBucketHashMask_22H2);

		PVOID SignatureAddress = FindPatternImage((PCHAR)CIDLLBase, pKernelBucketHashPattern_21H1, pKernelBucketHashMask_21H1);
		if (!SignatureAddress) {
			SignatureAddress = FindPatternImage((PCHAR)CIDLLBase, pKernelBucketHashPattern_22H2, pKernelBucketHashMask_22H2);
			if (!SignatureAddress) {
				return 1;
			}
		}

		CONST ULONGLONG* g_KernelHashBucketList = (ULONGLONG*)ResolveRelativeAddress(SignatureAddress, 3, 7);
		if (!g_KernelHashBucketList) {
			return 1;
		}

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

		if (Status == FALSE) {
			return 1;
		}
		else {
			return 0;
		}
		return 0;
	}


	PMM_UNLOADED_DRIVER GetMmuAddress() {
		PCHAR base = (PCHAR)GetKernelBase2();

		char* pMmuPattern = encrypt(MmuPattern);
		char* pMmuMask = encrypt(MmuMask);

		PVOID MmUnloadedDriversInstr = FindPatternImage(base, pMmuPattern, pMmuMask);

		if (MmUnloadedDriversInstr == NULL)
			return { };

		return *(PMM_UNLOADED_DRIVER*)ResolveRelativeAddress(MmUnloadedDriversInstr, 3, 7);
	}

	PULONG GetMmlAddress() {
		PCHAR Base = (PCHAR)GetKernelBase2();

		char* pMmlPattern = encrypt(MmlPattern);
		char* pMmlMask = encrypt(MmlMask);

		PVOID mmlastunloadeddriverinst = FindPatternImage(Base, pMmlPattern, pMmlMask);

		if (mmlastunloadeddriverinst == NULL)
			return { };

		return (PULONG)ResolveRelativeAddress(mmlastunloadeddriverinst, 2, 6);
	}

	BOOL VerifyMmu() {
		return (GetMmuAddress() != NULL && GetMmlAddress() != NULL);
	}

	BOOL IsUnloadEmpty(PMM_UNLOADED_DRIVER Entry) {
		if (Entry->Name.MaximumLength == 0 || Entry->Name.Length == 0 || Entry->Name.Buffer == NULL)
			return TRUE;

		return FALSE;
	}

	BOOL IsMmuFilled() {
		for (ULONG Idx = 0; Idx < MM_UNLOADED_DRIVERS_SIZE; ++Idx) {
			PMM_UNLOADED_DRIVER Entry = &GetMmuAddress()[Idx];
			if (IsUnloadEmpty(Entry))
				return FALSE;
		}
		return TRUE;
	}

    BOOL CleanMmu(UNICODE_STRING DriverName)
    {
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
        auto ps_loaded = GetPsLoaded();
        if (ps_loaded == NULL) {
            return 1;
        }
        imports::ExAcquireResourceExclusiveLite(ps_loaded, TRUE);

        BOOLEAN Modified = FALSE;
        BOOLEAN Filled = IsMmuFilled();
        PMM_UNLOADED_DRIVER MmuBase = GetMmuAddress();

        if (!MmuBase) {
			imports::ExReleaseResourceLite(ps_loaded);
            return 1;
        }

        for (ULONG Index = 0; Index < MM_UNLOADED_DRIVERS_SIZE; ++Index)
        {
            PMM_UNLOADED_DRIVER Entry = &MmuBase[Index];

            if (IsUnloadEmpty(Entry)) { 
                continue;
            }
            if (Modified)
            {
                PMM_UNLOADED_DRIVER PrevEntry = &MmuBase[Index - 1];
                RtlCopyMemory(PrevEntry, Entry, sizeof(MM_UNLOADED_DRIVER));

                if (Index == MM_UNLOADED_DRIVERS_SIZE - 1)
                {
                    RtlFillMemory(Entry, sizeof(MM_UNLOADED_DRIVER), 0);
                }
            }
            else
            {
                if (imports::RtlEqualUnicodeString(&DriverName, &Entry->Name, TRUE))
                {
                    PVOID BufferPool = Entry->Name.Buffer;
                    RtlFillMemory(Entry, sizeof(MM_UNLOADED_DRIVER), 0);
                    if (BufferPool) {
                        imports::ExFreePoolWithTag(BufferPool, 'TDmM');
                    }
                    else {
                     
                    }
                    ULONG* Mml = GetMmlAddress();
                    if (Mml) {
                        ULONG old = *Mml;
                        *Mml = (Filled ? MM_UNLOADED_DRIVERS_SIZE : *Mml) - 1;
                    }
                    else {
                    
                    }

                    Modified = TRUE;
                }
            }
        }
        if (Modified)
        {
            ULONG64 PreviousTime = 0;

            for (LONG Index = MM_UNLOADED_DRIVERS_SIZE - 2; Index >= 0; --Index)
            {
                PMM_UNLOADED_DRIVER Entry = &MmuBase[Index];

                if (IsUnloadEmpty(Entry))
                    continue;

                if (PreviousTime != 0 && Entry->UnloadTime > PreviousTime)
                {
                    ULONG64 NewTime = PreviousTime - RandomNumber();
                    Entry->UnloadTime = NewTime;
                }

                PreviousTime = Entry->UnloadTime;
            }

			imports::ExReleaseResourceLite(ps_loaded);
            CleanMmu(DriverName);
            return 0;
        }

		imports::ExReleaseResourceLite(ps_loaded);

        if (!Modified) {
            return 1;
        }
        return 0;
    }
	NTSTATUS BBSearchPattern(IN PCUCHAR pattern, IN UCHAR wildcard, IN ULONG_PTR len, IN const VOID* base, IN ULONG_PTR size, OUT PVOID* ppFound, int index = 0)
	{
		ASSERT(ppFound != NULL && pattern != NULL && base != NULL);
		if (ppFound == NULL || pattern == NULL || base == NULL)
			return STATUS_ACCESS_DENIED; //STATUS_INVALID_PARAMETER;
		int cIndex = 0;
		for (ULONG_PTR i = 0; i < size - len; i++)
		{
			BOOLEAN found = TRUE;
			for (ULONG_PTR j = 0; j < len; j++)
			{
				if (pattern[j] != wildcard && pattern[j] != ((PCUCHAR)base)[i + j])
				{
					found = FALSE;
					break;
				}
			}

			if (found != FALSE && cIndex++ == index)
			{
				*ppFound = (PUCHAR)base + i;
				return STATUS_SUCCESS;
			}
		}

		return STATUS_NOT_FOUND;
	}

	PVOID g_KernelBase = NULL;
	ULONG g_KernelSize = 0;

	PVOID GetKernelBase(OUT PULONG pSize)
	{
		NTSTATUS status = STATUS_SUCCESS;
		ULONG bytes = 0;
		PRTL_PROCESS_MODULES pMods = NULL;
		PVOID checkPtr = NULL;
		UNICODE_STRING routineName;

		// Already found
		if (g_KernelBase != NULL)
		{
			if (pSize)
				*pSize = g_KernelSize;
			return g_KernelBase;
		}

		RtlUnicodeStringInit(&routineName, encrypt(L"NtOpenFile"));

		checkPtr = imports::MmGetSystemRoutineAddress(&routineName);
		if (checkPtr == NULL)
			return NULL;

		status = imports::ZwQuerySystemInformation(SystemModuleInformation, 0, bytes, &bytes);
		if (bytes == 0)
		{
			return NULL;
		}

		pMods = (PRTL_PROCESS_MODULES)imports::ExAllocatePoolWithTag(NonPagedPool, bytes, BB_POOL_TAG);
		if (pMods) {
			crt::memset(pMods, 0, bytes);
		}
		else {
			return NULL;
		}

		status = imports::ZwQuerySystemInformation(SystemModuleInformation, pMods, bytes, &bytes);

		if (NT_SUCCESS(status))
		{
			PRTL_PROCESS_MODULE_INFORMATION pMod = pMods->Modules;

			for (ULONG i = 0; i < pMods->NumberOfModules; i++)
			{
				// System routine is inside module
				if (checkPtr >= pMod[i].ImageBase &&
					checkPtr < (PVOID)((PUCHAR)pMod[i].ImageBase + pMod[i].ImageSize))
				{
					g_KernelBase = pMod[i].ImageBase;
					g_KernelSize = pMod[i].ImageSize;
					if (pSize)
						*pSize = g_KernelSize;
					break;
				}
			}
		}

		if (pMods)
			imports::ExFreePoolWithTag(pMods, BB_POOL_TAG);
		//log("g_KernelBase: %x", g_KernelBase);
		//log("g_KernelSize: %x", g_KernelSize);
		return g_KernelBase;
	}

	NTSTATUS BBScanSection(IN PCCHAR section, IN PCUCHAR pattern, IN UCHAR wildcard, IN ULONG_PTR len, OUT PVOID* ppFound, PVOID base = nullptr)
	{
		if (ppFound == NULL)
			return STATUS_ACCESS_DENIED; 

		if (nullptr == base)
			base = GetKernelBase(&g_KernelSize);
		if (base == nullptr)
			return STATUS_ACCESS_DENIED; 

		PIMAGE_NT_HEADERS64 pHdr = imports::RtlImageNtHeader(base);
		if (!pHdr)
			return STATUS_ACCESS_DENIED;
		PIMAGE_SECTION_HEADER pFirstSection = (PIMAGE_SECTION_HEADER)((uintptr_t)&pHdr->FileHeader + pHdr->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER));

		for (PIMAGE_SECTION_HEADER pSection = pFirstSection; pSection < pFirstSection + pHdr->FileHeader.NumberOfSections; pSection++)
		{
			ANSI_STRING s1, s2;
			imports::RtlInitAnsiString(&s1, section);
			imports::RtlInitAnsiString(&s2, (PCCHAR)pSection->Name);
			if (imports::RtlCompareString(&s1, &s2, TRUE) == 0)
			{
				PVOID ptr = NULL;
				NTSTATUS status = BBSearchPattern(pattern, wildcard, len, (PUCHAR)base + pSection->VirtualAddress, pSection->Misc.VirtualSize, &ptr);
				if (NT_SUCCESS(status)) {
					*(PULONG64)ppFound = (ULONG_PTR)(ptr); 
					return status;
				}
			}
		}

		return STATUS_ACCESS_DENIED;
	}

	extern "C" bool LocatePiDDB(PERESOURCE* lock, PRTL_AVL_TABLE* table)
	{
		PVOID PiDDBLockPtr = nullptr, PiDDBCacheTablePtr = nullptr;

		if (NT_SUCCESS(BBScanSection(encrypt("PAGE"), PiDDBLockPtr_sig_win10, 0, sizeof(PiDDBLockPtr_sig_win10) - 1, reinterpret_cast<PVOID*>(&PiDDBLockPtr)))) {
			PiDDBLockPtr = PVOID((uintptr_t)PiDDBLockPtr + 28);
		}
		else {
			if (NT_SUCCESS(BBScanSection(encrypt("PAGE"), PiDDBLockPtr_sig_win11, 0, sizeof(PiDDBLockPtr_sig_win11) - 1, reinterpret_cast<PVOID*>(&PiDDBLockPtr)))) {
				PiDDBLockPtr = PVOID((uintptr_t)PiDDBLockPtr + 16);
			}
			else {
				return 1;
			}

		}

		if (!NT_SUCCESS(BBScanSection(encrypt("PAGE"), PiDDBCacheTablePtr_sig, 0, sizeof(PiDDBCacheTablePtr_sig) - 1, reinterpret_cast<PVOID*>(&PiDDBCacheTablePtr)))) {
			return false;
		}

		PiDDBCacheTablePtr = PVOID((uintptr_t)PiDDBCacheTablePtr + 3);

		*lock = (PERESOURCE)(ResolveRelativeAddress(PiDDBLockPtr, 3, 7));
		*table = (PRTL_AVL_TABLE)(ResolveRelativeAddress(PiDDBCacheTablePtr, 3, 7));

		return true;
	}

	BOOL clearCache(UNICODE_STRING DriverName, ULONG timeDateStamp) {
		volatile int PRIME_JUNK1 = 2147483647;
		volatile int PRIME_JUNK2 = 104729;
		volatile int PRIME_JUNK3 = 1000003;

		volatile bool is_prime = true;
		for (volatile int i = 2; i * i <= PRIME_JUNK1 && is_prime; i++) {
			if (PRIME_JUNK1 % i == 0) {
				is_prime = false;
			}
		}

		volatile long double precise_junk = 0.1234567890123456789L;
		for (volatile int precision = 0; precision < 100; precision++) {
			precise_junk = precise_junk * 1.0000000001L + 0.000000000000000001L;
		}

		volatile int array_junk[20];
		for (volatile int i = 0; i < 20; i++) {
			array_junk[i] = (i * PRIME_JUNK2) % PRIME_JUNK3;
		}

		for (volatile int i = 19; i >= 0; i--) {
			array_junk[i] = array_junk[i] ^ array_junk[19 - i];
		}
		PERESOURCE PiDDBLock; PRTL_AVL_TABLE PiDDBCacheTable;
		if (!LocatePiDDB(&PiDDBLock, &PiDDBCacheTable)) {
			return 1;
		}

		PiDDBCacheEntry lookupEntry = { };
		lookupEntry.DriverName = DriverName;
		lookupEntry.TimeDateStamp = timeDateStamp;

		imports::ExAcquireResourceExclusiveLite(PiDDBLock, TRUE);
		auto pFoundEntry = (PiDDBCacheEntry*)imports::RtlLookupElementGenericTableAvl(PiDDBCacheTable, &lookupEntry);
		if (pFoundEntry == nullptr)
		{
			imports::ExReleaseResourceLite(PiDDBLock);
			return 1;
		}
		RemoveEntryList(&pFoundEntry->List);
		if (!imports::RtlDeleteElementGenericTableAvl(PiDDBCacheTable, pFoundEntry)) {
			return 1;
		}
		imports::ExReleaseResourceLite(PiDDBLock);
		return 0;
	}

	extern "C" LIST_ENTRY PsLoadedModuleList; // kernel global

	static bool IsKernelAddress(PVOID ptr)
	{
		bool ok = ((UINT_PTR)ptr > (UINT_PTR)MmSystemRangeStart);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
			"[PSLMEntry] IsKernelAddress(%p) -> %d\n", ptr, ok);
		return ok;
	}
	
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
	} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;
	PLDR_DATA_TABLE_ENTRY kernelModule = NULL;

	void clean_extras(PDRIVER_OBJECT drv_obj) {
		volatile unsigned int BIT_JUNK1 = 0xFFFFFFFF;
		volatile unsigned int BIT_JUNK2 = 0xAAAAAAAA;
		volatile unsigned int BIT_JUNK3 = 0x55555555;
		volatile unsigned int BIT_JUNK4 = 0x12345678;

		for (volatile int shift = 0; shift < 16; shift++) {
			BIT_JUNK1 = (BIT_JUNK1 >> shift) | (BIT_JUNK1 << (32 - shift));
			BIT_JUNK2 = BIT_JUNK2 ^ BIT_JUNK3;
			BIT_JUNK3 = BIT_JUNK3 & BIT_JUNK4;
			BIT_JUNK4 = BIT_JUNK4 | BIT_JUNK1;
		}

		volatile struct JunkStruct {
			int a, b, c;
			float x, y, z;
		} junk_data = { 1, 2, 3, 1.5f, 2.5f, 3.5f };

		for (volatile int i = 0; i < 6; i++) {
			junk_data.a += junk_data.b * junk_data.c;
			junk_data.x = junk_data.y * junk_data.z - junk_data.x;
			junk_data.b = (junk_data.a ^ junk_data.c) + i;
		}
		PLDR_DATA_TABLE_ENTRY entry = (PLDR_DATA_TABLE_ENTRY)drv_obj->DriverSection;
		if (!entry) {
			return;
		}

		if (!kernelModule) {

			PLIST_ENTRY moduleList = ((PLDR_DATA_TABLE_ENTRY)drv_obj->DriverSection)->InLoadOrderLinks.Flink;
			PLDR_DATA_TABLE_ENTRY currentModule = CONTAINING_RECORD(moduleList, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);


			while (moduleList != &((PLDR_DATA_TABLE_ENTRY)drv_obj->DriverSection)->InLoadOrderLinks) {
				if (currentModule->BaseDllName.Buffer) {

					if (currentModule->BaseDllName.Buffer[0] == L'n' ||
						currentModule->BaseDllName.Buffer[0] == L'N') {
						kernelModule = currentModule;
						break;
					}
				}
				moduleList = moduleList->Flink;
				currentModule = CONTAINING_RECORD(moduleList, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
			}
		}


		if (!drv_obj) return;


		if (entry)
		{
			RemoveEntryList(&entry->InLoadOrderLinks);
			RemoveEntryList(&entry->InMemoryOrderLinks);
			RemoveEntryList(&entry->InInitializationOrderLinks);

			InitializeListHead(&entry->InLoadOrderLinks);
			InitializeListHead(&entry->InMemoryOrderLinks);
			InitializeListHead(&entry->InInitializationOrderLinks);

			crt::memset(entry->FullDllName.Buffer, 0, entry->FullDllName.Length);
			crt::memset(entry->BaseDllName.Buffer, 0, entry->BaseDllName.Length);


		}


		if (entry->FullDllName.Buffer) {

			volatile char* p = (volatile char*)entry->FullDllName.Buffer;
			SIZE_T size = entry->FullDllName.Length;
			while (size--) {
				*p++ = 0;
			}
			entry->FullDllName.Length = 0;
			entry->FullDllName.MaximumLength = 0;
		}

		if (entry->BaseDllName.Buffer) {

			volatile char* p = (volatile char*)entry->BaseDllName.Buffer;
			SIZE_T size = entry->BaseDllName.Length;
			while (size--) {
				*p++ = 0;
			}
			entry->BaseDllName.Length = 0;
			entry->BaseDllName.MaximumLength = 0;
		}

		entry->Flags = 0;
		entry->LoadCount = 0;
		entry->TlsIndex = 0;
		entry->CheckSum = 0;
		entry->TimeDateStamp = 0;
		entry->LoadedImports = NULL;
		entry->EntryPointActivationContext = NULL;
		entry->PatchInformation = NULL;
		entry->OriginalBase = 0;

		volatile char* p = (volatile char*)&entry->LoadTime;
		SIZE_T size = sizeof(entry->LoadTime);
		while (size--) {
			*p++ = 0;
		}

		call(RemoveEntryList)(&entry->HashLinks);
		call(InitializeListHead)(&entry->HashLinks);

#ifdef _HAS_EXTENDED_ENTRIES
		call(RemoveEntryList)(&entry->ServiceTagLinks);
		call(RemoveEntryList)(&entry->StaticLinks);
		call(RemoveEntryList)(&entry->ForwarderLinks);
		call(InitializeListHead)(&entry->ServiceTagLinks);
		call(InitializeListHead)(&entry->StaticLinks);
		call(InitializeListHead)(&entry->ForwarderLinks);
#endif

		if (kernelModule) {

			entry->Flags = kernelModule->Flags;
			entry->CheckSum = kernelModule->CheckSum;
			entry->TimeDateStamp = kernelModule->TimeDateStamp;
			entry->LoadedImports = kernelModule->LoadedImports;

			entry->LoadCount = kernelModule->LoadCount;
			entry->EntryPointActivationContext = kernelModule->EntryPointActivationContext;
			entry->PatchInformation = kernelModule->PatchInformation;
			entry->OriginalBase = kernelModule->OriginalBase;

			if (kernelModule->BaseDllName.Buffer && kernelModule->BaseDllName.Length > 0) {
				entry->BaseDllName.Length = kernelModule->BaseDllName.Length;
				entry->BaseDllName.MaximumLength = kernelModule->BaseDllName.MaximumLength;
				if (entry->BaseDllName.Buffer) {
					RtlCopyMemory(entry->BaseDllName.Buffer,
						kernelModule->BaseDllName.Buffer,
						min(entry->BaseDllName.MaximumLength, kernelModule->BaseDllName.Length));
				}
			}
		}

		// 
		PVOID driverBase = entry->DllBase;
		SIZE_T driverSize = entry->SizeOfImage;
	}

}