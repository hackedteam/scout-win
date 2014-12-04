#include <Wbemidl.h>
#include <comdef.h> 
#include <Shobjidl.h>
#pragma comment(lib, "wbemuuid.lib")

typedef PIMAGE_NT_HEADERS (WINAPI *CHECKSUMMAPPEDFILE) ( _In_   PVOID BaseAddress,  _In_   DWORD FileLength,  _Out_  PDWORD HeaderSum,  _Out_  PDWORD CheckSum);

BOOL ExecQuerySearchEntryHash(IWbemServices *pSvc, LPWSTR strQuery, LPWSTR strField, LPBYTE pSearchHash, LPVARIANT lpVar);
BOOL ExecQueryGetProp(IWbemServices *pSvc, LPWSTR strQuery, LPWSTR strField,  LPVARIANT lpVar);

LPBYTE GetRandomData(__in DWORD dwBuffLen);
VOID AppendRandomData(PBYTE pBuffer, DWORD uBuffLen);

LPBYTE AppendDataInSignedExecutable(LPWSTR lpTargetExecutable, LPBYTE lpPadData, DWORD dwPadDataSize, PDWORD dwFatExecutableSize);
LPBYTE AppendDataInSignedExecutable(HANDLE hExecutable, LPBYTE lpPadData, DWORD dwPadDataSize, PDWORD dwFatExecutableSize);
DWORD ComputePEChecksum(LPBYTE lpMz, DWORD dwBufferSize);

PWCHAR	GetApplicationList(BOOL bX64View);
VOID	IsX64System(PBOOL bIsWow64, PBOOL bIsx64OS);