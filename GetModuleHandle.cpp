#include "GetModuleHandle.h"

// Custom implementation of the GetModuleHandle() function traversing the Linked lists in the PEB structure
HMODULE GetModuleHandleListEntry(LPCWSTR moduleName, PLIST_ENTRY head)
{
	PLIST_ENTRY ModList = head;

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