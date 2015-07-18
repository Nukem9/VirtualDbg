#include "stdafx.h"

VOID DbgInterceptContextSwap(ULONG_PTR CR3Value, PVIRT_CPU Cpu)
{
	Cpu->DebuggerActive = DbgIsTargetProcess(CR3Value, Cpu->rip);
}

BOOLEAN DbgIsTargetProcess(ULONG_PTR CR3Value, ULONG_PTR EIPValue)
{
	// Returns TRUE if the CR3 register matches the processes' DirectoryBase
	// and if EIP is within usermode space
	UNREFERENCED_PARAMETER(CR3Value);
	UNREFERENCED_PARAMETER(EIPValue);

	return FALSE;
}

NTSTATUS DbgWaitForEvent()
{
	// Waits for a event that caused a VM exit, signaled by a
	// mutex
	return STATUS_SUCCESS;
}

NTSTATUS DbgSignalEvent(DbgEventData *Data)
{
	// Signals the mutex that an event has occurred
	UNREFERENCED_PARAMETER(Data);

	return STATUS_SUCCESS;
}