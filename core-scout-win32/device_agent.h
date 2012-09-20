#ifndef _DEVICE_AGENT
#define _DEVICE_AGENT

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


VOID GetDeviceInfo();

#endif


