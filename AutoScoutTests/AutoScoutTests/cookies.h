#ifndef _COOKIES_H
#define _COOKIES_H

#include <windows.h>


typedef struct _COOKIE_LIST
{
	LPSTR strDomain;
	LPSTR strName;
	LPSTR strValue;
} COOKIE_LIST, *LPCOOKIE_LIST;

extern LPCOOKIE_LIST pCookieList;

VOID DumpCookies();
BOOL AddCookieA(__in LPSTR strDomain, __in LPSTR strName, __in LPSTR strValue);
BOOL AddCookieW(__in LPWSTR strDomainW, __in LPWSTR strNameW, __in LPWSTR strValueW);
LPSTR GetCookieString(__in LPWSTR strDomain);
VOID NormalizeDomainA(__in LPSTR strDomain);
VOID NormalizeDomainW(__in LPWSTR strDomain);


#endif _COOKIES_H