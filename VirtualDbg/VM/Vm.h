#pragma once

VOID VmStart(PVOID StartContext);
CHAR VmIsActive();

ULONG_PTR IpiStartVirtualization(ULONG_PTR Argument);
NTSTATUS StartVirtualization(PVOID GuestRsp);