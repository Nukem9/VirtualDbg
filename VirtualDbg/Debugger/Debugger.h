#pragma once

#include "../VM/Cpu.h"

struct DbgEventData
{
	PVIRT_CPU Cpu;
};

NTSTATUS DbgInit(ULONG ProcessId);
VOID DbgBeginWaitForEvent(DbgEventData **Data);
VOID DbgEndWaitForEvent();
NTSTATUS DbgSignalEvent(DbgEventData *Data);
VOID DbgInterceptContextSwap(ULONG_PTR CR3Value, PVIRT_CPU Cpu);
BOOLEAN DbgIsTargetProcess(ULONG_PTR CR3Value, ULONG_PTR EIPValue);