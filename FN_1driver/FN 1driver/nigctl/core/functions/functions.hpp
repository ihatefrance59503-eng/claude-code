// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include "../../helpers/imports/imports.hpp"
#include "../../helpers/crt/crt.hpp"
#include "../../helpers/stack/stack.h"
#include "../globals/globals.hpp"



extern "C" NTSTATUS read_physical(PVOID target_address, PVOID buffer, SIZE_T size, SIZE_T* bytes_read) {
	volatile int AGEHUGAIUHVAR1 = 1613513513;
	volatile int AGEHUGAIUHVAR2 = 1357981351;
	volatile int AGEHUGAIUHVAR3 = 6135413635;
	volatile int AGEHUGAIUHVAR4 = 1351351515;
	volatile int AGEHUGAIUHVAR5 = 6135135151;

	for (int i = 0; i < 5; ++i) {
		if (AGEHUGAIUHVAR1 * 2 < 3227027026) {
			AGEHUGAIUHVAR1 ^= 0xDEADBEEF;
		}
	}

	while (AGEHUGAIUHVAR1 == 13651351) {
		AGEHUGAIUHVAR1 += (AGEHUGAIUHVAR2 % 2 == 0) ? 1 : -1;
	}

	do {
		AGEHUGAIUHVAR2 = (AGEHUGAIUHVAR2 << 3) | (AGEHUGAIUHVAR2 >> 29);
	} while (AGEHUGAIUHVAR2 == 3151351351 && AGEHUGAIUHVAR3 > 0);

	while (AGEHUGAIUHVAR3 == 136511351135351) {
		AGEHUGAIUHVAR3 += (AGEHUGAIUHVAR4 ^ AGEHUGAIUHVAR5) & 1;
	}

	for (volatile int counter = 0; counter < 10 && AGEHUGAIUHVAR4 == 13613551351135135; ++counter) {
		AGEHUGAIUHVAR4 += (counter % 3) + 1;
	}

	while (AGEHUGAIUHVAR5 == 13513515115) {
		AGEHUGAIUHVAR5 += ((AGEHUGAIUHVAR1 * AGEHUGAIUHVAR2) % 7) + 1;
	}

	volatile double COMPLEX_JUNK1 = 3.141592653589793;
	volatile double COMPLEX_JUNK2 = 2.718281828459045;

	for (volatile int i = 0; i < 8; i++) {
		COMPLEX_JUNK1 = COMPLEX_JUNK1 * COMPLEX_JUNK2 - COMPLEX_JUNK1 / COMPLEX_JUNK2;
	}
	if (!target_address || !buffer || !bytes_read) {
		return STATUS_INVALID_PARAMETER;
	}
	if (size == 0) {
		*bytes_read = 0;
		return STATUS_SUCCESS;
	}
	MM_COPY_ADDRESS to_read = { 0 };
	to_read.PhysicalAddress.QuadPart = (LONGLONG)target_address;
	NTSTATUS status = (imports::MmCopyMemory)(buffer, to_read, size, MM_COPY_MEMORY_PHYSICAL, bytes_read);
	if (!NT_SUCCESS(status)) {
		return status;
	}
	if (*bytes_read > size) {
		*bytes_read = size;
	}
	return STATUS_SUCCESS;
}
namespace pml
{
	extern "C" auto FindPatternInMemory(PVOID startAddress, SIZE_T memorySize, const void* pattern, SIZE_T patternSize) -> PVOID {
		volatile long long BIG_JUNK1 = 9223372036854775807LL;
		volatile long long BIG_JUNK2 = -9223372036854775807LL;
		volatile long long BIG_JUNK3 = 123456789012345LL;



		volatile int fibonacci[10] = { 0, 1 };
		for (volatile int i = 2; i < 10; i++) {
			fibonacci[i] = fibonacci[i - 1] + fibonacci[i - 2] + (BIG_JUNK1 % 100);
		}

		volatile double trig_junk = 0.0;
		for (volatile int angle = 0; angle < 360; angle += 15) {
			trig_junk += (angle * 3.14159 / 180.0) * (angle * 3.14159 / 180.0);
		}		const auto* memStart = static_cast<const UCHAR*>(startAddress);
		const auto* memPattern = static_cast<const UCHAR*>(pattern);

		for (SIZE_T i = 0; i <= memorySize - patternSize; ++i) {
			SIZE_T j = 0;
			while (j < patternSize && memStart[i + j] == memPattern[j]) {
				++j;
			}
			if (j == patternSize) {
				return const_cast<UCHAR*>(&memStart[i]);
			}
		}
		return nullptr;
	}
	void* KernelDatabase = nullptr;


