#include <iostream>
#include "GetPEB.h"
#include "GetModuleHandle.h"

int main()
{
    std::cout << "Alternative ways to get a PEB pointer:" << std::endl;
    std::cout << std::hex << get_PEB_asm() << std::endl;
    std::cout << std::hex << get_PEB_readword() << std::endl;
    std::cout << std::hex << get_PEB_NtQuery(GetCurrentProcess()) << std::endl;

    std::cout << "Alternative ways to get a Module Handle (= base address):" << std::endl;
    LPCWSTR module_name = L"ntdll.dll";
    std::cout << std::hex << GetModuleHandle(module_name) << std::endl;
    std::cout << std::hex << GetModuleHandleListEntry(module_name) << std::endl;
    std::cout << std::hex << GetModuleHandleSnapshot(module_name, GetCurrentProcessId()) << std::endl;
}