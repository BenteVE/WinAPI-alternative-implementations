#pragma once

#include "ntdll.h"
#include <string>
#include <codecvt> //for unicode to ascii conversion

PLIST_ENTRY GetModListPtr();

std::string UnicodeStringToAscii(UNICODE_STRING* unicode);

HMODULE GetModuleHandleCustom(LPCSTR ModuleName);