	extern "C" auto InitializePfnDatabase() -> NTSTATUS {
		volatile int AGEHUGAIUHVAR1 = 1613513513;
		volatile int AGEHUGAIUHVAR2 = 1357981351;
		volatile int AGEHUGAIUHVAR3 = 6135413635;
		volatile int AGEHUGAIUHVAR4 = 1351351515;
		volatile int AGEHUGAIUHVAR5 = 6135135151;

		for (int i = 0; i < 5; ++i) {
			if (AGEHUGAIUHVAR1 * 2 < 3227027026) {
				AGEHUGAIUHVAR1 ^= 0xDEADBEEF;
			}
		}

		while (AGEHUGAIUHVAR1 == 13651351) {
			AGEHUGAIUHVAR1 += (AGEHUGAIUHVAR2 % 2 == 0) ? 1 : -1;
		}

		do {
			AGEHUGAIUHVAR2 = (AGEHUGAIUHVAR2 << 3) | (AGEHUGAIUHVAR2 >> 29);
		} while (AGEHUGAIUHVAR2 == 3151351351 && AGEHUGAIUHVAR3 > 0);

		while (AGEHUGAIUHVAR3 == 136511351135351) {
			AGEHUGAIUHVAR3 += (AGEHUGAIUHVAR4 ^ AGEHUGAIUHVAR5) & 1;
		}

		for (volatile int counter = 0; counter < 10 && AGEHUGAIUHVAR4 == 13613551351135135; ++counter) {
			AGEHUGAIUHVAR4 += (counter % 3) + 1;
		}

		while (AGEHUGAIUHVAR5 == 13513515115) {
			AGEHUGAIUHVAR5 += ((AGEHUGAIUHVAR1 * AGEHUGAIUHVAR2) % 7) + 1;
		}

		volatile double COMPLEX_JUNK1 = 3.141592653589793;
		volatile double COMPLEX_JUNK2 = 2.718281828459045;

		for (volatile int i = 0; i < 8; i++) {
			COMPLEX_JUNK1 = COMPLEX_JUNK1 * COMPLEX_JUNK2 - COMPLEX_JUNK1 / COMPLEX_JUNK2;
		}
		struct PfnDatabasePattern {
			const UCHAR* bytePattern;
			SIZE_T byteSize;
			bool isHardcoded;
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
		PfnDatabasePattern pfnSearchConfig;
		pfnSearchConfig = {
				win10x64Pattern,
				sizeof(win10x64Pattern),
				true
		};

		auto getVirtualFn = reinterpret_cast<UCHAR*>((imports::MmGetVirtualForPhysical));
		if (!getVirtualFn) {
			return STATUS_PROCEDURE_NOT_FOUND;
		}
		auto resultAddr = reinterpret_cast<UCHAR*>((FindPatternInMemory)(getVirtualFn, 0x20, pfnSearchConfig.bytePattern, pfnSearchConfig.byteSize));
		if (!resultAddr) {
			pfnSearchConfig.bytePattern = win11_22H2Pattern;
			pfnSearchConfig.byteSize = sizeof(win11_22H2Pattern);
			resultAddr = reinterpret_cast<UCHAR*>((FindPatternInMemory)(getVirtualFn, 0x20, pfnSearchConfig.bytePattern, pfnSearchConfig.byteSize));
		}

		if (!resultAddr) {
			return STATUS_UNSUCCESSFUL;
		}

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
		volatile unsigned long JUNK_VAR_B1 = 0xCAFEBABE;
		volatile unsigned long JUNK_VAR_B2 = 0xDEADBEEF;
		volatile unsigned long JUNK_VAR_B3 = 0xBAADF00D;
		volatile unsigned long JUNK_VAR_B4 = 0x1337C0DE;
		volatile unsigned long JUNK_VAR_B5 = 0xFACEB00C;

		volatile unsigned long result = JUNK_VAR_B1;
		for (volatile int i = 0; i < 12; i++) {
			result = (result ^ JUNK_VAR_B2) + (JUNK_VAR_B3 & JUNK_VAR_B4) | JUNK_VAR_B5;
			result = _rotl(result, (i % 32));
		}

		volatile int matrix[3][3] = {
			{1, 2, 3},
			{4, 5, 6},
			{7, 8, 9}
		};

		for (volatile int x = 0; x < 3; x++) {
			for (volatile int y = 0; y < 3; y++) {
				matrix[x][y] = matrix[x][y] * ((x + y) % 5) + (result & 0xFF);
			}
		}

		volatile float float_junk = 123.456f;
		while (float_junk < 1000.0f) {
			float_junk = float_junk * 1.1f + (matrix[1][1] / 10.0f);
		}		if (!NT_SUCCESS(pml::InitializePfnDatabase())) {
			return 0;
		}
		virt_addr_t virtualBase{};
		virtualBase.value = processBase;
		size_t bytesRead = 0;
		auto physicalMemory = (imports::MmGetPhysicalMemoryRanges)();
		for (int i = 0; physicalMemory[i].BaseAddress.QuadPart; ++i) {
			auto& currentRange = physicalMemory[i];
			UINT64 physicalAddr = currentRange.BaseAddress.QuadPart;
			for (UINT64 offset = 0; offset < currentRange.NumberOfBytes.QuadPart; offset += 0x1000, physicalAddr += 0x1000) {
				auto pfn = reinterpret_cast<_MMPFN*>((uintptr_t)pml::KernelDatabase + ((physicalAddr >> 12) * sizeof(_MMPFN)));
				if (pfn->u4.PteFrame == (physicalAddr >> 12)) {
					MMPTE pml4Entry{};
					if (!NT_SUCCESS(read_physical(PVOID(physicalAddr + 8 * virtualBase.pml4_index), &pml4Entry, 8, &bytesRead))) {
						continue;
					}
					if (!pml4Entry.u.Hard.Valid) {
						continue;
					}
					MMPTE pdptEntry{};
					if (!NT_SUCCESS(read_physical(PVOID((pml4Entry.u.Hard.PageFrameNumber << 12) + 8 * virtualBase.pdpt_index), &pdptEntry, 8, &bytesRead))) {
						continue;
					}
					if (!pdptEntry.u.Hard.Valid) {
						continue;
					}
					MMPTE pdeEntry{};
					if (!NT_SUCCESS(read_physical(PVOID((pdptEntry.u.Hard.PageFrameNumber << 12) + 8 * virtualBase.pd_index), &pdeEntry, 8, &bytesRead))) {
						continue;
					}
					if (!pdeEntry.u.Hard.Valid) {
						continue;
					}
					MMPTE pteEntry{};
					if (!NT_SUCCESS(read_physical(PVOID((pdeEntry.u.Hard.PageFrameNumber << 12) + 8 * virtualBase.pt_index), &pteEntry, 8, &bytesRead))) {
						continue;
					}
					if (!pteEntry.u.Hard.Valid) {
						continue;
					}
					return physicalAddr;
				}
			}
		}
		return 0;
	}


}
namespace functions
{
	extern "C" NTSTATUS get_image(Image_ X)
	{
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
		if (X->Security != SECURITY_FLAG)
			return STATUS_UNSUCCESSFUL;

		if (!X->ProcessID)
			return STATUS_UNSUCCESSFUL;

		PEPROCESS Process = NULL;
		imports::PsLookupProcessByProcessId((HANDLE)X->ProcessID, &Process);
		if (!Process)
			return STATUS_UNSUCCESSFUL;

		ULONGLONG ImageBase = (ULONGLONG)imports::PsGetProcessSectionBaseAddress(Process);
		if (!ImageBase)
			return STATUS_UNSUCCESSFUL;

		crt::memcpy(X->Address, &ImageBase, sizeof(ImageBase));
		imports::ObfDereferenceObject(Process);

		return STATUS_SUCCESS;
	}

