#include <Windows.h>

#ifndef _WIN_HTTP
#define _WIN_HTTP

PBYTE WinHTTPGetResponse(PULONG uOut);
BOOL WinHTTPSendData(PBYTE pBuffer, ULONG uBuffLen);
BOOL WinHTTPSetup(PBYTE pServerUrl, PBYTE pAddrToConnect, ULONG uBufLen, PULONG pPortToConnect);
VOID WinHTTPClose();
BOOL ResolveName(PBYTE pServerUrl, PBYTE pAddrToConnect, ULONG uBufLen);

#endif