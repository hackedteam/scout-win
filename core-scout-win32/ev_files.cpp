#include <Windows.h>

#include "binpatched_vars.h"

int __cdecl compare(const void *first, const void *second)
{
	return CompareFileTime(&((PWIN32_FIND_DATA)first)->ftCreationTime, &((PWIN32_FIND_DATA)second)->ftCreationTime);
}

VOID bubusettete()
{
	HANDLE hFind;
	LPWSTR pTempPath, pFindArgument;
	WIN32_FIND_DATA pFindData;

	pTempPath = (LPWSTR)malloc(32767 * sizeof(WCHAR));
	GetEnvironmentVariable(L"TMP", pTempPath, 32767 * sizeof(WCHAR));

	pFindArgument = (LPWSTR)malloc(32767 * sizeof(WCHAR));
	wsprintf(pFindArgument, L"%s\\%S*tmp", pTempPath, BACKDOOR_ID + 4);
	free(pTempPath);
		

	hFind = FindFirstFile(pFindArgument, &pFindData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		free(pTempPath);
		return;
	}

	ULONG x = 0;
	do
		if (pFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		else
			x++;
	while (FindNextFile(hFind, &pFindData) != 0);
	FindClose(hFind);

	ULONG i=0;
	hFind = FindFirstFile(pFindArgument, &pFindData);
	PWIN32_FIND_DATA pFindDataArray = (PWIN32_FIND_DATA)malloc(sizeof(WIN32_FIND_DATA)*x);	
	do
	{
		if (pFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		memcpy(pFindDataArray + i, &pFindData, sizeof(WIN32_FIND_DATA));
		i++;

		if (i>= 4096)
			continue;
	}
	while (FindNextFile(hFind, &pFindData) != 0);
	FindClose(hFind);

	qsort(pFindDataArray, i, sizeof(WIN32_FIND_DATA), compare);
	for(x=0; x<=i; x++)
	{
		// do stuff
	}

	free(pFindArgument);
}