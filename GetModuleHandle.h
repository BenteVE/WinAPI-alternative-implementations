#pragma once

#include "ntdll.h"
#include <string>
#include <TlHelp32.h>

PLIST_ENTRY GetModListPtr();

HMODULE GetModuleHandleListEntry(LPCWSTR moduleName);
HMODULE GetModuleHandleSnapshot(LPCWSTR moduleName, DWORD processId);