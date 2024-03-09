#include "GetModuleHandle.h"

// Custom implementation of the GetModuleHandle() function traversing the Linked lists in the PEB structure
HMODULE GetModuleHandleListEntry(LPCWSTR moduleName, PLIST_ENTRY head)
{
	// Note: no matter in which list we got the head in the PPEB_LDR_DATA (InLoadOrderModuleList, InMemoryOrderList, InInitializationOrderList) 
	// this code always traverses in load order because the first part of the PLDR_DATA_TABLE_ENTRY struct is the InLoadOrderLinks
	// TODO: try changing this traversal to: cast first, then traverse from the PLDR_DATA_TABLE_ENTRY
	for (PLIST_ENTRY ModEntry = head->Flink; head != ModEntry; ModEntry = ModEntry->Flink)
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