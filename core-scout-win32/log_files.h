#ifndef _LOG_FILES_H
#define _LOG_FILES_H

//HANDLE CreateLogFile(ULONG uEvidenceType, PBYTE pAdditionalHeader, ULONG uAdditionalLen);
HANDLE CreateLogFile(ULONG uEvidenceType, PBYTE pAdditionalHeader, ULONG uAdditionalLen, BOOL bCreateFile, PBYTE *pOutBuffer, PULONG uOutLen);
PBYTE CreateLogHeader(ULONG uEvidenceType, PBYTE pAdditionalData, ULONG uAdditionalLen, PULONG uOutLen);
BOOL WriteLogFile(HANDLE hFile, PBYTE pBuffer, ULONG uBuffLen);
VOID ProcessEvidenceFiles();

//
// Struttura dei log file
//
// C'e' una dword in chiaro che indica: sizeof(LogStruct) + uDeviceIdLen + uUserIdLen + uSourceIdLen + uAdditionalData
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

#endif