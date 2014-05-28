#ifndef _FACEBOOK_H
#define _FACEBOOK_H

#include <Windows.h>

DWORD FacebookMessageHandler(LPSTR);
DWORD FacebookContactHandler(LPSTR strCookie);

#define FB_USER_ID "\"id\":\""
#define FB_THREAD_LIST_ID "\"threads\":[{"
#define FB_THREAD_LIST_END "\"ordered_threadlists\":"
#define FB_THREAD_IDENTIFIER "\\/messages\\/?action=read&amp;tid="
#define FB_THREAD_IDENTIFIER_V2 "\"thread_id\":\""
#define FB_THREAD_AUTHOR_IDENTIFIER "class=\\\"authors\\\">"
#define FB_THREAD_AUTHOR_IDENTIFIER_V2 "\"DocumentTitle.set(\\\""
#define FB_THREAD_STATUS_IDENTIFIER "class=\\\"threadRow noDraft"
#define FB_THREAD_STATUS_IDENTIFIER_V2 "\"unread_count\":"
#define FB_MESSAGE_TSTAMP_IDENTIFIER "data-utime=\\\""
#define FB_MESSAGE_TSTAMP_IDENTIFIER_V2 "\"timestamp\":"
#define FB_MESSAGE_BODY_IDENTIFIER "div class=\\\"content noh\\\" id=\\\""
#define FB_MESSAGE_AUTHOR_IDENTIFIER "\\u003C\\/a>\\u003C\\/strong>"
#define FB_MESSAGE_SCREEN_NAME_ID "\"id\":\"%s\",\"name\":\""
#define FB_NEW_LINE "\\u003Cbr \\/> "
#define FB_POST_FORM_ID "post_form_id\":\""
#define FB_PEER_ID_IDENTIFIER "\"fbid:"
#define FB_DTSG_ID "fb_dtsg\":\""

#define FACEBOOK_THREAD_LIMIT 15

#endif