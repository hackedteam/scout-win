#define MAX_POSITION_QUEUE 500

typedef struct 
{
	DWORD dwSize;
	LPBYTE lpBuffer;
} POSITION_LOGS, *LPPOSITION_LOGS;

extern POSITION_LOGS lpPositionLogs[MAX_POSITION_QUEUE];

VOID PositionMain();