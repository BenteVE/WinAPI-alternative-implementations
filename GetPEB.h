#pragma once

#include "ntdll.h"

PPEB get_PEB_asm();
PPEB get_PEB_readword();

// Usable for an external process
PPEB get_PEB_NtQuery(HANDLE hProc);

// Signature of the NtQueryInformationProcess function
// We need this to cast a pointer to the function
typedef NTSTATUS(__stdcall *tNtQueryInformationProcess)(
	HANDLE ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength,
	PULONG ReturnLength);
