#ifndef _CRYPT_H
#define _CRYPT_H

#define PAD_NOPAD 0
#define PAD_PKCS5 1

#define BLOCK_LEN 16

VOID Encrypt(PBYTE pBuffer, ULONG uBuffLen, PBYTE pKey, ULONG uPadding);
VOID Decrypt(PBYTE pBuffer, ULONG uBuffLen, PBYTE pKey);

#endif