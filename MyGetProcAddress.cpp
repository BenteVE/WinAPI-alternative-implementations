#include "MyProcGetAddress.h"

// A custom implementation of the GetProcAddress() function of the Windows API
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

/* Unicode to Ascii conversion */
std::string UnicodeStringToAscii(UNICODE_STRING* unicode) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::wstring wstr(unicode->Buffer, unicode->Length);
	const std::wstring wide_string = wstr;
	const std::string utf8_string = converter.to_bytes(wide_string);
	return utf8_string;
}

/* Our implementation of the GetModuleHandle function */
HMODULE MyGetModuleHandle(LPCSTR ModuleName)
{
	const auto ModList = GetModListPtr();

	for (auto ModEntry = ModList->Flink; ModList != ModEntry; ModEntry = ModEntry->Flink)
	{
		auto LdrMod = reinterpret_cast<PLDR_MODULE>(ModEntry);

		if (LdrMod->BaseAddress)
		{
			auto TmpModuleName = UnicodeStringToAscii(&LdrMod->BaseDllName);
			auto TmpModuleNameFull = UnicodeStringToAscii(&LdrMod->FullDllName);

			if (!_stricmp(TmpModuleName.c_str(), ModuleName) ||
				!_stricmp(TmpModuleNameFull.c_str(), ModuleName))
				return static_cast<HMODULE>(LdrMod->BaseAddress); // Module Handle == BaseAddress of that module
		}
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
FARPROC MyGetProcAddressInternal(HMODULE ModuleHandle, DWORD Ordinal)
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

	// Export forward ?
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

		// Load library
		const auto ForwardedModHandle = LoadLibraryA(ForwardedModule.c_str());
		if (!ForwardedModHandle)
			return nullptr;

		// Get addr of function
		FunctionAddr = MyGetProcAddressInternal(ForwardedModHandle, NameToOrdinal(ForwardedModHandle, ForwardedFunctionName));
		FreeLibrary(ForwardedModHandle);
	}

	return FunctionAddr;
}

FARPROC createHook(LPCSTR dllName, LPCSTR functionName) {

	HMODULE moduleHandle = MyGetModuleHandle(dllName);

	// Check if the module handle is retrieved correctly
	if (moduleHandle == nullptr) {
		return NULL;
	}

	DWORD ordinal = NameToOrdinal(moduleHandle, functionName);

	// Check if the ordinal is retrieved correctly
	if (ordinal == 0xFFFFFFFF) {
		return NULL;
	}

	FARPROC trueFunction = MyGetProcAddressInternal(moduleHandle, ordinal);

	// Check if the Address is retrieved correctly
	if (trueFunction == nullptr) {
		return NULL;
	}

	return trueFunction;

}