#include <Windows.h>
#include <Lm.h>
#include <Sddl.h>
#include "device_agent.h"



VOID GetDeviceInfo()
{
	HKEY hKey;
	ULONG uLen;
	PDEVICE_INFO pDeviceInfo;
#ifdef _DEBUG
	OutputDebugString(L"[+] Starting GetDeviceInfo\n");
#endif

	pDeviceInfo = (PDEVICE_INFO)malloc(sizeof(DEVICE_INFO));
	memset(pDeviceInfo, 0x0, sizeof(DEVICE_INFO));

	// Processor
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
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
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		uLen = sizeof(pDeviceInfo->osinfo.ver);
		if (RegQueryValueEx(hKey, L"ProductName", NULL, NULL, (PBYTE)pDeviceInfo->osinfo.ver, &uLen) != ERROR_SUCCESS)
			pDeviceInfo->osinfo.ver[0] = L'\0';

		uLen = sizeof(pDeviceInfo->osinfo.sp);
		if (RegQueryValueEx(hKey, L"CSDVersion", NULL, NULL, (PBYTE)pDeviceInfo->osinfo.sp, &uLen) != ERROR_SUCCESS)
			pDeviceInfo->osinfo.sp[0] = L'\0';

		uLen = sizeof(pDeviceInfo->osinfo.id);
		if (RegQueryValueEx(hKey, L"ProductId", NULL, NULL, (PBYTE)pDeviceInfo->osinfo.id, &uLen) != ERROR_SUCCESS)
			pDeviceInfo->osinfo.id[0] = L'\0';

		uLen = sizeof(pDeviceInfo->osinfo.owner);
		if (RegQueryValueEx(hKey, L"RegisteredOwner", NULL, NULL, (PBYTE)pDeviceInfo->osinfo.owner, &uLen) != ERROR_SUCCESS)
			pDeviceInfo->osinfo.owner[0] = L'\0';

		uLen = sizeof(pDeviceInfo->osinfo.org);
		if (RegQueryValueEx(hKey, L"RegisteredOrganization", NULL, NULL, (PBYTE)pDeviceInfo->osinfo.org, &uLen) != ERROR_SUCCESS)
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
	if (NetUserGetInfo(NULL, pDeviceInfo->userinfo.username, 1, &pUserInfo) == NERR_Success)
		pDeviceInfo->userinfo.priv = ((PUSER_INFO_1)pUserInfo)->usri1_priv;
	else
		pDeviceInfo->userinfo.priv = 0;

	if (NetUserGetInfo(NULL, pDeviceInfo->userinfo.username, 23, &pUserInfo) == NERR_Success)
	{
		PWCHAR pSidStr = NULL;
		wcsncpy_s(pDeviceInfo->userinfo.fullname, sizeof(pDeviceInfo->userinfo.fullname) / sizeof(WCHAR), ((PUSER_INFO_23)pUserInfo)->usri23_full_name, _TRUNCATE);
		if (!ConvertSidToStringSid(((PUSER_INFO_23)pUserInfo)->usri23_user_sid, &pSidStr))
			pDeviceInfo->userinfo.sid[0] = L'\0';
		else
			wcsncpy_s(pDeviceInfo->userinfo.sid, sizeof(pDeviceInfo->userinfo.sid) / sizeof(WCHAR), pSidStr, _TRUNCATE);
	}
	else
	{
		pDeviceInfo->userinfo.fullname[0] = L'\0';
		pDeviceInfo->userinfo.sid[0] = L'\0';
	}
	NetApiBufferFree(pUserInfo);

	
	// locale & timezone
	if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, pDeviceInfo->localinfo.lang, sizeof(pDeviceInfo->localinfo.lang) / sizeof(WCHAR)))
		pDeviceInfo->localinfo.lang[0] = L'\0';
	if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, pDeviceInfo->localinfo.country, sizeof(pDeviceInfo->localinfo.country) / sizeof(WCHAR)))
		pDeviceInfo->localinfo.country[0] = L'\0';

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		pDeviceInfo->localinfo.timebias = 0;
	else
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
	if (GetDiskFreeSpaceEx(wPath, &uDiskFree, &uDiskTotal, NULL))
	{
		pDeviceInfo->diskinfo.disktotal = (ULONG) (uDiskTotal.QuadPart / (1024*1024));
		pDeviceInfo->diskinfo.diskfree = (ULONG) (uDiskFree.QuadPart / (1024*1024));
	}
	else
		pDeviceInfo->diskinfo.disktotal = pDeviceInfo->diskinfo.diskfree = 0;

	free(wPath);
	free(pDeviceInfo);
}

VOID BuildDeviceBuffer()
{

}