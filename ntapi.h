#pragma once

#include <windows.h>
// Header file with undocumented all structures we need to make our alternative function work
#include "ntdll.h"

/*
	The function we will be using to retrieve information about the specified process.
*/
typedef NTSTATUS (__stdcall* tNtQueryInformationProcess)(
	HANDLE           ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID            ProcessInformation,
	ULONG            ProcessInformationLength,
	PULONG           ReturnLength
);
