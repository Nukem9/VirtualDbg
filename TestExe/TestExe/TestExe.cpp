#include "stdafx.h"

int main(int argc, char* argv[])
{
	HANDLE device = CreateFileA("\\\\.\\TitanHide", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

	if (device == INVALID_HANDLE_VALUE)
	{
		printf("Unable to obtain driver handle!\n");
		return 1;
	}

	DWORD bytesWritten;
	WriteFile(device, nullptr, sizeof(int), &bytesWritten, nullptr);


	CloseHandle(device);
	return 0;
}