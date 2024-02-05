#pragma once

#include <Windows.h>
#include <string>
#include <codecvt> //for unicode to ascii conversion

// These structures are only partially available in the official winternl.h header
// but they can be found on the internet or through reverse engineering
// https://github.com/wine-mirror/wine/blob/master/include/winternl.h

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;


// PEB loader data structure
typedef struct _LDR_MODULE
{
	// LIST_ENTRY (declared in winnt.h) is a part of a double linked list:
	// -> *Flink and *Blink point to the next en previous _LDR_MODULE in the list
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

typedef struct _PEB_LDR_DATA
{
	ULONG               Length;
	BOOLEAN             Initialized;
	PVOID               SsHandle;
	LIST_ENTRY          InLoadOrderModuleList;
	LIST_ENTRY          InMemoryOrderModuleList;
	LIST_ENTRY          InInitializationOrderModuleList;
	PVOID               EntryInProgress;
	BOOLEAN             ShutdownInProgress;
	HANDLE              ShutdownThreadId;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

std::string UnicodeStringToAscii(UNICODE_STRING* unicode);

PLIST_ENTRY GetModListPtr();

PIMAGE_NT_HEADERS GetNtHeader(LPCVOID ModuleBase);

HMODULE MyGetModuleHandle(LPCSTR ModuleName);

DWORD NameToOrdinal(HMODULE ModuleHandle, LPCSTR ProcName);

FARPROC MyGetProcAddressInternal(HMODULE ModuleHandle, DWORD Ordinal);

FARPROC createHook(LPCSTR dllName, LPCSTR functionName);