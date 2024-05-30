#include "GetProcAddress.h"

// A custom implementation of the GetProcAddress() function of the Windows API
// that traverses the Export Address Table (EAT)
// https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#export-directory-table

// Convert a pointer to the base of a PE module into a pointer to the IMAGE_NT_HEADER struct
PIMAGE_NT_HEADERS GetNtHeader(LPVOID ModuleBase)
{
	PIMAGE_DOS_HEADER DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ModuleBase);

	if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
	{
		PIMAGE_NT_HEADERS NtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(DosHeader) + DosHeader->e_lfanew);
		if (NtHeader->Signature == LOWORD(IMAGE_NT_SIGNATURE))
			return NtHeader;
	}

	return nullptr;
}

// Search the Export Address Table of a module for the ordinal number of the specified function name
DWORD NameToOrdinal(HMODULE ModuleHandle, LPCSTR ProcName)
{
	if (ModuleHandle == nullptr)
		return 0xFFFFFFFF;

	const auto NtHeader = GetNtHeader(ModuleHandle);
	if (!NtHeader)
		return 0xFFFFFFFF;

	// RVA of export table
	const DWORD ExportOffset = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

	// Pointer to export table
	const PIMAGE_EXPORT_DIRECTORY ExportDir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(reinterpret_cast<UINT_PTR>(ModuleHandle) + ExportOffset);

	// Pointer and length of AddressOfNames array
	const PDWORD NamesArray = reinterpret_cast<PDWORD>(reinterpret_cast<UINT_PTR>(ModuleHandle) + ExportDir->AddressOfNames);

	// Empty names array
	if (ExportDir->NumberOfNames == 0)
		return 0xFFFFFFFF;

	// Pointer to AddressOfNameOrdinals array
	const PWORD NameOrdinalsArray = reinterpret_cast<PWORD>(reinterpret_cast<UINT_PTR>(ModuleHandle) + ExportDir->AddressOfNameOrdinals);

	// Search for name of function in the NamesArray
	DWORD NameIndex = 0;
	BOOL FoundName = FALSE;
	for (DWORD I = 0; I < ExportDir->NumberOfNames; I++)
	{
		const auto Name = reinterpret_cast<const char *>(reinterpret_cast<UINT_PTR>(ModuleHandle) + NamesArray[I]);

		if (!strcmp(Name, ProcName))
		{
			NameIndex = I;
			FoundName = TRUE;
			break;
		}
	}

	// Not found
	if (!FoundName)
		return 0xFFFFFFFF;

	// Return ordinal #
	return NameOrdinalsArray[NameIndex] + ExportDir->Base;
}

// Search the Export Address Table for an export with the given name
FARPROC GetProcAddressEAT(HMODULE moduleHandle, LPCSTR procName)
{
	DWORD ordinal = NameToOrdinal(moduleHandle, procName);

	// Check if the ordinal is retrieved correctly
	if (ordinal == 0xFFFFFFFF)
	{
		return nullptr;
	}

	const PIMAGE_NT_HEADERS NtHeader = GetNtHeader(moduleHandle);
	if (!NtHeader)
		return nullptr;

	// Search the Export Address Table for an export with the given orginal

	// RVA of export table
	const DWORD ExportOffset = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	const DWORD ExportSize = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	// Pointer to export table and export functions array
	const PIMAGE_EXPORT_DIRECTORY ExportDir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(reinterpret_cast<UINT_PTR>(moduleHandle) + ExportOffset);
	const PDWORD FunctionArray = reinterpret_cast<PDWORD>(reinterpret_cast<UINT_PTR>(moduleHandle) + ExportDir->AddressOfFunctions);

	// Check if requested ordinal # is valid
	if ((ordinal < ExportDir->Base) ||
		((ordinal - ExportDir->Base) > ExportDir->NumberOfFunctions))
		return nullptr;

	// This works because the export table cannot have gaps
	// (if the ordinal is a gap then the corresponding export table entry contains zero)
	FARPROC FunctionAddr = reinterpret_cast<FARPROC>(reinterpret_cast<UINT_PTR>(moduleHandle) + FunctionArray[ordinal - ExportDir->Base]);

	// Check if the exported function is not a forwarder
	// A forwarder looks just like a regular exported function, except that the entry in the ordinal export table points to another DLL
	if ((reinterpret_cast<UINT_PTR>(FunctionAddr) >= reinterpret_cast<UINT_PTR>(ExportDir)) &&
		(reinterpret_cast<UINT_PTR>(FunctionAddr) < (reinterpret_cast<UINT_PTR>(ExportDir) + ExportSize)))
	{
		const char *ForwardedFunctionName = strchr(reinterpret_cast<const char *>(FunctionAddr), '.');
		if (!ForwardedFunctionName)
			return nullptr;

		// Module (DLL) name and function name
		std::string ForwardedModule;
		ForwardedModule.assign(reinterpret_cast<const char *>(FunctionAddr), ForwardedFunctionName - reinterpret_cast<char *>(FunctionAddr));
		ForwardedFunctionName++;

		// Module handles are not global or inheritable.
		// A call to LoadLibrary by one process does not produce a handle that another process can use � for example, in calling GetProcAddress.
		// The other process must make its own call to LoadLibrary for the module before calling GetProcAddress.
		HMODULE ForwardedModHandle = LoadLibraryA(ForwardedModule.c_str());
		if (!ForwardedModHandle)
			return nullptr;

		// Get address of the function
		FunctionAddr = GetProcAddressEAT(ForwardedModHandle, ForwardedFunctionName);

		// Free our handle to the module, this doesn't automatically unload the library!
		FreeLibrary(ForwardedModHandle);
	}

	return FunctionAddr;
}