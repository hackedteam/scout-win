#include "zmem.h"
#include "debug.h"

LPVOID zalloc(__in DWORD dwSize)
{
	LPBYTE pMem = (LPBYTE) malloc(dwSize);
	RtlSecureZeroMemory(pMem, dwSize);
#ifdef _DEBUG
	//OutputDebug(L"[*] Alloc => %08x\n", pMem);
#endif
	return(pMem);
}

VOID zfree(__in LPVOID pMem)
{ 
#ifdef _DEBUG
	//OutputDebug(L"[*] Free => %08x\n", pMem);
#endif

	if (pMem) 
		free(pMem); 
}

