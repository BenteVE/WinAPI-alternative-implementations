#include "GetModuleListEntry.h"

// Traverse PEB to PPEB_LDR_DATA using assembly
// Note: this function only works in x86 because the MSVC compiler doesn't support assembly in x64
#ifndef _WIN64
PPEB_LDR_DATA GetLdrDataAsm(PPEB peb)
{
	PPEB_LDR_DATA ldr; // Pointer to PPEB_LDR_DATA structure
	__asm
	{
		mov eax, [peb]

		// Using the PEB to get a pointer to the PPEB_LDR_DATA structure
		// typedef struct _PEB			/* win32/win64 */
		//	 PPEB_LDR_DATA				  /* 00c/018 */
		mov ecx, dword ptr[eax + 0Ch]

		mov[ldr], ecx
	}
	return ldr;
}
#endif

// Traverse PEB to PPEB_LDR_DATA using the structure definition
PPEB_LDR_DATA GetLdrDataStruct(PPEB peb)
{
	return peb->Ldr;
}

// Traverse PPEB_LDR_DATA get a LIST_ENTRY using assembly
// Note: this function only works in x86 because the MSVC compiler doesn't support assembly in x64
#ifndef _WIN64
PLIST_ENTRY GetListEntryAsm(PPEB_LDR_DATA ldr)
{
	PLIST_ENTRY head; // Pointer to a LIST_ENTRY, this is the first  is part of a double linked list of contains pointers toPointer to _LDR_MODULE structure

	__asm
	{
		mov eax, [ldr]

		// Using the PPEB_LDR_DATA to get a pointer to the LIST_ENTRY structure InLoadOrderModuleList
		// This LIST_ENTRY forms a double linked list and contains a pointer to the first PLDR_DATA_TABLE_ENTRY
		// Note: PPEB_LDR_DATA has alignment = 4 bytes, so InLoadOrderModuleList is at offset 0C
		// Note: we immediately take the Flink in the LIST_ENTRY
		mov ecx, dword ptr[eax + 0Ch]

		mov[head], ecx
	}
	return head;
}
#endif

// Traverse PPEB_LDR_DATA get a LIST_ENTRY using the structure definition
PLIST_ENTRY GetListEntryStruct(PPEB_LDR_DATA ldr)
{
	return ldr->InLoadOrderModuleList.Flink;
}