#pragma once

#include "../VM/Cpu.h"

struct DbgEventData
{
	PVIRT_CPU Cpu;
};

VOID DbgInterceptContextSwap(ULONG_PTR CR3Value, PVIRT_CPU Cpu);
BOOLEAN DbgIsTargetProcess(ULONG_PTR CR3Value, ULONG_PTR EIPValue);
NTSTATUS DbgWaitForEvent();
NTSTATUS DbgSignalEvent(DbgEventData *Data);