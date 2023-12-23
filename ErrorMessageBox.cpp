#include "pch.h"
#include "ErrorMessageBox.h"

//used for showing fault messages
void showErrorMessageBox(Error error, LPCSTR dllName, LPCSTR functionName) {
	switch (error)
	{
	case Error::handle:
		MessageBox(HWND_DESKTOP, L"Module handle not found", L"Module handle not found", MB_OK);
		break;
	case Error::ordinal:
		MessageBox(HWND_DESKTOP, L"Ordinal not found", L"Ordinal not found", MB_OK);
		break;
	case Error::getProcAddress:
		MessageBox(HWND_DESKTOP, L"GetProcAddress Failed", L"GetProcAddress Failed, aborting detours", MB_OK);
		break;
	default:
		break;
	}
}