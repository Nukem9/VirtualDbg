#include "Driver.h"
#include "Debugger/Debugger.h"

#define VM_BEGIN_VIRTUALIZATION CTL_CODE(FILE_DEVICE_UNKNOWN, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define VM_STOP_VIRTUALIZATION	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define VM_QUERY_RUNNING		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define VM_DEBUG_PROCESS		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define VM_DEBUG_CONSUME		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x905, METHOD_BUFFERED, FILE_ANY_ACCESS)

NTSTATUS DispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	NTSTATUS status				= STATUS_SUCCESS;
	PIO_STACK_LOCATION ioStack	= IoGetCurrentIrpStackLocation(Irp);

	//
	// Check if MajorFunction was for DeviceIoControl
	//
	if (ioStack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
	{
		ULONG controlCode	= ioStack->Parameters.DeviceIoControl.IoControlCode;
		ULONG inputLength	= ioStack->Parameters.DeviceIoControl.InputBufferLength;
		ULONG outputLength	= ioStack->Parameters.DeviceIoControl.OutputBufferLength;

		switch (controlCode)
		{
		case VM_BEGIN_VIRTUALIZATION:
		{
			//
			// Start the virtual machine initialization thread
			//
			HANDLE threadHandle;
			status = PsCreateSystemThread(&threadHandle, THREAD_ALL_ACCESS, nullptr, nullptr, nullptr, VmStart, nullptr);

			if (NT_SUCCESS(status))
				ZwClose(threadHandle);
		}
		break;

		case VM_STOP_VIRTUALIZATION:
		{
			status = STATUS_NOT_IMPLEMENTED;
		}
		break;

		case VM_QUERY_RUNNING:
		{
			//
			// Output buffer size is expected to be a CHAR
			//
			if (outputLength != sizeof(CHAR))
			{
				DbgPrint("bad infolength 1\n");
				status = STATUS_INFO_LENGTH_MISMATCH;
				break;
			}

			*(CHAR *)Irp->AssociatedIrp.SystemBuffer	= VmIsActive();
			Irp->IoStatus.Information					= sizeof(CHAR);
		}
		break;

		case VM_DEBUG_PROCESS:
		{
			//
			// Input buffer is expected to be a DWORD
			// containing a process ID
			//
			if (inputLength != sizeof(ULONG))
			{
				DbgPrint("bad infolength 2\n");
				status = STATUS_INFO_LENGTH_MISMATCH;
				break;
			}

			status = DbgInit(*(ULONG *)Irp->AssociatedIrp.SystemBuffer);
		}
		break;

		case VM_DEBUG_CONSUME:
		{
			// Set a temporary timer value
			LARGE_INTEGER timeout;
			timeout.QuadPart = -10000;	// 10000ns = 1ms
			timeout.QuadPart *= 50;		// milliseconds

			// Event vars
			DbgEventData *data = nullptr;
			volatile BOOLEAN *completeStatus = nullptr;

			DbgPrint("wait for event\n");
			while (!DbgWaitForEvent(&data, &completeStatus))
			{
				// Allow the CPU to do other work while we wait
				KeDelayExecutionThread(UserMode, TRUE, &timeout);
			}
			
			DbgPrint("got an event\n");

			//data->Handled = true;
			DbgCompleteEvent(completeStatus);

			DbgPrint("exit ioctl\n");
		}
		break;

		}
	}

	//
	// Complete the request, but don't boost priority
	//
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}