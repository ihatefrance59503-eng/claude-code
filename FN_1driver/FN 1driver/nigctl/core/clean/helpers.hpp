// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include "../../helpers/string/crypt.hpp"
#include "../../helpers/imports/imports.hpp"
#include "../../helpers/crt/crt.hpp"



struct PiDDBCacheEntry
{
	LIST_ENTRY		List;
	UNICODE_STRING	DriverName;
	ULONG			TimeDateStamp;
	NTSTATUS		LoadStatus;
	char			_0x0028[16]; // data from the shim engine, or uninitialized memory for custom drivers
};


typedef struct _SYSTEM_MODULE {
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR  FullPathName[MAXIMUM_FILENAME_LENGTH];
} SYSTEM_MODULE, * PSYSTEM_MODULE;

typedef struct _SYSTEM_MODULE_INFORMATION {
	ULONG NumberOfModules;
	SYSTEM_MODULE Modules[1];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

#define MM_UNLOADED_DRIVERS_SIZE 50
typedef struct _MM_UNLOADED_DRIVER {
	UNICODE_STRING 	Name;
	PVOID 			ModuleStart;
	PVOID 			ModuleEnd;
	ULONG64 		UnloadTime;
} MM_UNLOADED_DRIVER, * PMM_UNLOADED_DRIVER;

/* 1903, 1909, 2004, 20H2, 21H1*/
#define KernelBucketHashPattern_21H1 "\x4C\x8D\x35\x00\x00\x00\x00\xE9\x00\x00\x00\x00\x8B\x84\x24"
#define KernelBucketHashMask_21H1 "xxx????x????xxx"

/* 22H2 */
#define KernelBucketHashPattern_22H2 "\x48\x8B\x1D\x00\x00\x00\x00\xEB\x00\xF7\x43\x40\x00\x20\x00\x00"
#define KernelBucketHashMask_22H2 "xxx????x?xxxxxxx"
#define BB_POOL_TAG 'Esk' 

	/* 1903, 1909, 2004, 20H2, 21H1*/
UCHAR PiDDBLockPtr_sig_win10[] = "\x8B\xD8\x85\xC0\x0F\x88\x00\x00\x00\x00\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x66\xFF\x88\x00\x00\x00\x00\xB2\x01\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x4C\x8B\x00\x24";

/* 22H2 */
UCHAR PiDDBLockPtr_sig_win11[] = "\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x0F\x85\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xE8";

/* 1903, 1909, 2004, 20H2, 21H1, 22H2 */
UCHAR PiDDBCacheTablePtr_sig[] = "\x66\x03\xD2\x48\x8D\x0D";

PVOID GetKernelBase2() {
	PVOID KernelBase = NULL;

	ULONG size = NULL;
	NTSTATUS status = imports::ZwQuerySystemInformation(SystemModuleInformation, 0, 0, &size);
	if (STATUS_INFO_LENGTH_MISMATCH != status) {
		return KernelBase;
	}
	
	PSYSTEM_MODULE_INFORMATION Modules = (PSYSTEM_MODULE_INFORMATION)imports::ExAllocatePool(NonPagedPool, size);
	if (!Modules) {
		return KernelBase;
	}

	if (!NT_SUCCESS(status = imports::ZwQuerySystemInformation(SystemModuleInformation, Modules, size, 0))) {
		imports::ExFreePoolWithTag(Modules, 0);
		return KernelBase;
	}

	if (Modules->NumberOfModules > 0) {
		KernelBase = Modules->Modules[0].ImageBase;
	}

	imports::ExFreePoolWithTag(Modules,0);
	return KernelBase;
}
PVOID ResolveRelativeAddress(_In_ PVOID Instruction,_In_ ULONG OffsetOffset,_In_ ULONG InstructionSize)
{
	ULONG_PTR Instr = (ULONG_PTR)Instruction;
	LONG RipOffset = *(PLONG)(Instr + OffsetOffset);
	PVOID ResolvedAddr = (PVOID)(Instr + InstructionSize + RipOffset);

	return ResolvedAddr;
}
ULONGLONG GetExportedFunction(CONST ULONGLONG mod,CONST CHAR* name) {
	const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(mod);
	const auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<ULONGLONG>(dos_header) + dos_header->e_lfanew);

	const auto data_directory = nt_headers->OptionalHeader.DataDirectory[0];
	const auto export_directory = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(mod + data_directory.VirtualAddress);

	const auto address_of_names = reinterpret_cast<ULONG*>(mod + export_directory->AddressOfNames);

	for (size_t i = 0; i < export_directory->NumberOfNames; i++)
	{
		const auto function_name = reinterpret_cast<const char*>(mod + address_of_names[i]);

		if (!crt::stricmp(function_name, name))
		{
			const auto name_ordinal = reinterpret_cast<unsigned short*>(mod + export_directory->AddressOfNameOrdinals)[i];

			const auto function_rva = mod + reinterpret_cast<ULONG*>(mod + export_directory->AddressOfFunctions)[name_ordinal];
			return function_rva;
		}
	}

	return 0;
}

