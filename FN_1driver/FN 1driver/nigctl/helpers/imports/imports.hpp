// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
//\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\xFF\x15\x00\x00\x00\x00\x44\x39\x35\x00\x00\x00\x00\xBE xxx????xxxx?xx????xxx????x
#pragma once
#include "../structs/structs.hpp"
#include "../utilites/utilites.hpp"
#include "../string/crypt.hpp"


extern "C" PVOID NTAPI PsGetProcessSectionBaseAddress(PEPROCESS Process);
extern "C" PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader(IN PVOID ModuleAddress);
extern "C" NTSTATUS NTAPI ZwQuerySystemInformation(SYSTEM_INFORMATION_CLASS systemInformationClass, PVOID systemInformation, ULONG systemInformationLength, PULONG returnLength);
extern "C" NTSYSAPI NTSTATUS NTAPI ObReferenceObjectByName(_In_ PUNICODE_STRING ObjectName,_In_ ULONG Attributes,_In_opt_ PACCESS_STATE AccessState,_In_opt_ ACCESS_MASK DesiredAccess,_In_ POBJECT_TYPE ObjectType,_In_ KPROCESSOR_MODE AccessMode,_Inout_opt_ PVOID ParseContext,_Out_ PVOID* Object);
typedef NTSTATUS(*IofCallDriver_t)(PDEVICE_OBJECT DeviceObject, PIRP Irp);
typedef VOID(*IoDeleteDevice_t)(PDEVICE_OBJECT DeviceObject);
typedef NTSTATUS(*IoDeleteSymbolicLink_t)(PUNICODE_STRING SymbolicLinkName);
typedef NTSTATUS(*IoCreateDriver_t)(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE InitializationFunction);
typedef NTSTATUS(*IoCreateSymbolicLink_t)(PUNICODE_STRING SymbolicLinkName, PUNICODE_STRING DeviceName);
typedef NTSTATUS(*IoCreateDevice_t)(PDRIVER_OBJECT DriverObject, ULONG DeviceExtensionSize, PUNICODE_STRING DeviceName, DEVICE_TYPE DeviceType, ULONG DeviceCharacteristics, BOOLEAN Exclusive, PDEVICE_OBJECT* DeviceObject);
typedef PHYSICAL_MEMORY_RANGE* (*MmGetPhysicalMemoryRanges_t)();
typedef NTSTATUS(*MmCopyMemory_t)(PVOID Destination, MM_COPY_ADDRESS Source, SIZE_T Length, ULONG Flags, SIZE_T* BytesWritten);
typedef PVOID(*MmMapIoSpaceEx_t)(PHYSICAL_ADDRESS PhysicalAddress, SIZE_T NumberOfBytes, ULONG Protect);
typedef VOID(*MmUnmapIoSpace_t)(PVOID BaseAddress, SIZE_T NumberOfBytes);
typedef KIRQL(*KeAcquireSpinLockRaiseToDpc_t)(_Inout_ PKSPIN_LOCK SpinLock);
typedef VOID(*KeAcquireSpinLockAtDpcLevel_t)(_Inout_ PKSPIN_LOCK SpinLock);
typedef VOID(*KeReleaseSpinLockFromDpcLevel_t)(_Inout_ PKSPIN_LOCK SpinLock);
typedef KIRQL(*KeGetCurrentIrql_t)(VOID);
typedef VOID(*KeRaiseIrql_t)(KIRQL NewIrql, PKIRQL OldIrql);
typedef VOID(*KeLowerIrql_t)(KIRQL NewIrql);
typedef BOOLEAN(*MmIsAddressValid_t)(PVOID VirtualAddress);
typedef PVOID(*ExAllocatePool2_t)(POOL_FLAGS PoolType, SIZE_T NumberOfBytes, ULONG Tag);
typedef VOID(*ExFreePoolWithTag_t)(PVOID P, ULONG Tag);
typedef VOID(*ExFreePool_t)(PVOID P);

