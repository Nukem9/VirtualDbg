#include "Driver.h"

#define WINNT_DEVICE_NAME L"\\Device\\VirtualDbg"
#define MSDOS_DEVICE_NAME L"\\DosDevices\\VirtualDbg"

UNICODE_STRING usDriverName;
UNICODE_STRING usDosDeviceName;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	//
	// Initialize unicode driver device names
	//
	RtlInitUnicodeString(&usDriverName, WINNT_DEVICE_NAME);
	RtlInitUnicodeString(&usDosDeviceName, MSDOS_DEVICE_NAME);

	//
	// Create the I/O manager instance
	//
	PDEVICE_OBJECT deviceObject;
	NTSTATUS status = IoCreateDevice(DriverObject, 0, &usDriverName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_UNKNOWN, FALSE, &deviceObject);

	if (!NT_SUCCESS(status))
		return status;

	//
	// Symbolic link to DOS path
	//
	status = IoCreateSymbolicLink(&usDosDeviceName, &usDriverName);

	if (!NT_SUCCESS(status))
		return status;

	//
	// Set up major function pointers
	//
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = DispatchDefault;

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoControl;
	DriverObject->DriverUnload = DriverUnload;

	//
	// Init done
	//
	DriverObject->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	IoDeleteSymbolicLink(&usDosDeviceName);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS DispatchDefault(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}