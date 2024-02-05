/*
* A similar implementation to GetProcAddress to test if this method works on Wine
* We try to find the address of a real function through the PEB lists
*/

#include "MyProcGetAddress.h"

// format typedef struct myStructDefinition{struct info} myStruct, *myStructPointer;
// (common C idiom to avoid having to write "struct S")
typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

// PEB loader data structure
typedef struct _LDR_MODULE
{
	LIST_ENTRY      InLoadOrderModuleList;
	LIST_ENTRY      InMemoryOrderModuleList;
	LIST_ENTRY      InInitializationOrderModuleList;
	PVOID           BaseAddress;
	PVOID           EntryPoint;
	ULONG           SizeOfImage;
	UNICODE_STRING  FullDllName;
	UNICODE_STRING  BaseDllName;
	ULONG           Flags;
	SHORT           LoadCount;
	SHORT           TlsIndex;
	LIST_ENTRY      HashTableEntry;
	ULONG           TimeDateStamp;
} LDR_MODULE, * PLDR_MODULE;

/* Case Insensitive string compare */
int StringICompareA(const char* strA, LPCSTR strB) {
	return _stricmp(strA, strB);
}

/* String compare */
int StringCompareA(const char* strA, LPCSTR strB) {
	return strcmp(strA, strB);
}

/* Unicode to Ascii conversion */
std::string UnicodeStringToAscii(UNICODE_STRING* unicode) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::wstring wstr(unicode->Buffer, unicode->Length);
	const std::wstring wide_string = wstr;
	const std::string utf8_string = converter.to_bytes(wide_string);
	return utf8_string;
}

/* Using assembly code to retrieve the pointer to the list of PEB modules */
PLIST_ENTRY GetModListPtr()
{
	//
	// IN _PEB_NT:
	//   PPEB_LDR_DATA                LdrData;               //00C
	//
	// IN _PEB_LDR_DATA:
	//   LIST_ENTRY      InLoadOrderModuleList;              // 0C
	// 
	// LIST_ENTRY = Double linked list
	// typedef struct _LIST_ENTRY {
	// struct _LIST_ENTRY *Flink;       // offset 00 -> in reality it's a pointer to _LDR_MODULE
	// struct _LIST_ENTRY *Blink;       // offset 04
	// } LIST_ENTRY, *PLIST_ENTRY, *RESTRICTED_POINTER PRLIST_ENTRY;
	//
	// _LDR_MODULE:
	// typedef struct _LDR_MODULE {
	// LIST_ENTRY     InLoadOrderModuleList;               // 00
	// LIST_ENTRY     InMemoryOrderModuleList;             // 08
	// LIST_ENTRY     InInitializationOrderModuleList;     // 10
	// PVOID          BaseAddress;                         // 18
	// PVOID          EntryPoint;                          // 1C
	// ULONG          SizeOfImage;                         // 20
	// UNICODE_STRING FullDllName;                         // 24  
	// UNICODE_STRING BaseDllName;                         // 28
	// ...
	//

	// ReSharper disable once CppDeclaratorNeverUsed
	PVOID LdrData;
	PLIST_ENTRY ModList;

	/*
		Wikipedia on Win32 Thread Information Block: 
		The TIB can be used to get a lot of information on the process without calling Win32 API. 
		Examples include emulating GetLastError(), GetVersion(). Through the pointer to the PEB one 
		can obtain access to the import tables (IAT), process startup arguments, image name, etc. 
		It is accessed from the FS segment register on 32-bit Windows and GS on 64-bit Windows. 

		fs: [30h] = GS:[60h] => pointer to the Linear address of Process Environment Block (PEB) 
	*/

	__asm
	{
		xor eax, eax					//set register to 0 (opcode shorter than: mov eax, 0)
		xor ecx, ecx
		mov eax, fs: [30h]				// get pointer to Linear address of Process Environment Block (PEB) 
		mov ecx, dword ptr[eax + 0Ch]	// IN _PEB_NT: 0Ch offset points to PEB_LDR_DATA LdrData
		mov[LdrData], ecx				
		xor eax, eax
		mov eax, dword ptr[ecx + 0Ch]	// IN PEB_LDR_DATA: 0Ch offset points to InLoadOrderModuleList
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


/* Our implementation of the GetModuleHandle function */
HMODULE MyGetModuleHandle(LPCSTR ModuleName)
{
	// _LDR_MODULE:
	// typedef struct _LDR_MODULE {
	// LIST_ENTRY     InLoadOrderModuleList;               // 00
	// LIST_ENTRY     InMemoryOrderModuleList;             // 08
	// LIST_ENTRY     InInitializationOrderModuleList;     // 10
	// PVOID          BaseAddress;                         // 18
	// PVOID          EntryPoint;                          // 1C
	// ULONG          SizeOfImage;                         // 20
	// UNICODE_STRING FullDllName;                         // 24  
	// UNICODE_STRING BaseDllName;                         // 28
	// ...
	//
	// typedef struct _UNICODE_STRING {
	// USHORT Length;
	// USHORT MaximumLength;
	// PWSTR  Buffer;
	// } UNICODE_STRING, *PUNICODE_STRING;

	const auto ModList = GetModListPtr();

	for (auto ModEntry = ModList->Flink; ModList != ModEntry; ModEntry = ModEntry->Flink)
	{
		auto LdrMod = reinterpret_cast<PLDR_MODULE>(ModEntry);

		if (LdrMod->BaseAddress)
		{
			auto TmpModuleName = UnicodeStringToAscii(&LdrMod->BaseDllName);
			auto TmpModuleNameFull = UnicodeStringToAscii(&LdrMod->FullDllName);

			if (!StringICompareA(TmpModuleName.c_str(), ModuleName) ||
				!StringICompareA(TmpModuleNameFull.c_str(), ModuleName))
				return static_cast<HMODULE>(LdrMod->BaseAddress); // Module Handle == BaseAddress of that module
		}
	}

	return nullptr;
}


/* Returns the corresponding ordinal number for the specified function name. */
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

		if (!StringCompareA(Name, ProcName))
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


/*
*    Our implementation of the GetProcAddress function
*    in this implementation the argument is the ordinal of an exported function rather than the name
*/
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