typedef LONG(*RtlCompareUnicodeString_t)(PUNICODE_STRING String1, PUNICODE_STRING String2, BOOLEAN CaseInSensitive);
typedef VOID(*KeInitializeSpinLock_t)(PKSPIN_LOCK SpinLock);
typedef VOID(*ObDereferenceObjectDeferDelete_t)(PVOID Object);
typedef BOOLEAN(*ObReferenceObjectSafe_t)(PVOID Object);
typedef VOID(*KeReleaseSpinLock_t)(PKSPIN_LOCK SpinLock, KIRQL NewIrql);
typedef NTSTATUS(*IoGetDeviceObjectPointer_t)(PUNICODE_STRING ObjectName, ACCESS_MASK DesiredAccess, PFILE_OBJECT* FileObject, PDEVICE_OBJECT* DeviceObject);
typedef KIRQL(*KfRaiseIrql_t)(KIRQL NewIrql);
typedef NTSTATUS(*PsLookupProcessByProcessId_t)(_In_ HANDLE ProcessId, _Outptr_ PEPROCESS* Process);
typedef NTSTATUS(*ZwQuerySystemInformation_t)(SYSTEM_INFORMATION_CLASS systemInformationClass, PVOID systemInformation, ULONG systemInformationLength, PULONG returnLength);
typedef PVOID(*PsGetProcessSectionBaseAddress_t)(PEPROCESS Process);
typedef PVOID(*ExAllocatePoolWithTag_t)(_In_ __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE PoolType,_In_ SIZE_T NumberOfBytes,_In_ ULONG Tag);
typedef PVOID(*ExAllocatePool_t)(__drv_strictTypeMatch(__drv_typeExpr) _In_ POOL_TYPE PoolType,_In_ SIZE_T NumberOfBytes);
typedef VOID(*IofCompleteRequest_t)(_In_ PIRP Irp, _In_ CCHAR PriorityBoost);
typedef PIO_STACK_LOCATION(*IoGetCurrentIrpStackLocation_t)(_In_ PIRP Irp);
typedef PVOID(*MmGetSystemRoutineAddress_t)(PUNICODE_STRING SystemRoutineName);
typedef VOID(*RtlZeroMemory_t)(VOID* Destination, SIZE_T Length);
typedef VOID(*ExReleaseResourceLite_t)(_Inout_ PERESOURCE Resource);
typedef VOID(*RtlInitAnsiString_t)(_Out_ PANSI_STRING DestinationString, _In_opt_z_ __drv_aliasesMem PCSZ SourceString);
typedef PIMAGE_NT_HEADERS(*RtlImageNtHeader_t)(IN PVOID ModuleAddress);

typedef VOID(*KeQuerySystemTimePrecise_t)(_Out_ PLARGE_INTEGER CurrentTime);
typedef ULONG(*RtlRandomEx_t)(_Inout_ PULONG Seed);
typedef BOOLEAN(*RtlEqualUnicodeString_t)(const UNICODE_STRING*, const UNICODE_STRING*, BOOLEAN);
typedef NTSTATUS(*RtlGetVersion_t)(PRTL_OSVERSIONINFOW);
typedef PIMAGE_NT_HEADERS(*RtlImageNtHeader_t)(PVOID);
typedef VOID(*KeQuerySystemTimePrecise_t)(PLARGE_INTEGER);
typedef VOID(*ExFreePoolWithTag_t)(PVOID, ULONG);
typedef NTSTATUS(*ExAcquireResourceExclusiveLite_t)(PERESOURCE, BOOLEAN);
typedef VOID(*ExReleaseResourceLite_t)(PERESOURCE);
typedef VOID(*ObfDereferenceObject_t)(PVOID);
typedef BOOLEAN(*RtlDeleteElementGenericTableAvl_t)(PVOID, PVOID);
typedef PVOID(*RtlLookupElementGenericTableAvl_t)(PVOID, PVOID);
typedef LONG(*RtlCompareString_t)(_In_ const STRING* String1, _In_ const STRING* String2, BOOLEAN CaseInSensitive);
typedef PVOID(*MmGetVirtualForPhysical_t)(PHYSICAL_ADDRESS);

