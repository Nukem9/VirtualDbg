#include "stdafx.h"

#define VM_BEGIN_VIRTUALIZATION CTL_CODE(FILE_DEVICE_UNKNOWN, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define VM_STOP_VIRTUALIZATION	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define VM_QUERY_RUNNING		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define VM_DEBUG_PROCESS		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define VM_DEBUG_CONSUME		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x905, METHOD_BUFFERED, FILE_ANY_ACCESS)

int main(int argc, char* argv[])
{
	HANDLE device = CreateFileA("\\\\.\\VirtualDbg", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (device == INVALID_HANDLE_VALUE)
	{
		printf("Unable to obtain driver handle!\n");
		return 1;
	}

	DWORD bytesWritten;
	if (!DeviceIoControl(device, VM_BEGIN_VIRTUALIZATION, nullptr, 0, nullptr, 0, &bytesWritten, nullptr))
	{
		printf("Virtualization init failed\n");
		return 1;
	}

	printf("waiting for input\n");
	getchar();

	ULONG pid = 1112;
	if (!DeviceIoControl(device, VM_DEBUG_PROCESS, &pid, sizeof(ULONG), nullptr, 0, &bytesWritten, nullptr))
	{
		printf("Debug process failed\n");
		return 1;
	}

	printf("waiting for input\n");
	getchar();

	if (!DeviceIoControl(device, VM_DEBUG_CONSUME, nullptr, 0, nullptr, 0, &bytesWritten, nullptr))
	{
		printf("Consume event failed\n");
		return 1;
	}

	Sleep(2000);

	__debugbreak();
	CloseHandle(device);
	return 0;
}