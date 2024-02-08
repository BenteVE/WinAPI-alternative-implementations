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


FARPROC getTrueAddress(LPCSTR moduleName, LPCSTR procName) {

	HMODULE h_module_custom = GetModuleHandleCustom(moduleName);
	if (h_module_custom == nullptr) {
		fprintf(console.stream, "GetModuleHandleCustom failed to retrieve a handle for module %s\n", moduleName);
		return nullptr;
	}
	fprintf(console.stream, "GetModuleHandleCustom retrieved handle %p for module %s\n", h_module_custom, moduleName);

	// Compare results of custom function with API function
	HMODULE h_module = GetModuleHandleCustom(moduleName);
	if (h_module == nullptr) {
		fprintf(console.stream, "GetModuleHandle failed to retrieve a handle for module %s\n", moduleName);
		return nullptr;
	}
	fprintf(console.stream, "GetModuleHandle retrieved handle %p for module %s\n", h_module, moduleName);



	FARPROC	address_custom = GetProcAddressCustom(h_module, procName);
	if (address_custom == nullptr) {
		fprintf(console.stream, "GetProcAddressCustom failed to retrieve an address for function %s\n", procName);
		return nullptr;
	}
	fprintf(console.stream, "GetProcAddressCustom retrieved address %p for function %s\n", address_custom, procName);

	// Compare results of custom function with API function
	FARPROC	address = GetProcAddressCustom(h_module, procName);
	if (address == nullptr) {
		fprintf(console.stream, "GetProcAddress failed to retrieve an address for function %s\n", procName);
		return nullptr;
	}
	fprintf(console.stream, "GetProcAddress retrieved address %p for function %s\n", address, procName);


	return address_custom;
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

		trueMessageBox = (TrueMessageBox)getTrueAddress("user32.dll", "MessageBoxW"); //Notepad++ 32-bit uses MessageBoxW
		if (!trueMessageBox) {
			return FALSE;
		}

		trueSetTextColor = (TrueSetTextColor)getTrueAddress("gdi32.dll", "SetTextColor");
		if (!trueSetTextColor) {
			return FALSE;
		}

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		// Attach all detours
		DetourAttach(&(PVOID&)trueMessageBox, MessageBoxHook);
		DetourAttach(&(PVOID&)trueSetTextColor, SetTextColorHook);

		LONG lError = DetourTransactionCommit();
		if (lError != NO_ERROR) {
			fprintf(console.stream, "Failed to attach the hooks\n");
			return FALSE;
		}
		fprintf(console.stream, "Hooks installed successfully\n");
	}
	break;

	case DLL_PROCESS_DETACH:
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		//Detach all detours
		DetourDetach(&(PVOID&)trueMessageBox, MessageBoxHook);
		DetourDetach(&(PVOID&)trueSetTextColor, SetTextColorHook);

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


