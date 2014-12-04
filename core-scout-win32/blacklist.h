
#ifndef _BLACKLIST_H__
#define _BLACKLIST_H__

#include <Windows.h>
#include "utils.h"
#include <Shlwapi.h>

//black list 

#define MAX_BLACKLIST_ITEMS 10

typedef enum
{
	OS_UNKNOWN,
	OS_ALL,
	OS_XP,
	OS_VISTA,
	OS_7,
	OS_8,	
	OS_10
} OS_TYPE;

typedef struct
{
	WCHAR	Name[64];
	OS_TYPE OS;
}BLACKLIST_ITEM, *PBLACKLIST_ITEM;


#ifdef _BLACKLIST_ITEMS_
BLACKLIST_ITEM g_BlackList[MAX_BLACKLIST_ITEMS] = 
{
	{L"Kaspersky",	  OS_XP},
	{L"Касперского" , OS_XP}
};
#endif

LPWSTR	BL_GetAppList();
BOOL	BL_CheckList();
OS_TYPE BL_GetOSType(LPWSTR lpwsOSName);
OS_TYPE BL_GetOSVersion(LPWSTR *lpwsOSName);



#endif