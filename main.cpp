#include <iostream>
#include "GetPEB.h"
#include "GetModuleHandle.h"
#include "GetProcAddress.h"
#include "GetModuleListEntry.h"

int main()
{
    std::cout << "Alternative ways to get a PEB pointer:" << std::endl;
    std::cout << std::hex << get_PEB_asm() << std::endl;
    std::cout << std::hex << get_PEB_readword() << std::endl;
    std::cout << std::hex << get_PEB_NtQuery(GetCurrentProcess()) << std::endl;
    PPEB peb = get_PEB_asm();

    std::cout << "Alternative ways to traverse the PEB to get a PPEB_LDR_DATA:" << std::endl;
    std::cout << std::hex << GetLdrDataAsm(peb) << std::endl;
    std::cout << std::hex << GetLdrDataStruct(peb) << std::endl;
    PPEB_LDR_DATA ldr = GetLdrDataAsm(peb);

    std::cout << "Alternative ways to traverse the PPEB_LDR_DATA to get a LIST_ENTRY:" << std::endl;
    std::cout << std::hex << GetListEntryAsm(ldr) << std::endl;
    std::cout << std::hex << GetListEntryStruct(ldr) << std::endl;
    PLIST_ENTRY head = GetListEntryAsm(ldr);

    std::cout << "Alternative ways to get a Module Handle (= base address):" << std::endl;
    LPCWSTR module_name = L"ntdll.dll";
    std::cout << std::hex << GetModuleHandle(module_name) << std::endl;
    std::cout << std::hex << GetModuleHandleListEntry(module_name, head) << std::endl;
    std::cout << std::hex << GetModuleHandleSnapshot(module_name, GetCurrentProcessId()) << std::endl;

    std::cout << "Alternative ways to get the address of a function (= GetProcAddress):" << std::endl;
    HMODULE module_handle = GetModuleHandle(module_name);
    LPCSTR function_name = "NtQueryInformationProcess";
    std::cout << std::hex << GetProcAddress(module_handle, function_name) << std::endl;
    std::cout << std::hex << GetProcAddressEAT(module_handle, function_name) << std::endl;
}