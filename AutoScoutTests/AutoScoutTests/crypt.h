#include <Windows.h>

#ifndef _CRYPT_H
#define _CRYPT_H

#define PAD_NOPAD 0
#define PAD_PKCS5 1

#define BLOCK_LEN 16

VOID CalculateSHA1(__out PBYTE pSha1Buffer, __in PBYTE pBuffer, __in ULONG uBufflen);
VOID Encrypt(__inout PBYTE pBuffer, __in ULONG uBuffLen, __in PBYTE pKey, __in ULONG uPadding);
VOID Decrypt(__inout PBYTE pBuffer, __in ULONG uBuffLen, __in PBYTE pKey);

#endif