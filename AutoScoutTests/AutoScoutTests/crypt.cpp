#include <Windows.h>
#include "crypt.h"
#include "aes.h"
#include "sha1.h"

VOID CalculateSHA1(__out PBYTE pSha1Buffer, __in PBYTE pBuffer, __in ULONG uBufflen)
{
	SHA1Context pSha1Context;

	SHA1Reset(&pSha1Context);
	SHA1Input(&pSha1Context, pBuffer, uBufflen);
	SHA1Result(&pSha1Context);

	for(ULONG x=0; x<5; x++)
		((PULONG)pSha1Buffer)[x] = ntohl(pSha1Context.Message_Digest[x]);
}

VOID Encrypt(__inout PBYTE pBuffer, __in ULONG uBuffLen, __in PBYTE pKey, __in ULONG uPadding)
{
	BYTE pInitVector[16];
	aes_context pAesContext;

	memset(pInitVector, 0x0, BLOCK_LEN);
	aes_set_key(&pAesContext, pKey, BLOCK_LEN*8);

	if (uPadding == PAD_NOPAD)
		aes_cbc_encrypt(&pAesContext, pInitVector, pBuffer, pBuffer, uBuffLen);
	else if (uPadding == PAD_PKCS5)
		aes_cbc_encrypt_pkcs5(&pAesContext, pInitVector, pBuffer, pBuffer, uBuffLen);
#ifdef _DEBUG
	else
		OutputDebugString(L"Unknown padding\n");
#endif
}

VOID Decrypt(__inout PBYTE pBuffer, __in ULONG uBuffLen, __in PBYTE pKey)
{
	BYTE pInitVector[16];
	aes_context pAesContext;

	memset(pInitVector, 0x0, BLOCK_LEN);
	aes_set_key(&pAesContext, pKey, BLOCK_LEN*8);
	aes_cbc_decrypt(&pAesContext, pInitVector, pBuffer, pBuffer, uBuffLen);
}