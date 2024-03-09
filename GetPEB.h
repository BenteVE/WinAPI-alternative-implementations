#pragma once

#include "ntapi.h"

PPEB get_PEB_asm();
PPEB get_PEB_readword();
PPEB get_PEB_NtQuery();

// Usable for an external process
PEB get_PEB_NtQuery(HANDLE hProc);
