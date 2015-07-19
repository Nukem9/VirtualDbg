#include "stdafx.h"

// Debug event entry list
SINGLE_LIST_ENTRY EventListHeader;
KSPIN_LOCK EventListLock;

typedef struct
{
	SINGLE_LIST_ENTRY ListEntry;

	volatile BOOLEAN *CompletionStatus;
	DbgEventData *Data;
} EVENTDATA_CONTAINER, *PEVENTDATA_CONTAINER;

// Event signaling
KEVENT DbgConsumeEventSignal;
KEVENT DbgResponseEventSignal;

// Process parameters
PEPROCESS TargetProcess;
ULONG_PTR TargetUserCR3;	// Set value
ULONG_PTR TargetUserEIP;	// Maximum value

NTSTATUS DbgInit(ULONG ProcessId)
{
	// Singly linked list spin-lock for debug data events
	RtlSecureZeroMemory(&EventListHeader, sizeof(SINGLE_LIST_ENTRY));
	KeInitializeSpinLock(&EventListLock);

	// Initialize thread synchronization events
	KeInitializeEvent(&DbgConsumeEventSignal, NotificationEvent, FALSE);		// Do not auto-reset
	KeInitializeEvent(&DbgResponseEventSignal, SynchronizationEvent, FALSE);	// Do auto-reset

	// Query the process structure by ID
	NTSTATUS status = STATUS_SUCCESS;

	PEPROCESS localUserPrc = nullptr;
	ULONG_PTR localUserCR3 = 0;
	ULONG_PTR localUserEIP = (ULONG_PTR)MmHighestUserAddress;

	if (!NT_SUCCESS(status = PsLookupProcessByProcessId((HANDLE)ProcessId, &localUserPrc)))
		return status;

	// The system process isn't a valid target
	if (localUserPrc == PsInitialSystemProcess)
	{
		ObDereferenceObject(localUserPrc);
		return STATUS_INVALID_PARAMETER;
	}

	// EPROCESS::DirectoryBase is undocumented, but there is still an easy way to obtain it.
	// Read CR3 directly after switching to the process' address space.
	KAPC_STATE apcState;

	KeStackAttachProcess(localUserPrc, &apcState);
	localUserCR3 = __readcr3();
	KeUnstackDetachProcess(&apcState);

	// Validate
	if (!localUserCR3 || !localUserEIP)
		return STATUS_INVALID_PARAMETER;

	// Debug prints
	DbgPrint(
		"Initialized VirtualDBG with:\n\tTarget PID: 0x%X\n\tTarget Prc: %p\n\tTarget CR3: %p\n\tTarget EIP: %p\n",
		ProcessId,
		localUserPrc,
		localUserCR3,
		localUserEIP);

	InterlockedExchangePointer((volatile PVOID *)&TargetProcess, (PVOID)localUserPrc);
	InterlockedExchangePointer((volatile PVOID *)&TargetUserCR3, (PVOID)localUserCR3);
	InterlockedExchangePointer((volatile PVOID *)&TargetUserEIP, (PVOID)localUserEIP);
	return STATUS_SUCCESS;
}

NTSTATUS DbgClose()
{
	if (!TargetProcess)
		return STATUS_INVALID_DEVICE_STATE;

	// Dereference object and return
	ObDereferenceObject(TargetProcess);

	// Invalidate all pointers
	InterlockedExchangePointer((volatile PVOID *)&TargetProcess, nullptr);
	InterlockedExchangePointer((volatile PVOID *)&TargetUserCR3, nullptr);
	InterlockedExchangePointer((volatile PVOID *)&TargetUserEIP, nullptr);
	return STATUS_SUCCESS;
}

BOOLEAN DbgWaitForEvent(DbgEventData **Data, volatile BOOLEAN **CompletionStatus)
{
	// Barrier around event data being changed
	PSINGLE_LIST_ENTRY listEntry = ExInterlockedPopEntryList(&EventListHeader, &EventListLock);

	// Returned value will be null if there is nothing in the list
	if (!listEntry)
		return FALSE;

	// Capture internal buffer
	auto container = (PEVENTDATA_CONTAINER)CONTAINING_RECORD(listEntry, EVENTDATA_CONTAINER, ListEntry);

	*Data				= container->Data;
	*CompletionStatus	= container->CompletionStatus;

	// Free list entry
	ExFreePoolWithTag(container, 'DBGE');
	return TRUE;
}

VOID DbgCompleteEvent(volatile BOOLEAN *CompletionStatus)
{
	// There's no longer a debug event to be used.
	//
	// However, ExInterlockedPopEntryList already "removed"
	// the event from the list

	// Tell the VM thread to resume
	*CompletionStatus = TRUE;
}

NTSTATUS DbgSignalEvent(DbgEventData *Data)
{
	// Event completion signal variable set from the other thread
	volatile BOOLEAN eventCompleted = FALSE;

	// Using a stack variable across threads is OK because it will not
	// be touched until the event is signaled. It will not be paged due
	// to DISPATCH_LEVEL Irql.
	auto container = (PEVENTDATA_CONTAINER)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(EVENTDATA_CONTAINER), 'DBGE');

	container->CompletionStatus = &eventCompleted;
	container->Data				= Data;

	// Atomic list insert
	ExInterlockedPushEntryList(&EventListHeader, &container->ListEntry, &EventListLock);

	// Wait for the other thread to respond
	//
	// WARNING: This allows for context switches which
	// *might* cause undefined behavior or a BSOD. The registers are
	// OK, but the kernel stack isn't considered valid.
	//
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !!!!!! THIS CAUSES KERNEL CODE TO EXECUTE IN A NON-VM STATE !!!!!!
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	{
		KeLowerIrql(PASSIVE_LEVEL);

		// Spin
		while (eventCompleted == FALSE)
			YieldProcessor();

		KeRaiseIrqlToDpcLevel();
	}

	return STATUS_SUCCESS;
}

VOID DbgInterceptContextSwap(ULONG_PTR CR3Value, PVIRT_CPU Cpu)
{
	Cpu->DebuggerActive = DbgIsTargetProcess(CR3Value, Cpu->rip);
}

BOOLEAN DbgIsTargetProcess(ULONG_PTR CR3Value, ULONG_PTR EIPValue)
{
	// Returns TRUE if the CR3 register matches the processes' DirectoryBase
	// and if EIP is within user space
	return	CR3Value == TargetUserCR3 &&
			EIPValue <= TargetUserEIP;
}