#pragma once

#include <Windows.h>
#include <string>
#include <codecvt> //for unicode to ascii conversion

typedef struct _UNICODE_STRING UNICODE_STRING;
typedef struct _LDR_MODULE LDR_MODULE;

int StringICompareA(const char* strA, LPCSTR strB);

int StringCompareA(const char* strA, LPCSTR strB);

std::string UnicodeStringToAscii(UNICODE_STRING* unicode);

PLIST_ENTRY GetModListPtr();

PIMAGE_NT_HEADERS GetNtHeader(LPCVOID ModuleBase);

HMODULE MyGetModuleHandle(LPCSTR ModuleName);

DWORD NameToOrdinal(HMODULE ModuleHandle, LPCSTR ProcName);

FARPROC MyGetProcAddressInternal(HMODULE ModuleHandle, DWORD Ordinal);

FARPROC createHook(LPCSTR dllName, LPCSTR functionName);