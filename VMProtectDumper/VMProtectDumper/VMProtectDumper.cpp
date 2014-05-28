#include <Windows.h>
#include <stdio.h>
#include <tlhelp32.h> 

HANDLE hProcess = NULL;
LPWSTR strOutFileName = NULL;

VOID die(DWORD dwNum)
{
	if (hProcess)
		TerminateProcess(hProcess, 0);
	ExitProcess(0);
}

LPVOID RemoteGetModuleHandle()
{
	HANDLE hSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 pMod;

	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(hProcess));
	if (hSnap == INVALID_HANDLE_VALUE)
		die(printf("[!] CreateToolhelp32Snapshot fail: %08x\n", GetLastError()));

	pMod.dwSize = sizeof(MODULEENTRY32);
	if (!Module32First(hSnap, &pMod))
		die(printf("[!] Module32First fail: %08x\n", GetLastError()));

	return pMod.modBaseAddr;
}

DWORD HandleBreakPoint(LPDEBUG_EVENT pEvent)
{
	IMAGE_DOS_HEADER lpDosHeader;
	IMAGE_NT_HEADERS lpNtHeaders;

	if (Sleep != pEvent->u.Exception.ExceptionRecord.ExceptionAddress)
		return DBG_EXCEPTION_NOT_HANDLED;

	printf("[*] Breakpoint hit!\n");
	LPBYTE lpExe = (LPBYTE) RemoteGetModuleHandle();

	DWORD dwOut;
	LPBYTE lpBuffer = (LPBYTE) malloc(sizeof(IMAGE_DOS_HEADER));
	if (!ReadProcessMemory(hProcess, lpExe, lpBuffer, sizeof(IMAGE_DOS_HEADER), &dwOut))
		die(printf("[!] ReadProcessMemory fail @ %d: %08x\n", __LINE__, GetLastError()));
	memcpy(&lpDosHeader, lpBuffer, sizeof(IMAGE_DOS_HEADER));
	

	lpBuffer = (LPBYTE) realloc(lpBuffer, sizeof(IMAGE_NT_HEADERS));
	if (!ReadProcessMemory(hProcess, lpExe + lpDosHeader.e_lfanew, lpBuffer, sizeof(IMAGE_NT_HEADERS), &dwOut))
		die(printf("[!] ReadProcessMemory fail @ %d: %08x\n", __LINE__, GetLastError()));
	memcpy(&lpNtHeaders, lpBuffer, sizeof(IMAGE_NT_HEADERS));
	
	lpBuffer = (LPBYTE) realloc(lpBuffer, sizeof(IMAGE_SECTION_HEADER) * lpNtHeaders.FileHeader.NumberOfSections);
	if (!ReadProcessMemory(hProcess, lpExe + lpDosHeader.e_lfanew + sizeof(IMAGE_NT_HEADERS), lpBuffer, sizeof(IMAGE_SECTION_HEADER) * lpNtHeaders.FileHeader.NumberOfSections, &dwOut))
		die(printf("[!] ReadProcessMemory fail @ %d: %08x\n", __LINE__, GetLastError()));

	PIMAGE_SECTION_HEADER pSectionHeader = (PIMAGE_SECTION_HEADER) lpBuffer;
	for (DWORD i=0; i<lpNtHeaders.FileHeader.NumberOfSections; i++)
	{
		if (!memcmp(pSectionHeader[i].Name, ".rdata", 7))
		{
			LPBYTE lpData = (LPBYTE) malloc(pSectionHeader[i].Misc.VirtualSize);
			if (!ReadProcessMemory(hProcess, lpExe + pSectionHeader[i].VirtualAddress, lpData, pSectionHeader[i].Misc.VirtualSize, &dwOut))
				die(printf("[!] ReadProcessMemory fail @ %d: %08x\n", __LINE__, GetLastError()));

			HANDLE hOutFile = CreateFile(strOutFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hOutFile == INVALID_HANDLE_VALUE)
				die(printf("[!] Can't create output file %S, got: %08x\n", strOutFileName, GetLastError()));

			if (!WriteFile(hOutFile, lpData, pSectionHeader[i].Misc.VirtualSize, &dwOut, NULL))
				die(printf("[!] Can't write the output file %S, got: %08x\n", strOutFileName, GetLastError()));
			CloseHandle(hOutFile);

			die(printf("[*] All done %d bytes dumped.\n", pSectionHeader[i].Misc.VirtualSize));
		}
	}
}

