#pragma once

#include "ntdll.h"
#include <string>

PIMAGE_NT_HEADERS GetNtHeader(LPVOID ModuleBase);

DWORD NameToOrdinal(HMODULE ModuleHandle, LPCSTR ProcName);

FARPROC GetProcAddressEAT(HMODULE ModuleHandle, LPCSTR ProcName);