typedef NTSTATUS(*ObReferenceObjectByName_t)(_In_ PUNICODE_STRING ObjectName,_In_ ULONG Attributes,_In_opt_ PACCESS_STATE AccessState,_In_opt_ ACCESS_MASK DesiredAccess,_In_ POBJECT_TYPE ObjectType,_In_ KPROCESSOR_MODE AccessMode,_Inout_opt_ PVOID ParseContext,_Out_ PVOID* Object);
typedef VOID(*RtlInitUnicodeString_t)(_Out_ PUNICODE_STRING DestinationString, _In_opt_z_ __drv_aliasesMem PCWSTR SourceString);
typedef VOID(*KeLowerIrql_t)(_In_ _Notliteral_ _IRQL_restores_ KIRQL NewIrql);
typedef KIRQL(*KfRaiseIrql_t)(_In_ KIRQL NewIrql);
typedef PPEB(*PsGetProcessPeb_t)(PEPROCESS Process);




namespace imports
{

	static IofCallDriver_t IofCallDriver = nullptr;
	static IoDeleteDevice_t IoDeleteDevice = nullptr;
	static IoDeleteSymbolicLink_t IoDeleteSymbolicLink = nullptr;
	static IoCreateDriver_t IoCreateDriver = nullptr;
	static IoCreateSymbolicLink_t IoCreateSymbolicLink = nullptr;
	static IoCreateDevice_t IoCreateDevice = nullptr;
	static MmGetPhysicalMemoryRanges_t MmGetPhysicalMemoryRanges = nullptr;
	static MmCopyMemory_t MmCopyMemory = nullptr;
	static MmMapIoSpaceEx_t MmMapIoSpaceEx = nullptr;
	static MmUnmapIoSpace_t MmUnmapIoSpace = nullptr;
	static MmIsAddressValid_t MmIsAddressValid = nullptr;
	static ExAllocatePool2_t ExAllocatePool2 = nullptr;
	static ExFreePoolWithTag_t ExFreePoolWithTag = nullptr;
	static ExFreePool_t ExFreePool = nullptr;
	static RtlCompareUnicodeString_t RtlCompareUnicodeString = nullptr;
	static ObDereferenceObjectDeferDelete_t ObDereferenceObjectDeferDelete = nullptr;
	static ObReferenceObjectSafe_t ObReferenceObjectSafe = nullptr;
	static KeReleaseSpinLock_t KeReleaseSpinLock = nullptr;
	static IoGetDeviceObjectPointer_t IoGetDeviceObjectPointer = nullptr;
	static ObfDereferenceObject_t ObfDereferenceObject = nullptr;
	static PsLookupProcessByProcessId_t PsLookupProcessByProcessId = nullptr;
	static ZwQuerySystemInformation_t ZwQuerySystemInformation = nullptr;
	static PsGetProcessSectionBaseAddress_t PsGetProcessSectionBaseAddress = nullptr;
	static ExAllocatePoolWithTag_t ExAllocatePoolWithTag = nullptr;
	static ExAllocatePool_t ExAllocatePool = nullptr;
	static IoGetCurrentIrpStackLocation_t IoGetCurrentIrpStackLocation = nullptr;
	static MmGetSystemRoutineAddress_t MmGetSystemRoutineAddress = nullptr;
	static RtlInitAnsiString_t RtlInitAnsiString = nullptr;
	static RtlEqualUnicodeString_t RtlEqualUnicodeString = nullptr;
	static RtlGetVersion_t RtlGetVersion = nullptr;
	static RtlImageNtHeader_t RtlImageNtHeader = nullptr;
	static KeQuerySystemTimePrecise_t KeQuerySystemTimePrecise = nullptr;
	static ExAcquireResourceExclusiveLite_t ExAcquireResourceExclusiveLite = nullptr;
	static ExReleaseResourceLite_t ExReleaseResourceLite = nullptr;
	static RtlDeleteElementGenericTableAvl_t RtlDeleteElementGenericTableAvl = nullptr;
	static RtlLookupElementGenericTableAvl_t RtlLookupElementGenericTableAvl = nullptr;
	static RtlCompareString_t RtlCompareString = nullptr;
	static MmGetVirtualForPhysical_t MmGetVirtualForPhysical = nullptr;
	static RtlRandomEx_t RtlRandomEx = nullptr;
	static IofCompleteRequest_t IofCompleteRequest = nullptr;
	static ObReferenceObjectByName_t ObReferenceObjectByName = nullptr;
	static RtlInitUnicodeString_t RtlInitUnicodeString_f = nullptr;
	static KeLowerIrql_t KeLowerIrql = nullptr;
	static KfRaiseIrql_t KfRaiseIrql = nullptr;
	static PsGetProcessPeb_t PsGetProcessPeb = nullptr;

