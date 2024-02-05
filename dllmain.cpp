#include <Windows.h>
#include "detours.h"

#include "MyProcGetAddress.h"
#include "Console.h"

Console console;

/*--------------------------------------------------
		User32.dll MessageBox hook
----------------------------------------------------*/
typedef int(WINAPI* TrueMessageBox)(HWND, LPCTSTR, LPCTSTR, UINT);

TrueMessageBox trueMessageBox = NULL;

BOOL WINAPI MessageBoxHook(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
	LPCTSTR lpCaptionChanged = L"Hooked MessageBox";
	return trueMessageBox(hWnd, lpText, lpCaptionChanged, uType);
}

/*--------------------------------------------------
		GDI32.dll SetTextColor hook
----------------------------------------------------*/
typedef COLORREF(WINAPI* TrueSetTextColor)(HDC, COLORREF);

TrueSetTextColor trueSetTextColor = NULL;

BOOL WINAPI SetTextColorHook(HDC hdc, COLORREF color)
{
	//fprintf(console.stream, "SetTextColor for Handle Device context: %p\n", hdc);
	return trueSetTextColor(hdc, 0x00FF0000); // rgb color blue
}

/*--------------------------------------------------
		ntdll.dll NtCreateFile hook
----------------------------------------------------*/
#include <winternl.h>

typedef NTSTATUS(WINAPI* TrueNtCreateFile)(PHANDLE FileHandle, ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize,
	ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);

TrueNtCreateFile trueNtCreateFile = NULL;

BOOL WINAPI NtCreateFileHook(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK
	IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength)
{
	//fprintf(console.stream, "NtCreateFile created file with handle: %p\n", *FileHandle);
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
		if (!console.open()) {
			// Indicate DLL loading failed
			return FALSE;
		}

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
		DetourAttach(&(PVOID&)trueMessageBox, MessageBoxHook);
		DetourAttach(&(PVOID&)trueSetTextColor, SetTextColorHook);
		DetourAttach(&(PVOID&)trueNtCreateFile, NtCreateFileHook);

		LONG lError = DetourTransactionCommit();
		if (lError != NO_ERROR) {
			fprintf(console.stream, "Failed to attach the hooks\n");
			return FALSE;
		}
	}
	break;

	case DLL_PROCESS_DETACH:
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		//Detach all detours
		DetourDetach(&(PVOID&)trueMessageBox, MessageBoxHook);
		DetourDetach(&(PVOID&)trueSetTextColor, SetTextColorHook);
		DetourDetach(&(PVOID&)trueNtCreateFile, NtCreateFileHook);

		LONG lError = DetourTransactionCommit();
		if (lError != NO_ERROR) {
			fprintf(console.stream, "Failed to detach the hooks\n");
			return FALSE;
		}
	}
	break;
	}

	return TRUE;
}


