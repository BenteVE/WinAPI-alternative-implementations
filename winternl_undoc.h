#pragma once

#include <Windows.h>

//#include <winternl.h>

// These structures are only partially available in the official winternl.h header
// but they can be found on the internet or through reverse engineering
// https://github.com/wine-mirror/wine/blob/master/include/winternl.h

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING* PUNICODE_STRING;

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

typedef struct _OBJECT_ATTRIBUTES {
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES* POBJECT_ATTRIBUTES;

typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID Pointer;
	} DUMMYUNIONNAME;
	ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;
