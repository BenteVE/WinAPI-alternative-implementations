#pragma once
#include "pch.h"

//used for showing fault messages
enum class Error { handle, ordinal, getProcAddress };

void showErrorMessageBox(Error error, LPCSTR dllName, LPCSTR functionName);
