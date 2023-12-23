/*
	PROJECT SETTINGS:
		- MICROSOFT DETOURS:
			Linker => Input => Additional Dependencies => detoursX86.lib
			VC++ Directories => Library Directories => Add /lib folder
*/


#include "pch.h"
#include "detours.h"
#include <string>
#include "MyProcGetAddress.h"
#include "ErrorMessageBox.h"
/*--------------------------------------------------
		Kernel32.dll WriteFile hook
----------------------------------------------------*/

typedef BOOL(WINAPI* TrueWriteFile)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);

TrueWriteFile trueWritefile = NULL;

// The function we will execute instead of the real WriteFile function
BOOL WINAPI hookedWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	const char* pBuf = "Your original text was replaced by the hooked function!";
	return trueWritefile(hFile, pBuf, 55, lpNumberOfBytesWritten, lpOverlapped);
}



/*--------------------------------------------------
		User32.dll MessageBox hook
----------------------------------------------------*/
typedef int(WINAPI* TrueMessageBox)(HWND, LPCTSTR, LPCTSTR, UINT);

TrueMessageBox trueMessageBox = NULL;

BOOL WINAPI hookedMessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
	//LPCTSTR lpTextChanged = L"This messagebox is also changed";
	LPCTSTR lpCaptionChanged = L"Hooked MessageBox";
	return trueMessageBox(hWnd, lpText, lpCaptionChanged, uType);
}

/*--------------------------------------------------
		GDI32.dll SetTextColor hook
----------------------------------------------------*/
typedef int(WINAPI* TrueSetTextColor)(HDC, COLORREF);

TrueSetTextColor trueSetTextColor = NULL;

BOOL WINAPI hookedSetTextColor(HDC hdc, COLORREF color)
{
	return trueSetTextColor(hdc, 0x00FF0000); // set color rgbBlue
}

/*--------------------------------------------------
		ntdll.dll NtCreateFile hook
----------------------------------------------------*/
#include <winternl.h>
int ntCreateFileCount = 0;
typedef NTSTATUS(WINAPI* TrueNtCreateFile)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize,
	ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);

TrueNtCreateFile trueNtCreateFile = NULL;

BOOL WINAPI hookedNtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK
	IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength)
{
	ntCreateFileCount++;
	return trueNtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

/*--------------------------------------------------
		DllMain
----------------------------------------------------*/

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{

	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	{

		//Get addresses of true functions
		trueWritefile = (TrueWriteFile)createHook("kernel32.dll", "WriteFile");
		if (trueWritefile == NULL)
			break;

		trueMessageBox = (TrueMessageBox)createHook("user32.dll", "MessageBoxW"); //Notepad++ 32-bit uses MessageBoxW
		if (trueMessageBox == NULL)
			break;

		trueSetTextColor = (TrueSetTextColor)createHook("gdi32.dll", "SetTextColor");
		if (trueSetTextColor == NULL)
			break;

		trueNtCreateFile = (TrueNtCreateFile)createHook("ntdll.dll", "NtCreateFile");
		if (trueNtCreateFile == NULL)
			break;


		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		// Attach all detours
		DetourAttach(&(PVOID&)trueWritefile, hookedWriteFile);
		DetourAttach(&(PVOID&)trueMessageBox, hookedMessageBox);
		DetourAttach(&(PVOID&)trueSetTextColor, hookedSetTextColor);
		DetourAttach(&(PVOID&)trueNtCreateFile, hookedNtCreateFile);

		LONG lError = DetourTransactionCommit();
		if (lError != NO_ERROR) {
			MessageBox(HWND_DESKTOP, L"Failed to detour", L"ATTACH FAILED", MB_OK);
			return FALSE;
		}
	}
	break;

	case DLL_PROCESS_DETACH:
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		//Detach all detours
		DetourDetach(&(PVOID&)trueWritefile, hookedWriteFile);
		DetourDetach(&(PVOID&)trueMessageBox, hookedMessageBox);
		DetourDetach(&(PVOID&)trueSetTextColor, hookedSetTextColor);
		DetourDetach(&(PVOID&)trueNtCreateFile, hookedNtCreateFile);

		LONG lError = DetourTransactionCommit();
		if (lError != NO_ERROR) {
			MessageBox(HWND_DESKTOP, L"Failed to detour", L"DETACH FAILED", MB_OK);
			return FALSE;
		}

		// print NtCreateFile execution count
		wchar_t buffer[256];
		wsprintfW(buffer, L"%d", ntCreateFileCount);
		MessageBox(HWND_DESKTOP, buffer, L"NtCreateFile execution count", MB_OK);
	}
	break;
	}

	return TRUE;
}


