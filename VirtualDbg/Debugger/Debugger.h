#pragma once

#include "../VM/Cpu.h"

struct DbgEventData
{
	bool Handled;
	PVIRT_CPU Cpu;
};

NTSTATUS DbgInit(ULONG ProcessId);
NTSTATUS DbgClose();
BOOLEAN DbgWaitForEvent(DbgEventData **Data, volatile BOOLEAN **CompletionStatus);
VOID DbgCompleteEvent(volatile BOOLEAN *CompletionStatus);
NTSTATUS DbgSignalEvent(DbgEventData *Data);
VOID DbgInterceptContextSwap(ULONG_PTR CR3Value, PVIRT_CPU Cpu);
BOOLEAN DbgIsTargetProcess(ULONG_PTR CR3Value, ULONG_PTR EIPValue);