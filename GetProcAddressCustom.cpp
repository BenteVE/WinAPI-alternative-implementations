#include "GetProcAddressCustom.h"

/* Used to get the NTHeader of a PE file */
PIMAGE_NT_HEADERS GetNtHeader(LPCVOID ModuleBase)
{
	auto DosHeader = static_cast<const _IMAGE_DOS_HEADER*>(ModuleBase);

	if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
	{
		const auto NtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(DosHeader) + DosHeader->e_lfanew);
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
	const auto ExportOffset = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

	// Pointer to export table
	const auto ExportDir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(reinterpret_cast<UINT_PTR>(ModuleHandle) + ExportOffset);

	// Pointer and length of AddressOfNames array
	const auto NamesArray = reinterpret_cast<PDWORD>(reinterpret_cast<UINT_PTR>(ModuleHandle) + ExportDir->AddressOfNames);

	// Empty names array
	if (ExportDir->NumberOfNames == 0)
		return 0xFFFFFFFF;

	// Pointer to AddressOfNameOrdinals array
	const auto NameOrdinalsArray = reinterpret_cast<PWORD>(reinterpret_cast<UINT_PTR>(ModuleHandle) + ExportDir->AddressOfNameOrdinals);

	// Search for name of function in the NamesArray
	WORD NameIndex = 0;
	auto FoundName = FALSE;
	for (DWORD I = 0; I < ExportDir->NumberOfNames; I++)
	{
		const auto Name = reinterpret_cast<const char*>(reinterpret_cast<UINT_PTR>(ModuleHandle) + NamesArray[I]);

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


// Search the Export Address Table for an export with the given orginal
FARPROC GetProcAddressCustom(HMODULE ModuleHandle, DWORD Ordinal)
{
	const auto NtHeader = GetNtHeader(ModuleHandle);
	if (!NtHeader)
		return nullptr;

	// RVA of export table
	const auto ExportOffset = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	const auto ExportSize = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	// Pointer to export table and export functions array
	const auto ExportDir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(reinterpret_cast<UINT_PTR>(ModuleHandle) + ExportOffset);
	const auto FunctionArray = reinterpret_cast<PDWORD>(reinterpret_cast<UINT_PTR>(ModuleHandle) + ExportDir->AddressOfFunctions);

	// Check if requested ordinal # is valid
	if ((Ordinal < ExportDir->Base) ||
		((Ordinal - ExportDir->Base) > ExportDir->NumberOfFunctions))
		return nullptr;

	// This works because the export table cannot have gaps
	// (if the ordinal is a gap then the corresponding export table entry contains zero)
	auto FunctionAddr = reinterpret_cast<FARPROC>(reinterpret_cast<UINT_PTR>(ModuleHandle) + FunctionArray[Ordinal - ExportDir->Base]);

	// Check if the exported function is not a forwarder
	// A forwarder looks just like a regular exported function, except that the entry in the ordinal export table points to another DLL
	if ((reinterpret_cast<UINT_PTR>(FunctionAddr) >= reinterpret_cast<UINT_PTR>(ExportDir)) &&
		(reinterpret_cast<UINT_PTR>(FunctionAddr) < (reinterpret_cast<UINT_PTR>(ExportDir) + ExportSize)))
	{
		auto ForwardedFunctionName = strchr(reinterpret_cast<const char*>(FunctionAddr), '.');
		if (!ForwardedFunctionName)
			return nullptr;

		// Module (DLL) name and function name		
		std::string ForwardedModule;
		ForwardedModule.assign(reinterpret_cast<const char*>(FunctionAddr), ForwardedFunctionName - reinterpret_cast<char*>(FunctionAddr));
		ForwardedFunctionName++;

		// Module handles are not global or inheritable. 
		// A call to LoadLibrary by one process does not produce a handle that another process can use — for example, in calling GetProcAddress. 
		// The other process must make its own call to LoadLibrary for the module before calling GetProcAddress.
		const auto ForwardedModHandle = LoadLibraryA(ForwardedModule.c_str());
		if (!ForwardedModHandle)
			return nullptr;

		// Get address of the function
		FunctionAddr = GetProcAddressCustom(ForwardedModHandle, NameToOrdinal(ForwardedModHandle, ForwardedFunctionName));

		// Free our handle to the module, this doesn't automatically unload the library!
		FreeLibrary(ForwardedModHandle);
	}

	return FunctionAddr;
}