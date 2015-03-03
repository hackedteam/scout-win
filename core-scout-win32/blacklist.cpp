#define _BLACKLIST_ITEMS_
#include "blacklist.h"
#include "zmem.h"


//determine the os type
OS_TYPE BL_GetOSType(LPWSTR lpwsOSName)
{
	if(lpwsOSName == NULL)
		return OS_UNKNOWN;

	if(StrStrI(lpwsOSName, L"Windows XP"))
		return OS_XP;
	if(StrStrI(lpwsOSName, L"Windows Vista"))
		return OS_VISTA;
	if(StrStrI(lpwsOSName, L"Windows 7"))
		return OS_7;
	if(StrStrI(lpwsOSName, L"Windows 8"))
		return OS_8;
	if(StrStrI(lpwsOSName, L"Windows 10"))
		return OS_10;

	return OS_UNKNOWN;
}


//get the os version
OS_TYPE BL_GetOSVersion(LPWSTR *lpwsOSName)
{
	IWbemLocator  *pLoc=NULL;
	IWbemServices *pSvc=NULL;
	HRESULT		   hRes;
	VARIANT		   vVariant;
	BOOL		   bComAvailable=FALSE;

	zfree(*lpwsOSName);

	//initialize com obj
	hRes = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
	if((hRes == S_OK) && (pLoc != NULL))
		bComAvailable = TRUE;
	
	if(bComAvailable)
	{		
		WCHAR strRootCIM[] = { L'R', L'O', L'O', L'T', L'\\', L'C', L'I', L'M', L'V', L'2', L'\0' };
		BSTR bRootCIM	   = SysAllocString(strRootCIM);

		//connection to WMI
		if(pLoc->ConnectServer(bRootCIM, NULL, NULL, 0, NULL, 0, 0, &pSvc) == WBEM_S_NO_ERROR)
		{
			//set authentication information
			if(CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE) != S_OK)
			{
				#ifdef _DEBUG
					OutputDebugString(L"[!!] CoSetProxyBlanket!\n");
				#endif

				bComAvailable = FALSE;
			}

			//query the wmi to get os info
			WCHAR pSelect1[] = { L'S', L'E', L'L', L'E', L'C', L'T', L' ', L'*', L' ', L'F', L'R', L'O', L'M', L' ', L'W', L'i', L'n', L'3', L'2', L'_', L'O', L'p', L'e', L'r', L'a', L't', L'i', L'n', L'g', L'S', L'y', L's', L't', L'e', L'm', L'\0' };
			
			VariantInit(&vVariant);
			if(ExecQueryGetProp(pSvc, pSelect1, L"Caption", &vVariant) && vVariant.vt == VT_BSTR)
			{
				//alloc memory for os version string
				DWORD dwSize = wcslen(vVariant.bstrVal) + 1;

				*lpwsOSName = (LPWSTR)malloc(dwSize * sizeof(WCHAR));
				if(*lpwsOSName != NULL)				
					wcscpy_s(*lpwsOSName, dwSize, vVariant.bstrVal);				
				else
					bComAvailable = FALSE;
			}
			VariantClear(&vVariant);
		}
		else
			bComAvailable = FALSE;
		
		SysFreeString(bRootCIM);

		//release com objs
		if(pSvc)
			pSvc->Release();
		if(pLoc)
			pLoc->Release();

		pSvc = NULL;
		pLoc = NULL;
	}

	//return the os type
	return BL_GetOSType(*lpwsOSName);
}


//return the list of the installed applications (get the 32 and 64 bit list)
LPWSTR BL_GetAppList()
{
	LPWSTR	lpwsAppList=NULL, lpws32=NULL, lpws64=NULL;
	BOOL	bIsWow64, bIsOS64;

	//verify if the os is 64 bit
	IsX64System(&bIsWow64, &bIsOS64);

	//get the 32 bit list
	lpws32 = GetApplicationList(FALSE);
	if(bIsOS64)
	{
		//get the 64 bit application list
		lpws64 = GetApplicationList(TRUE);
		
		DWORD dwSize = 0;

		if(lpws32 != NULL)
			dwSize += wcslen(lpws32);
		if(lpws64 != NULL)
			dwSize += wcslen(lpws64);

		//alloc memory for the complete list
		if(dwSize > 0)
		{
			dwSize += 1; //add 1 char for the \0
			lpwsAppList = (LPWSTR)malloc(dwSize * sizeof(WCHAR));
			if(lpwsAppList != NULL)
			{
				if(lpws32)						
				{
					//32 bit list
					wcscpy_s(lpwsAppList, dwSize, lpws32);
					if(lpws64)
						wcscat_s(lpwsAppList, dwSize, lpws64);
				}
				else
				{
					//64 bit list
					if(lpws64)
						wcscpy_s(lpwsAppList, dwSize, lpws64);
				}			
			}
		}

		zfree(lpws32);
		zfree(lpws64);
	}
	else
	{
		lpwsAppList = lpws32;		
	}

	return lpwsAppList;
}



//check the software list for blacklisted programs and OS
//return true if no blacklisted software is found
BOOL BL_CheckList()
{
	LPWSTR lpwsOSVer=NULL, lpwsAppList=NULL;
	OS_TYPE OsType;
	DWORD i;
	BOOL bListOK=TRUE;

	//get os version
	OsType = BL_GetOSVersion(&lpwsOSVer);
	if(OsType == OS_UNKNOWN)
	{
		zfree(lpwsOSVer);	
		return TRUE;
	}

	//get the program list
	lpwsAppList = BL_GetAppList();
	if(lpwsAppList == NULL)
	{
		zfree(lpwsOSVer);		
		return TRUE;
	}

	#ifdef _DEBUG
		OutputDebugString(lpwsAppList);
	#endif

	//loop through the list
	for(i=0; i<MAX_BLACKLIST_ITEMS; i++)
	{
		if(g_BlackList[i].OS == OS_UNKNOWN)
			break;

		//verify the os
		if((g_BlackList[i].OS != OsType) && (g_BlackList[i].OS != OS_ALL))
			continue;

		//check the program name
		if(StrStrI(lpwsAppList, g_BlackList[i].Name))		
		{
			bListOK = FALSE;
			break;
		}
	}

	zfree(lpwsOSVer);
	zfree(lpwsAppList);

	return bListOK;
}
