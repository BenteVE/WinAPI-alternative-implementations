#pragma once

#include "ntdll.h"

#ifndef _WIN64
PPEB_LDR_DATA GetLdrDataAsm(PPEB peb);
PLIST_ENTRY GetListEntryAsm(PPEB_LDR_DATA ldr);
#endif

PPEB_LDR_DATA GetLdrDataStruct(PPEB peb);
PLIST_ENTRY GetListEntryStruct(PPEB_LDR_DATA ldr);