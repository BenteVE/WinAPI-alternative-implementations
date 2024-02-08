#pragma once

#include "winternl_undoc.h"
#include <string>

PIMAGE_NT_HEADERS GetNtHeader(LPCVOID ModuleBase);

DWORD NameToOrdinal(HMODULE ModuleHandle, LPCSTR ProcName);

FARPROC GetProcAddressCustom(HMODULE ModuleHandle, LPCSTR ProcName);
