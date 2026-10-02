#include "../Includes.h"

#define ExAllocatePoolnew(a, b) NT.ExAllocatePoolWithTag(a, b, 'WinD')

uint64_t g_original_function = 0;
BYTE g_original_bytes[64] = { 0 };
PDEVICE_OBJECT g_DeviceObject = nullptr;
UNICODE_STRING g_SymbolicLink = { 0 };

extern "C" NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName, OPTIONAL PDRIVER_INITIALIZE InitializationFunction);

#include "../Modules/Spoofer/Spoofer.h"

VOID CleanupDriver()
{
	Spoofer::Cleanup();
	EACHooks::UninstallHooks();
	if (g_SymbolicLink.Buffer != nullptr) {
		IoDeleteSymbolicLink(&g_SymbolicLink);
		RtlFreeUnicodeString(&g_SymbolicLink);
		g_SymbolicLink.Buffer = nullptr;
	}

	if (g_DeviceObject != nullptr) {
		IoDeleteDevice(g_DeviceObject);
		g_DeviceObject = nullptr;
	}
}

VOID DummyUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	CleanupDriver();
}

NTSTATUS DeviceDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	return IoControl::Dispatch(DeviceObject, Irp);
}

NTSTATUS DriverInitialize(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = DummyUnload;

	for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = DeviceDispatch;
	}

	UNICODE_STRING deviceName;
	RtlInitUnicodeString(&deviceName, L"\\Device\\WinDrvMgr");

	NTSTATUS status = IoCreateDevice(
		DriverObject,
		0,
		&deviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&g_DeviceObject
	);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	g_DeviceObject->Flags |= DO_BUFFERED_IO;
	g_DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	UNICODE_STRING symbolicLink;
	RtlInitUnicodeString(&symbolicLink, L"\\??\\WinDrvMgr");

	IoDeleteSymbolicLink(&symbolicLink);
	status = IoCreateSymbolicLink(&symbolicLink, &deviceName);

	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(g_DeviceObject);
		return status;
	}

	WCHAR* symbolicLinkCopy = (WCHAR*)ExAllocatePoolnew(NonPagedPool, (wcslen(L"\\??\\WinDrvMgr") + 1) * sizeof(WCHAR));
	if (symbolicLinkCopy) {
		wcscpy(symbolicLinkCopy, L"\\??\\WinDrvMgr");
		RtlInitUnicodeString(&g_SymbolicLink, symbolicLinkCopy);
	}

	Spoofer::Initialize();
	EACHooks::setup();

	return STATUS_SUCCESS;
}NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {

	if (!Resolver.SetupASM())
	{
		return STATUS_UNSUCCESSFUL;
	}

	Utility::InitRandom();
	mousemove::InitializeMouseControl();

	if (DriverObject != nullptr) {
		// Loaded normally via sc start / service manager
		return DriverInitialize(DriverObject, RegistryPath);
	}

	// Manual mapping fallback
	UNICODE_STRING driverNameU;
	RtlInitUnicodeString(&driverNameU, L"\\Driver\\WinDrvMgr");

	NTSTATUS status = IoCreateDriver(&driverNameU, DriverInitialize);

	if (!NT_SUCCESS(status)) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	return STATUS_SUCCESS;
}