DWORD HandleException(LPDEBUG_EVENT pEvent)
{
	switch (pEvent->u.Exception.ExceptionRecord.ExceptionCode)
	{
	case EXCEPTION_BREAKPOINT:
		return HandleBreakPoint(pEvent);
		break;
	case EXCEPTION_ACCESS_VIOLATION:
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		break;
	case EXCEPTION_SINGLE_STEP:
		return DBG_EXCEPTION_HANDLED;
		break;
	case DBG_CONTROL_C:
		break;
	default:
		break;
	}
}

VOID HandleDll(LPDEBUG_EVENT pEvent)
{
	if (GetModuleHandle(L"kernel32") != pEvent->u.LoadDll.lpBaseOfDll) // aslr for kernel32 is only at boot (forse non su 8.1 ma chissene)
		return;

	printf("[*] kernel32 loaded on target process @ %08x\n", pEvent->u.LoadDll.lpBaseOfDll);

	DWORD dwOut;
	if (!WriteProcessMemory(hProcess, Sleep, "\xcc", 1, &dwOut))
		die(printf("[!] Cannot write process memory\n"));
}

int main()
{
	int argc;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc < 3)
		die(printf("[!] Need a file name to analyze and a filename for the memory dump\n"));

	strOutFileName = argv[2];
#ifndef _DEBUG
	printf("[!] PRIMA di eseguire questo, guarda il certificato e verifica che la firma sia valida senno' esegui questo programma solo su una VM disposable!!!\n");
	printf("[!] Premi 'k' e invio per andare avanti\n");
	if (getchar() != 'k')
		exit(printf("[*] Exiting..\n"));
#endif

	STARTUPINFO pStartupInfo;
	PROCESS_INFORMATION pProcessInfo;
	SecureZeroMemory(&pStartupInfo, sizeof(STARTUPINFO));
	SecureZeroMemory(&pProcessInfo, sizeof(PROCESS_INFORMATION));
	pStartupInfo.cb = sizeof(STARTUPINFO);

	if (!CreateProcess(argv[1], NULL, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &pStartupInfo, &pProcessInfo))
		exit(printf("[!] Cannot start %S, got => %08x\n", argv[1], GetLastError()));

	printf("[*] Process %d started\n", GetProcessId(hProcess));

	hProcess = pProcessInfo.hProcess;	
	while (1)
	{
		DEBUG_EVENT pEvent;
		if (!WaitForDebugEvent(&pEvent, INFINITE))
			break;

		DWORD dwStatus = DBG_CONTINUE;

		switch (pEvent.dwDebugEventCode)
		{
		case EXCEPTION_DEBUG_EVENT:
			dwStatus = HandleException(&pEvent);
			break;
		case LOAD_DLL_DEBUG_EVENT:
			HandleDll(&pEvent);
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			break; // FIXME
		case CREATE_PROCESS_DEBUG_EVENT:
		case CREATE_THREAD_DEBUG_EVENT:
		case EXIT_THREAD_DEBUG_EVENT:
		case UNLOAD_DLL_DEBUG_EVENT:
		case OUTPUT_DEBUG_STRING_EVENT:
		case RIP_EVENT:
			break;
		default:
			printf("[!] Got unknown debug event => %d\n", pEvent.dwDebugEventCode);
			TerminateProcess(pProcessInfo.hProcess, 0);
			ExitProcess(0);
		}

		ContinueDebugEvent(pEvent.dwProcessId, pEvent.dwThreadId, dwStatus);
	}

	TerminateProcess(pProcessInfo.hProcess, 0);
}