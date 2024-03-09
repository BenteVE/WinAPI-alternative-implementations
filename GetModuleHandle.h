#pragma once

#include "ntdll.h"
#include <TlHelp32.h>

HMODULE GetModuleHandleListEntry(LPCWSTR moduleName, PLIST_ENTRY head);
HMODULE GetModuleHandleSnapshot(LPCWSTR moduleName, DWORD processId);