	extern "C" void* get_kernel_export(LPWSTR FunctionName)
	{
		utilites::junk_func_c();
		struct {
			ULONG_PTR padding1;
			USHORT length;
			USHORT maxLength;
			PWSTR buffer;
			ULONG_PTR padding2;
		} customStack;

		UNICODE_STRING* _x = (UNICODE_STRING*)((char*)&customStack + sizeof(ULONG_PTR));

		customStack.padding1 = (ULONG_PTR)FunctionName;
		customStack.padding2 = (ULONG_PTR)_x;

		struct {
			ULONG_PTR shadowSpace[4];
			UNICODE_STRING* param1;
			PCWSTR param2;
		} rtlCallFrame;
		rtlCallFrame.shadowSpace[0] = 0;
		rtlCallFrame.shadowSpace[1] = 0;
		rtlCallFrame.shadowSpace[2] = 0;
		rtlCallFrame.shadowSpace[3] = 0;
		rtlCallFrame.param1 = _x;
		rtlCallFrame.param2 = FunctionName;
		utilites::junk_func_g();

		call(RtlInitUnicodeString)(rtlCallFrame.param1, rtlCallFrame.param2);

		void* mmGetAddr = call(MmGetSystemRoutineAddress);
		utilites::junk_func_a();

		if (_x->Buffer) {
			struct {
				ULONG_PTR shadowSpace[4];
				UNICODE_STRING* param1;
			} mmCallFrame;
			mmCallFrame.shadowSpace[0] = 0;
			mmCallFrame.shadowSpace[1] = 0;
			mmCallFrame.shadowSpace[2] = 0;
			mmCallFrame.shadowSpace[3] = 0;
			mmCallFrame.param1 = _x;

			void* result = ((void* (*)(UNICODE_STRING*))(mmGetAddr))(mmCallFrame.param1);
			return result;
		}
		return nullptr;
	}

