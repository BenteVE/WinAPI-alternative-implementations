#include <iostream>
#include "GetPEB.h"
#include "GetModuleHandle.h"
#include "GetProcAddress.h"

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

    std::cout << "Alternative ways to get the address of a function (= GetProcAddress):" << std::endl;
    HMODULE module_handle = GetModuleHandle(module_name);
    LPCSTR function_name = "NtQueryInformationProcess";
    std::cout << std::hex << GetProcAddress(module_handle, function_name) << std::endl;
    std::cout << std::hex << GetProcAddressEAT(module_handle, function_name) << std::endl;
}