#include <Windows.h>
#include "winternl_undoc.h"
#include "detours.h"

#include "GetProcAddressCustom.h"
#include "GetModuleHandleCustom.h"

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

		// Compare the addresses returned by our custom function with the ones from the Windows API function
		
		LPCSTR user32 = "user32.dll";
		LPCSTR messageBox = "MessageBoxW"; //Notepad++ 32-bit uses MessageBoxW


		HMODULE h_user32 = GetModuleHandleCustom(user32);

		// Check if the module handle is retrieved correctly
		if (h_user32 == nullptr) {
			fprintf(console.stream, "GetModuleHandleCustom failed to retrieve a handle for module %s", user32);
			return FALSE;
		}
		fprintf(console.stream, "GetModuleHandleCustom retrieved handle %p for module %s", h_user32, user32);

		trueMessageBox = (TrueMessageBox)GetProcAddressCustom(h_user32, messageBox);

		// Check if the Address is retrieved correctly
		if (trueMessageBox == nullptr) {
			fprintf(console.stream, "GetProcAddressCustom failed to retrieve an address for function %s", messageBox);
			return FALSE;
		}
		fprintf(console.stream, "GetProcAddressCustom failed to retrieve an address for function %s", messageBox);


		LPCSTR gdi32 = "gdi32.dll";
		LPCSTR setTextColor = "SetTextColor";

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		// Attach all detours
		DetourAttach(&(PVOID&)trueMessageBox, MessageBoxHook);
		//DetourAttach(&(PVOID&)trueSetTextColor, SetTextColorHook);

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
		//DetourDetach(&(PVOID&)trueSetTextColor, SetTextColorHook);

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


