#include "stdafx.h"
#include "../Misc/Pe.h"

ULONG_PTR GetNtoskrnlBase()
{
	//
	// Query the buffer size needed to list all modules
	//
	ULONG modulesSize	= 0;
	NTSTATUS status		= AuxKlibQueryModuleInformation(&modulesSize, sizeof(AUX_MODULE_EXTENDED_INFO), NULL);

	if (!NT_SUCCESS(status) || modulesSize == 0)
		return 0;

	//
	// Calculate the number of modules.
	//
	ULONG numberOfModules = modulesSize / sizeof(AUX_MODULE_EXTENDED_INFO);

	//
	// Allocate memory to receive data.
	//
	PAUX_MODULE_EXTENDED_INFO modules = (PAUX_MODULE_EXTENDED_INFO)ExAllocatePoolWithTag(
		PagedPool,
		modulesSize,
		'KLIB'
		);

	if (!modules)
		return 0;

	RtlZeroMemory(modules, modulesSize);

	//
	// Obtain the module information.
	//
	status = AuxKlibQueryModuleInformation(&modulesSize, sizeof(AUX_MODULE_EXTENDED_INFO), modules);

	//
	// Test symbol to check in the module range
	//
	ULONG_PTR symbolAddress = (ULONG_PTR)&DbgPrint;

	//
	// Enumerate all of the entries looking for NTOS*
	//
	ULONG_PTR moduleBase = 0;

	if (NT_SUCCESS(status))
	{
		for (ULONG i = 0; i < numberOfModules; i++)
		{
			ULONG_PTR moduleStart	= (ULONG_PTR)modules[i].BasicInfo.ImageBase;
			ULONG_PTR moduleEnd		= (ULONG_PTR)moduleStart + modules[i].ImageSize;

			if (symbolAddress >= moduleStart && moduleStart < moduleEnd)
				moduleBase = moduleStart;
		}
	}

	ExFreePoolWithTag(modules, 'KLIB');
	return moduleBase;
}

ULONG_PTR GetSSDTBase()
{
	//
	// The SSDT is found by using the pointer located inside of
	// KeAddSystemServiceTable, which is exported by NTOSKRNL.
	//
	UNICODE_STRING routineName;
	RtlInitUnicodeString(&routineName, L"KeAddSystemServiceTable");

	PVOID KeAddSystemServiceTable = MmGetSystemRoutineAddress(&routineName);

	if (!KeAddSystemServiceTable)
		return 0;

	//
	// Get a copy of the function's code
	//
	UCHAR functionData[1024];
	ULONG functionSize = 0;
	RtlCopyMemory(functionData, KeAddSystemServiceTable, sizeof(functionData));

	for (ULONG i = 0; i < sizeof(functionData); i++)
	{
		if (functionData[i] == 0xC3)
		{
			functionSize = i + 1;
			break;
		}
	}

	//
	// Will fail if 0xC3 (RETN) is never found
	//
	if (functionSize <= 0)
		return 0;

	//
	// Determine the SSDT RVA with a byte scan
	//
	ULONG rva = 0;

	for (ULONG i = 0; i < functionSize; i++)
	{
		//
		// 48 83 BC 18 80 4A 35 00 00       cmp qword ptr [rax+rbx+354A80h], 0
		//
		if (memcmp(&functionData[i], "\x48\x83\xBC", 3) == 0)
		{
			//
			// Verify the zero
			//
			if (functionData[i + 8] == 0x00)
			{
				rva = *(ULONG *)&functionData[i + 4];
				break;
			}
		}
	}

	//
	// NtosnkrlBase + RVA = SSDT address
	//
	ULONG_PTR ssdtAddress = GetNtoskrnlBase() + rva;

	//
	// Also check validity
	//
	if (!MmIsAddressValid((PVOID)ssdtAddress))
		return 0;

	return ssdtAddress;
}