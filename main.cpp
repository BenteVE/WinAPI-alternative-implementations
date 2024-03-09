#include <iostream>
#include "GetPEB.h"

int main()
{
    std::cout << "Alternative ways to get a PEB pointer:" << std::endl;
    std::cout << std::hex << get_PEB_asm() << std::endl;
    std::cout << std::hex << get_PEB_readword() << std::endl;
    std::cout << std::hex << get_PEB_NtQuery() << std::endl;
}