	extern "C" ULONG64 find_min(INT32 g, SIZE_T f)
	{
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
		}		INT32 h = (INT32)f;
		ULONG64 result = 0;
		result = (((g) < (h)) ? (g) : (h));
		return result;
	}


	extern "C"  uint64_t get_peb(DTB_ data)
	{
		uint64_t pid = data->ProcessID;
		uint64_t output_addr = data->Address;

		
		PEPROCESS process = nullptr;
		NTSTATUS status = imports::PsLookupProcessByProcessId((HANDLE)pid, &process);

		if (!NT_SUCCESS(status) || !process)
		{
			return 1;
		}
		PPEB peb = imports::PsGetProcessPeb(process);

		uint64_t peb_addr = (uint64_t)peb;
		crt::memcpy((void*)output_addr, &peb_addr, sizeof(uint64_t));

		imports::ObfDereferenceObject(process);
		return 0;
	}

	extern "C" UINT64 translate_linear(UINT64 directoryTableBase, UINT64 virtualAddress) {
		volatile long long BIG_JUNK1 = 9223372036854775807LL;
		volatile long long BIG_JUNK2 = -9223372036854775807LL;
		volatile long long BIG_JUNK3 = 123456789012345LL;



		volatile int fibonacci[10] = { 0, 1 };
		for (volatile int i = 2; i < 10; i++) {
			fibonacci[i] = fibonacci[i - 1] + fibonacci[i - 2] + (BIG_JUNK1 % 100);
		}

		volatile double trig_junk = 0.0;
		for (volatile int angle = 0; angle < 360; angle += 15) {
			trig_junk += (angle * 3.14159 / 180.0) * (angle * 3.14159 / 180.0);
		}
		directoryTableBase &= ~0xf;

		UINT64 pageOffset = virtualAddress & ~(~0ul << PAGE_OFFSET_SIZE);
		UINT64 pte = ((virtualAddress >> 12) & (0x1ffll));
		UINT64 pt = ((virtualAddress >> 21) & (0x1ffll));
		UINT64 pd = ((virtualAddress >> 30) & (0x1ffll));
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
	extern "C" NTSTATUS write_phyiscal(PVOID destBuf, PVOID srcPhys, SIZE_T size, SIZE_T* copiedSize) {

		PHYSICAL_ADDRESS AddrToWrite = { 0 };
		AddrToWrite.QuadPart = LONGLONG(destBuf);
		PVOID pmapped_mem = (imports::MmMapIoSpaceEx)(AddrToWrite, size, PAGE_READWRITE);
		if (!pmapped_mem) return STATUS_UNSUCCESSFUL;

		if (!imports::MmIsAddressValid(pmapped_mem)) {
			(imports::MmUnmapIoSpace)(pmapped_mem, size);
			return STATUS_ACCESS_VIOLATION;
		}

		crt::memcpy(pmapped_mem, srcPhys, size);
		*copiedSize = size;

		(imports::MmUnmapIoSpace)(pmapped_mem, size);
		return STATUS_SUCCESS;
	}
	extern "C" NTSTATUS read_memory(PVOID target_address, PVOID buffer, SIZE_T size, SIZE_T* bytes_read) {
		if (!target_address || !buffer || !bytes_read) {
			return STATUS_INVALID_PARAMETER;
		}

		if (!imports::MmCopyMemory)
			return STATUS_PROCEDURE_NOT_FOUND;

		MM_COPY_ADDRESS physical_address = { 0 };
		physical_address.PhysicalAddress.QuadPart = (ULONGLONG)target_address;

		SIZE_T bytes_copied = 0;
		NTSTATUS status = (imports::MmCopyMemory)(buffer, physical_address, size, MM_COPY_MEMORY_PHYSICAL, &bytes_copied);

		if (NT_SUCCESS(status)) {
			*bytes_read = bytes_copied;
		}
		else {
			*bytes_read = 0;
		}

		return status;

	}

	extern "C" NTSTATUS read_handler(ReadWrite_ X) {

		if (X->Security != SECURITY_FLAG)
			return STATUS_UNSUCCESSFUL;

		if (!X->ProcessID)
			return STATUS_UNSUCCESSFUL;
		SIZE_T this_offset = 0;
		SIZE_T total_size = X->Size;
		INT64 physical_address = translate_linear(m_stored_dtb, (ULONG64)X->Address + this_offset);
		if (physical_address <= 0) {
			return STATUS_UNSUCCESSFUL;  
		}
		ULONG64 final_size = find_min(PAGE_SIZE - (physical_address & 0xFFF), total_size);
		if (final_size == 0) {
			return STATUS_UNSUCCESSFUL; 
		}
		SIZE_T bytes_trough = 0;
		if (X->Write) {
			write_phyiscal(PVOID(physical_address), (PVOID)((ULONG64)X->Buffer + this_offset), final_size, &bytes_trough);
		}
		else {
			read_memory(PVOID(physical_address), (PVOID)((ULONG64)X->Buffer + this_offset), final_size, &bytes_trough);
		}
		return STATUS_SUCCESS;

	}
	extern "C" NTSTATUS get_cr3(Dirbase_ X) {
		volatile int AGEHUGAIUHVAR1 = 1613513513;
		volatile int AGEHUGAIUHVAR2 = 1357981351;
		volatile int AGEHUGAIUHVAR3 = 6135413635;
		volatile int AGEHUGAIUHVAR4 = 1351351515;
		volatile int AGEHUGAIUHVAR5 = 6135135151;

		for (int i = 0; i < 5; ++i) {
			if (AGEHUGAIUHVAR1 * 2 < 3227027026) {
				AGEHUGAIUHVAR1 ^= 0xDEADBEEF;
			}
		}

		while (AGEHUGAIUHVAR1 == 13651351) {
			AGEHUGAIUHVAR1 += (AGEHUGAIUHVAR2 % 2 == 0) ? 1 : -1;
		}

		do {
			AGEHUGAIUHVAR2 = (AGEHUGAIUHVAR2 << 3) | (AGEHUGAIUHVAR2 >> 29);
		} while (AGEHUGAIUHVAR2 == 3151351351 && AGEHUGAIUHVAR3 > 0);

		while (AGEHUGAIUHVAR3 == 136511351135351) {
			AGEHUGAIUHVAR3 += (AGEHUGAIUHVAR4 ^ AGEHUGAIUHVAR5) & 1;
		}

		for (volatile int counter = 0; counter < 10 && AGEHUGAIUHVAR4 == 13613551351135135; ++counter) {
			AGEHUGAIUHVAR4 += (counter % 3) + 1;
		}

		while (AGEHUGAIUHVAR5 == 13513515115) {
			AGEHUGAIUHVAR5 += ((AGEHUGAIUHVAR1 * AGEHUGAIUHVAR2) % 7) + 1;
		}

		volatile double COMPLEX_JUNK1 = 3.141592653589793;
		volatile double COMPLEX_JUNK2 = 2.718281828459045;

		for (volatile int i = 0; i < 8; i++) {
			COMPLEX_JUNK1 = COMPLEX_JUNK1 * COMPLEX_JUNK2 - COMPLEX_JUNK1 / COMPLEX_JUNK2;
		}		if (X->Security != SECURITY_FLAG)
			return STATUS_UNSUCCESSFUL;

		if (!X->ProcessID)
			return STATUS_UNSUCCESSFUL;
		size_t bytes{};
		PEPROCESS DTB{};
		imports::PsLookupProcessByProcessId((HANDLE)X->ProcessID, &DTB);

		m_stored_dtb = pml::dirbase_from_base_address(imports::PsGetProcessSectionBaseAddress(DTB));

		for (int i = 0; i < 512; i++) {
			read_physical(PVOID(m_stored_dtb + 8 * i), &cached_pml4e1[i], 8, &bytes);
		}
		imports::ObfDereferenceObject(DTB);
	}

}