#include <Windows.h>
#include "crypt.h"
#include "aes_alg.h"



VOID Encrypt(PBYTE pBuffer, ULONG uBuffLen, PBYTE pKey, ULONG uPadding)
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

VOID Decrypt(PBYTE pBuffer, ULONG uBuffLen, PBYTE pKey)
{
	BYTE pInitVector[16];
	aes_context pAesContext;

	memset(pInitVector, 0x0, BLOCK_LEN);
	aes_set_key(&pAesContext, pKey, BLOCK_LEN*8);
	aes_cbc_decrypt(&pAesContext, pInitVector, pBuffer, pBuffer, uBuffLen);
}