PVOID GetKernelModuleBase(CHAR* ModuleName) {
	PVOID ModuleBase = NULL;

	ULONG size = NULL;
	NTSTATUS status = imports::ZwQuerySystemInformation(SystemModuleInformation, 0, 0, &size);
	if (STATUS_INFO_LENGTH_MISMATCH != status) {
		return ModuleBase;
	}

	PSYSTEM_MODULE_INFORMATION Modules = (PSYSTEM_MODULE_INFORMATION)imports::ExAllocatePool(NonPagedPool, size);
	if (!Modules) {
		return ModuleBase;
	}

	if (!NT_SUCCESS(status = imports::ZwQuerySystemInformation(SystemModuleInformation, Modules, size, 0))) {
		imports::ExFreePoolWithTag(Modules, 0);
		return ModuleBase;
	}

	for (UINT i = 0; i < Modules->NumberOfModules; i++) {
		CHAR* CurrentModuleName = reinterpret_cast<CHAR*>(Modules->Modules[i].FullPathName);
		if (crt::stristr(CurrentModuleName, ModuleName)) {
			ModuleBase = Modules->Modules[i].ImageBase;
			break;
		}
	}

	imports::ExFreePoolWithTag(Modules, 0);
	return ModuleBase;
}

BOOL CheckMask(PCHAR Base,PCHAR Pattern,PCHAR Mask) {
	for (; *Mask; ++Base, ++Pattern, ++Mask) {
		if (*Mask == 'x' && *Base != *Pattern) {
			return FALSE;
		}
	}
	return TRUE;
}

PVOID FindPattern2(PCHAR Base, DWORD Length, PCHAR Pattern, PCHAR Mask)
{
	if (!Base || !Pattern || !Mask) {
		return 0;
	}

	SIZE_T maskLen = crt::strlen(Mask);

	if (maskLen == 0 || Length < maskLen) {
	
		return 0;
	}

	DWORD ScanLength = Length - (DWORD)maskLen;

	for (DWORD i = 0; i <= ScanLength; i++) {

		if ((i % 0x1000) == 0) {
			
		}

		BOOL match = TRUE;

		for (SIZE_T j = 0; j < maskLen; j++) {

			if (Mask[j] != '?') {
				if (Base[i + j] != Pattern[j]) {
					match = FALSE;
					break;
				}
			}
		}

		if (match) {
			PVOID found = &Base[i];

			return found;
		}
	}
	return 0;
}


PVOID FindPatternImage(PCHAR Base, PCHAR Pattern, PCHAR Mask)
{
	if (!Base || !Pattern || !Mask) {
		return 0;
	}

	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)Base;

	if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
		return 0;
	}

	PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(Base + dos->e_lfanew);

	if (nt->Signature != IMAGE_NT_SIGNATURE) {
		return 0;
	}
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);

	for (DWORD i = 0; i < nt->FileHeader.NumberOfSections; i++, section++)
	{
		CHAR name[9]{};
		RtlCopyMemory(name, section->Name, 8);

		BOOL isText =(*(PINT)section->Name == 'EGAP') || crt::memcmp(section->Name, ".text", 5) == 0;
		if (!isText)
			continue;

		PCHAR secBase = Base + section->VirtualAddress;
		DWORD secSize = section->Misc.VirtualSize;
		PVOID match = FindPattern2(secBase, secSize, Pattern, Mask);

		if (match) {
			return match;
		}
		else {
		}
	}
	return 0;
}


PERESOURCE GetPsLoaded() {
	PCHAR base = (PCHAR)GetKernelBase2();
	auto cMmGetSystemRoutineAddress = reinterpret_cast<decltype(&MmGetSystemRoutineAddress)>(GetExportedFunction((ULONGLONG)base, encrypt("MmGetSystemRoutineAddress")));
	ERESOURCE PsLoadedModuleResource;
	UNICODE_STRING routineName = RTL_CONSTANT_STRING(L"PsLoadedModuleResource");
	auto cPsLoadedModuleResource = reinterpret_cast<decltype(&PsLoadedModuleResource)>(cMmGetSystemRoutineAddress(&routineName));

	return cPsLoadedModuleResource;
}

ULONG RandomNumberInRange(ULONG min, ULONG max)
{
	ULONG seed = (ULONG)KeQueryPerformanceCounter(NULL).QuadPart;
	ULONG rand = imports::RtlRandomEx(&seed);
	return (rand % (max - min + 1)) + min;
}

UCHAR RandomNumber() {
	PVOID Base = GetKernelBase2();

	auto cMmGetSystemRoutineAddress = reinterpret_cast<decltype(&MmGetSystemRoutineAddress)>(GetExportedFunction((ULONGLONG)Base, encrypt("MmGetSystemRoutineAddress")));

	UNICODE_STRING RoutineName = RTL_CONSTANT_STRING(L"RtlRandom");
	auto cRtlRandom = reinterpret_cast<decltype(&RtlRandom)>(cMmGetSystemRoutineAddress(&RoutineName));

	ULONG Seed = 1234765;
	ULONG Rand = cRtlRandom(&Seed) % 100;

	UCHAR RandInt = 0;

	if (Rand >= 101 || Rand <= -1)
		RandInt = 72;

	return (UCHAR)(Rand);
}	
