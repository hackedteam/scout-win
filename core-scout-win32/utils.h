#include <Wbemidl.h>
#include <comdef.h> 
#include <Shobjidl.h>
#pragma comment(lib, "wbemuuid.lib")

BOOL ExecQuerySearchEntryHash(IWbemServices *pSvc, LPWSTR strQuery, LPWSTR strField, LPBYTE pSearchHash, LPVARIANT lpVar);
BOOL ExecQueryGetProp(IWbemServices *pSvc, LPWSTR strQuery, LPWSTR strField,  LPVARIANT lpVar);

LPBYTE GetRandomData(__in DWORD dwBuffLen);
VOID AppendRandomData(PBYTE pBuffer, DWORD uBuffLen);