	inline void resolve_imports()
	{
		utilites::junk_func_b();

		LPWSTR name;

		name = encrypt(L"IofCallDriver");
		IofCallDriver = reinterpret_cast<IofCallDriver_t>(get_kernel_export(name));

		name = encrypt(L"IoDeleteDevice");
		IoDeleteDevice = reinterpret_cast<IoDeleteDevice_t>(get_kernel_export(name));

		name = encrypt(L"IoDeleteSymbolicLink");
		IoDeleteSymbolicLink = reinterpret_cast<IoDeleteSymbolicLink_t>(get_kernel_export(name));

		name = encrypt(L"IoCreateDriver");
		IoCreateDriver = reinterpret_cast<IoCreateDriver_t>(get_kernel_export(name));
		utilites::junk_func_g();

		name = encrypt(L"IoCreateSymbolicLink");
		IoCreateSymbolicLink = reinterpret_cast<IoCreateSymbolicLink_t>(get_kernel_export(name));

		name = encrypt(L"IoCreateDevice");
		IoCreateDevice = reinterpret_cast<IoCreateDevice_t>(get_kernel_export(name));

		name = encrypt(L"MmGetPhysicalMemoryRanges");
		MmGetPhysicalMemoryRanges = reinterpret_cast<MmGetPhysicalMemoryRanges_t>(get_kernel_export(name));

		name = encrypt(L"MmCopyMemory");
		MmCopyMemory = reinterpret_cast<MmCopyMemory_t>(get_kernel_export(name));

		name = encrypt(L"MmMapIoSpaceEx");
		MmMapIoSpaceEx = reinterpret_cast<MmMapIoSpaceEx_t>(get_kernel_export(name));

		name = encrypt(L"MmUnmapIoSpace");
		MmUnmapIoSpace = reinterpret_cast<MmUnmapIoSpace_t>(get_kernel_export(name));

		name = encrypt(L"MmIsAddressValid");
		MmIsAddressValid = reinterpret_cast<MmIsAddressValid_t>(get_kernel_export(name));

		name = encrypt(L"ExAllocatePool2");
		ExAllocatePool2 = reinterpret_cast<ExAllocatePool2_t>(get_kernel_export(name));

		name = encrypt(L"ExFreePoolWithTag");
		ExFreePoolWithTag = reinterpret_cast<ExFreePoolWithTag_t>(get_kernel_export(name));

		name = encrypt(L"ExFreePool");
		ExFreePool = reinterpret_cast<ExFreePool_t>(get_kernel_export(name));

		name = encrypt(L"RtlCompareUnicodeString");
		RtlCompareUnicodeString = reinterpret_cast<RtlCompareUnicodeString_t>(get_kernel_export(name));

		name = encrypt(L"ObDereferenceObjectDeferDelete");
		ObDereferenceObjectDeferDelete = reinterpret_cast<ObDereferenceObjectDeferDelete_t>(get_kernel_export(name));

		name = encrypt(L"ObReferenceObjectSafe");
		ObReferenceObjectSafe = reinterpret_cast<ObReferenceObjectSafe_t>(get_kernel_export(name));

		name = encrypt(L"KeReleaseSpinLock");
		KeReleaseSpinLock = reinterpret_cast<KeReleaseSpinLock_t>(get_kernel_export(name));
		utilites::junk_func_d();

		name = encrypt(L"IoGetDeviceObjectPointer");
		IoGetDeviceObjectPointer = reinterpret_cast<IoGetDeviceObjectPointer_t>(get_kernel_export(name));

		name = encrypt(L"ObfDereferenceObject");
		ObfDereferenceObject = reinterpret_cast<ObfDereferenceObject_t>(get_kernel_export(name));

		name = encrypt(L"PsLookupProcessByProcessId");
		PsLookupProcessByProcessId = reinterpret_cast<PsLookupProcessByProcessId_t>(get_kernel_export(name));

		name = encrypt(L"ZwQuerySystemInformation");
		ZwQuerySystemInformation = reinterpret_cast<ZwQuerySystemInformation_t>(get_kernel_export(name));
		
		name = encrypt(L"PsGetProcessSectionBaseAddress");
		PsGetProcessSectionBaseAddress = reinterpret_cast<PsGetProcessSectionBaseAddress_t>(get_kernel_export(name));
		
		name = encrypt(L"ExAllocatePoolWithTag");
		ExAllocatePoolWithTag = reinterpret_cast<ExAllocatePoolWithTag_t>(get_kernel_export(name));
		
		name = encrypt(L"RtlInitUnicodeString");
		RtlInitUnicodeString_f = reinterpret_cast<RtlInitUnicodeString_t>(get_kernel_export(name));

		name = encrypt(L"ExAllocatePool");
		ExAllocatePool = reinterpret_cast<ExAllocatePool_t>(get_kernel_export(name));
		
		name = encrypt(L"IofCompleteRequest");
		IofCompleteRequest = reinterpret_cast<IofCompleteRequest_t>(get_kernel_export(name));

		name = encrypt(L"IoGetCurrentIrpStackLocation");
		IoGetCurrentIrpStackLocation = reinterpret_cast<IoGetCurrentIrpStackLocation_t>(get_kernel_export(name));

		name = encrypt(L"MmGetSystemRoutineAddress");
		MmGetSystemRoutineAddress = reinterpret_cast<MmGetSystemRoutineAddress_t>(get_kernel_export(name));
		
		name = encrypt(L"ExReleaseResourceLite");
		ExReleaseResourceLite = reinterpret_cast<ExReleaseResourceLite_t>(get_kernel_export(name));
		
		name = encrypt(L"RtlInitAnsiString");
		RtlInitAnsiString = reinterpret_cast<RtlInitAnsiString_t>(get_kernel_export(name));
		
		name = encrypt(L"RtlImageNtHeader");
		RtlImageNtHeader = reinterpret_cast<RtlImageNtHeader_t>(get_kernel_export(name));
		
		name = encrypt(L"KeQuerySystemTimePrecise");
		KeQuerySystemTimePrecise = reinterpret_cast<KeQuerySystemTimePrecise_t>(get_kernel_export(name));

		name = encrypt(L"RtlEqualUnicodeString");
		RtlEqualUnicodeString = reinterpret_cast<RtlEqualUnicodeString_t>(get_kernel_export(name));

		name = encrypt(L"RtlGetVersion");
		RtlGetVersion = reinterpret_cast<RtlGetVersion_t>(get_kernel_export(name));

		name = encrypt(L"RtlImageNtHeader");
		RtlImageNtHeader = reinterpret_cast<RtlImageNtHeader_t>(get_kernel_export(name));

		name = encrypt(L"KeQuerySystemTimePrecise");
		KeQuerySystemTimePrecise = reinterpret_cast<KeQuerySystemTimePrecise_t>(get_kernel_export(name));

		name = encrypt(L"ExFreePoolWithTag");
		ExFreePoolWithTag = reinterpret_cast<ExFreePoolWithTag_t>(get_kernel_export(name));

		name = encrypt(L"ExAcquireResourceExclusiveLite");
		ExAcquireResourceExclusiveLite = reinterpret_cast<ExAcquireResourceExclusiveLite_t>(get_kernel_export(name));

		name = encrypt(L"ExReleaseResourceLite");
		ExReleaseResourceLite = reinterpret_cast<ExReleaseResourceLite_t>(get_kernel_export(name));

		name = encrypt(L"ObfDereferenceObject");
		ObfDereferenceObject = reinterpret_cast<ObfDereferenceObject_t>(get_kernel_export(name));

		name = encrypt(L"RtlDeleteElementGenericTableAvl");
		RtlDeleteElementGenericTableAvl = reinterpret_cast<RtlDeleteElementGenericTableAvl_t>(get_kernel_export(name));

		name = encrypt(L"RtlLookupElementGenericTableAvl");
		RtlLookupElementGenericTableAvl = reinterpret_cast<RtlLookupElementGenericTableAvl_t>(get_kernel_export(name));

		name = encrypt(L"RtlCompareString");
		RtlCompareString = reinterpret_cast<RtlCompareString_t>(get_kernel_export(name));

		name = encrypt(L"MmGetVirtualForPhysical");
		MmGetVirtualForPhysical = reinterpret_cast<MmGetVirtualForPhysical_t>(get_kernel_export(name));

		name = encrypt(L"RtlRandomEx");
		RtlRandomEx = reinterpret_cast<RtlRandomEx_t>(get_kernel_export(name));

		name = encrypt(L"IofCompleteRequest");
		IofCompleteRequest = reinterpret_cast<IofCompleteRequest_t>(get_kernel_export(name));

		name = encrypt(L"ObReferenceObjectByName");
		ObReferenceObjectByName = reinterpret_cast<ObReferenceObjectByName_t>(get_kernel_export(name));

		name = encrypt(L"KeLowerIrql");
		KeLowerIrql = reinterpret_cast<KeLowerIrql_t>(get_kernel_export(name));

		name = encrypt(L"KfRaiseIrql");
		KfRaiseIrql = reinterpret_cast<KfRaiseIrql_t>(get_kernel_export(name));

		name = encrypt(L"PsGetProcessPeb");
		PsGetProcessPeb = reinterpret_cast<PsGetProcessPeb_t>(get_kernel_export(name));
	}

}