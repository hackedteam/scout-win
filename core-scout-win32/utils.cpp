#include <Windows.h>
#include "utils.h"
#include "proto.h"

BOOL ExecQueryGetProp(IWbemServices *pSvc, LPWSTR strQuery, LPWSTR strField,  LPVARIANT lpVar)
{
	BOOL bRet = FALSE;
	IEnumWbemClassObject *pEnum;
	
	BSTR bstrQuery = SysAllocString(strQuery);
	BSTR bstrField = SysAllocString(strField);
	
	WCHAR strWQL[] = { L'W', L'Q', L'L', L'\0' };
	BSTR bWQL = SysAllocString(strWQL);

	HRESULT hr = pSvc->ExecQuery(bWQL, bstrQuery, 0, NULL, &pEnum);
	if (hr == S_OK)
	{
		ULONG uRet;
		IWbemClassObject *apObj;
		hr = pEnum->Next(5000, 1, &apObj, &uRet);
		if (hr == S_OK)
		{
			hr = apObj->Get(bstrField, 0, lpVar, NULL, NULL);
			if (hr == WBEM_S_NO_ERROR)
				bRet = TRUE;

			apObj->Release();
		}
		pEnum->Release();
	}

	SysFreeString(bstrQuery);
	SysFreeString(bstrField);
	SysFreeString(bWQL);
	
	return bRet;
}

BOOL ExecQuerySearchEntryHash(IWbemServices *pSvc, LPWSTR strQuery, LPWSTR strField, LPBYTE pSearchHash, LPVARIANT lpVar)
{
	BOOL bFound = FALSE;
	IEnumWbemClassObject *pEnum;
	WCHAR strWQL[] = { L'W', L'Q', L'L', L'\0' };
	
	BSTR bWQL = SysAllocString(strWQL);
	BSTR bstrQuery = SysAllocString(strQuery);
	BSTR bstrField = SysAllocString(strField);

	HRESULT hr = pSvc->ExecQuery(bWQL, bstrQuery, 0, NULL, &pEnum);
	if (hr == S_OK)
	{
		ULONG uRet;
		IWbemClassObject *apObj;

		while (pEnum->Next(5000, 1, &apObj, &uRet) == S_OK)
		{
			hr = apObj->Get(bstrField, 0, lpVar, NULL, NULL);
			if (hr != WBEM_S_NO_ERROR ||  lpVar->vt != VT_BSTR)
				continue;

			BYTE pSha1Buffer[20];
			CalculateSHA1(pSha1Buffer, (LPBYTE)lpVar->bstrVal, 21*sizeof(WCHAR));
			if (!memcmp(pSha1Buffer, pSearchHash, 20))
				bFound = TRUE;

			apObj->Release();
		}

		pEnum->Release();
	}

	SysFreeString(bstrQuery);
	SysFreeString(bstrField);
	SysFreeString(bWQL);

	return bFound;
}


LPBYTE GetRandomData(__in DWORD dwBuffLen)
{
	LPBYTE lpBuffer = (LPBYTE) malloc(dwBuffLen);
	AppendRandomData(lpBuffer, dwBuffLen);
	return lpBuffer;
}

VOID AppendRandomData(PBYTE pBuffer, DWORD uBuffLen)
{
	DWORD i;
	static BOOL uFirstTime = TRUE;

	if (uFirstTime) {
		srand(GetTickCount());
		uFirstTime = FALSE;
	}

	for (i=0; i<uBuffLen; i++)
		pBuffer[i] = rand();
}