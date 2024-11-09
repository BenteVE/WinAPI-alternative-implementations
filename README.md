# WinAPI-alternative-implementations

This project contains some alternative implementations for the following WinAPI functions:

- `GetProcAddress` by traversing the Export Address Table
- `GetModuleHandle` by traversing the `PEB`, `PPEB_LDR_DATA` and `LIST_ENTRY` structures with assembly and C++
- `GetModuleHandle` by using `CreateToolhelp32Snapshot`

The main program simply verifies these implementations by comparing their return values to those of the original functions.
