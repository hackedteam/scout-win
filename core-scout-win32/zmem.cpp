#include "zmem.h"

LPVOID zalloc(__in DWORD dwSize)
{
	LPBYTE pMem = (LPBYTE) malloc(dwSize);
	RtlSecureZeroMemory(pMem, dwSize);

	return(pMem);
}

VOID zfree(__in LPVOID pMem)
{ 

	if (pMem) 
		free(pMem); 
}

