#include "GetModuleHandle.h"

// A custom implementation of the GetModuleHandle() function of the Windows API
// that uses assembly code to traverse:
// - the Thread Information Block (TIB)  https://en.wikipedia.org/wiki/Win32_Thread_Information_Block
// - the Process Environment Block (PEB) https://en.wikipedia.org/wiki/Process_Environment_Block


// Note: this function only works in x86 because the MSVC compiler doesn't support assembly in x64
PLIST_ENTRY GetModListPtr()
{
	PVOID LdrData; // Pointer to PPEB_LDR_DATA structure
	PLIST_ENTRY ModList; // Pointer to a LIST_ENTRY, this is the first  is part of a double linked list of contains pointers toPointer to _LDR_MODULE structure

	__asm
	{
		// clear registers (opcode shorter than: mov eax, 0)
		xor eax, eax
		xor ecx, ecx

		// Using the TIB to get a pointer to the PEB
		// The TIB is accessed from the FS segment register on 32-bit Windows and GS on 64-bit Windows
		// Offset to PEB:  fs: [30h] // GS:[60h]
		mov eax, fs: [30h]

		// Using the PEB to get a pointer to the PPEB_LDR_DATA structure
		// typedef struct _PEB			/* win32/win64 */
		//	 PPEB_LDR_DATA				  /* 00c/018 */
		mov ecx, dword ptr[eax + 0Ch]
		mov[LdrData], ecx

		// clear register
		xor eax, eax

		// Using the PPEB_LDR_DATA to get a pointer to the LIST_ENTRY structure InLoadOrderModuleList
		// This LIST_ENTRY forms a double linked list and contains a pointer to the first _LDR_MODULE
		mov eax, dword ptr[ecx + 0Ch]
		mov[ModList], eax
	}

	return ModList;
}

// Custom implementation of the GetModuleHandle() function traversing the Linked lists in 
HMODULE GetModuleHandleListEntry(LPCWSTR moduleName)
{
	PLIST_ENTRY ModList = GetModListPtr();

	for (PLIST_ENTRY ModEntry = ModList->Flink; ModList != ModEntry; ModEntry = ModEntry->Flink)
	{
		PLDR_DATA_TABLE_ENTRY LdrMod = reinterpret_cast<PLDR_DATA_TABLE_ENTRY>(ModEntry);

		if (LdrMod->DllBase)
		{
			if (_wcsicmp(LdrMod->BaseDllName.Buffer, moduleName) == 0 ||
				_wcsicmp(LdrMod->FullDllName.Buffer, moduleName) == 0)
				return static_cast<HMODULE>(LdrMod->DllBase); // Module Handle == base address of that module
		}
	}

	return nullptr;
}

// Custom implementation for GetModuleHandle() function using CreateToolhelp32Snapshot
HMODULE GetModuleHandleSnapshot(LPCWSTR moduleName, DWORD processId)
{
	HMODULE moduleBaseAddress = nullptr;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 moduleEntry{};
		moduleEntry.dwSize = sizeof(moduleEntry);
		if (Module32First(hSnap, &moduleEntry))
		{
			do
			{
				if (_wcsicmp(moduleEntry.szModule, moduleName) == 0)
				{
					moduleBaseAddress = (HMODULE)moduleEntry.modBaseAddr;
					break;
				}
			} while (Module32Next(hSnap, &moduleEntry));
		}
	}
	CloseHandle(hSnap);
	return moduleBaseAddress;
}