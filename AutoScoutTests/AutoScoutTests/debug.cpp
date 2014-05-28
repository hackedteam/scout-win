#include <Windows.h>

#include "globals.h"
#include "zmem.h"
#include "utils.h"
#include "mayhem.h"

VOID OutputDebug(LPWSTR strFormat, ...)
{
	va_list vArgs;

	va_start(vArgs, strFormat);
	LPWSTR strDebug = (LPWSTR) malloc((MAX_DEBUG_STRING + 2) * sizeof(WCHAR)); // do not use zalloc here
	SecureZeroMemory(strDebug, (MAX_DEBUG_STRING + 2) * sizeof(WCHAR));
	vswprintf_s(strDebug, MAX_DEBUG_STRING, strFormat, vArgs);
	OutputDebugString(strDebug);

	free(strDebug);
}