// A small collection of alternative ways to get a pointer to the PEB structure
// - the Process Environment Block (PEB) https://en.wikipedia.org/wiki/Process_Environment_Block
// 
// A custom implementation of the GetModuleHandle() function of the Windows API
// that uses assembly code to traverse:



#include "GetPEB.h"



// A function to get a pointer to the PEB from the TIB using assembly
// - the Thread Information Block (TIB)  https://en.wikipedia.org/wiki/Win32_Thread_Information_Block
// Note: this function only works in x86 because the MSVC compiler doesn't support assembly in x64
#ifndef _WIN64
PPEB get_PEB_asm()
{
	PPEB peb;

	__asm
	{
		// clear register (opcode shorter than: mov eax, 0)
		xor eax, eax

		// Using the TIB to get a pointer to the PEB
		// The TIB is accessed from the FS segment register on 32-bit Windows and GS on 64-bit Windows
		// Offset to PEB:  fs: [30h] // GS:[60h]
		mov eax, fs: [30h]
		mov[peb], eax
	}

	return peb;
}
#endif _WIN64


// A function to get a pointer to the PEB using the functions in winnt.h
PPEB get_PEB_readword()
{
#ifdef _WIN64
	PPEB peb = (PPEB)__readgsqword(0x60);
#else
	PPEB peb = (PPEB)__readfsdword(0x30);
#endif
	return peb;
}

// A function to get a pointer to the PEB using NtQueryInformationProcess
PPEB get_PEB_NtQuery()
{
	PROCESS_BASIC_INFORMATION pbi; //contains PEB pointer
	PEB peb = { 0 };

	// To get access to the NtQueryInformationProcess function,
	// we need to get the address using GetProcAddress()
	// ...
	tNtQueryInformationProcess NtQueryInformationProcess =
		(tNtQueryInformationProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");

	NTSTATUS status = NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), 0);

	if (NT_SUCCESS(status))
		return pbi.PebBaseAddress;
	
	return nullptr;
}

// A function to get the PEB of an external process using NtQueryInformationProcess
PEB get_PEB_NtQuery(HANDLE hProc)
{
	PROCESS_BASIC_INFORMATION pbi; //contains PEB pointer
	PEB peb = { 0 };

	tNtQueryInformationProcess NtQueryInformationProcess =
		(tNtQueryInformationProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");

	NTSTATUS status = NtQueryInformationProcess(hProc, ProcessBasicInformation, &pbi, sizeof(pbi), 0);

	if (NT_SUCCESS(status))
	{
		ReadProcessMemory(hProc, pbi.PebBaseAddress, &peb, sizeof(peb), 0);
	}

	return peb;
}