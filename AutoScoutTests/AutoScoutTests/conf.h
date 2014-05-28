BOOL LoadConf();
BOOL DecryptConf();
BOOL GetSyncIp(LPSTR strIp);
BOOL ConfIsModuleEnabled(LPWSTR strModule);
DWORD ConfGetRepeat(LPWSTR strModule);
DWORD ConfGetIter(LPWSTR strModule);
BOOL NewConf(LPBYTE lpBuffer, BOOL bPause, BOOL bSave);