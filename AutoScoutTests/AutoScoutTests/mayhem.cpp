#include <Windows.h>

BOOL FakeConditionalVersion()
{
	OSVERSIONINFOEX pOsVersionInfo;
	RtlSecureZeroMemory(&pOsVersionInfo, sizeof(OSVERSIONINFOEX));

	pOsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if (!GetVersionEx((POSVERSIONINFO) &pOsVersionInfo))
		return FALSE;

	if (pOsVersionInfo.dwMajorVersion > 0xf1)
		return TRUE;

	return FALSE;
}
