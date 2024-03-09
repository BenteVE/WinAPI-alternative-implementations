// A small collection of alternative ways to get a pointer to the PEB structure
// - the Process Environment Block (PEB) https://en.wikipedia.org/wiki/Process_Environment_Block

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

// A function to get a pointer to the PEB using the read functions in winnt.h
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
// (Usable for an external process)
PPEB get_PEB_NtQuery(HANDLE hProc)
{
	PROCESS_BASIC_INFORMATION pbi; //contains PEB pointer

	// Only the declaration of NtQueryInformationProcess is included in ntdll.h, 
	// to get access to the implemented function, we use GetProcAddress()
	// (ntdll.dll should always be loaded)
	tNtQueryInformationProcess NtQueryInformationProcess =
		(tNtQueryInformationProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");

	NTSTATUS status = NtQueryInformationProcess(hProc, ProcessBasicInformation, &pbi, sizeof(pbi), 0);

	if (NT_SUCCESS(status))
		return pbi.PebBaseAddress;
	
	return nullptr;
}