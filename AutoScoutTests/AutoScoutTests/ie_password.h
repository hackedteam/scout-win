#ifndef _IE_PASSWORD
#define _IE_PASSWORD
#include <Windows.h>


DWORD GetIEUrlHistory(LPWSTR *strUrlHistory);
VOID ParseIEBlob(DATA_BLOB *lpDataBlob, LPWSTR strUrl, LPBYTE *lpBuffer, LPDWORD dwBuffSize);
VOID DumpIE7Passwords(LPBYTE *lpBuffer, LPDWORD dwBuffSize);
VOID DumpIEVault(LPBYTE *lpBuffer, LPDWORD dwBuffSize);
VOID DumpIEPasswords(LPBYTE *lpBuffer, LPDWORD dwBuffSize);
BOOL IEPasswordResolveApi();

typedef struct _VAULT_ENTRY{
	DWORD dwSchemaElemId;
	DWORD dwUnk1;
	DWORD dwUnk2;
	DWORD dwUnk3;
	LPWSTR strName;
} VAULT_ENTRY, *LPVAULT_ENTRY;

typedef struct _VAULT_CRED{
	GUID gSchema;
	LPWSTR strProgram;
	LPVAULT_ENTRY lpResource;
	LPVAULT_ENTRY lpUser;
	LPVAULT_ENTRY lpPassword;
	BYTE pUnk[24];
} VAULT_CRED, *LPVAULT_CRED;

typedef DWORD (WINAPI *VaultOpenVault_t)(_GUID *pVaultId, DWORD dwFlags, LPVOID *pVaultHandle); 
typedef DWORD (WINAPI *VaultEnumerateItems_t)(LPVOID lpVaultHandle, DWORD dwFlags, LPDWORD lpCount, LPVAULT_CRED *pVaultCred); 
typedef DWORD (WINAPI *VaultGetItem_t)(LPVOID pVaultHandle, _GUID *pSchemaId, LPVAULT_ENTRY pResource, LPVAULT_ENTRY pIdentity, LPVAULT_ENTRY pPackageSid, HWND__ *hwndOwner, DWORD dwFlags, LPVAULT_CRED *ppItem);
typedef DWORD (WINAPI *VaultCloseVault_t)(LPVOID *pVaultHandle);
typedef VOID (WINAPI *VaultFree_t)(LPVOID pMemory);

typedef BOOL (WINAPI *CryptUnprotectData_t)(DATA_BLOB *pDataIn, LPWSTR *ppszDataDescr, DATA_BLOB *pOptionalEntropy, LPVOID pvReserved, CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct, DWORD dwFlags, DATA_BLOB *pDataOut);
typedef BOOL (WINAPI *CredEnumerate_t)(LPWSTR strFilter, DWORD dwFlags, LPDWORD dwCount, PCREDENTIAL **Credentials);
typedef VOID (WINAPI *CredFree_t)(LPVOID Buffer);
#endif