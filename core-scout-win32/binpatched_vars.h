#ifndef _BINPATCHED_H
#define _BINPATCHED_H

#define KEY_LEN 16

#ifdef _DEBUG
#define CLIENT_KEY			"4yeN5zu0+il3Jtcb5a1sBcAdjYFcsD9z"	// per server (auth)
#define ENCRYPTION_KEY		"3j9WmmDgBqyU270FTid3719g64bP4s52"	// for log
#define ENCRYPTION_KEY_CONF "uX-o0BOIkiyOyVXH4L3FYhbai-CvMU-_"	// for conf e sha1
						     
#define BACKDOOR_ID			"RCS_0000001167"					// castore "poveri"
#define SYNC_INTERVAL		5000
#define SYNC_SERVER			"rcs-castore"
#define DEMO_TAG			"hxVtdxJ/Z8LvK3ULSnKRUmLE"
#else
#define CLIENT_KEY			"ANgs9oGFnEL_vxTxe9eIyBx5lZxfd6QZ"
#define ENCRYPTION_KEY		"WfClq6HxbSaOuJGaH5kWXr7dQgjYNSNg"
#define ENCRYPTION_KEY_CONF	"6uo_E0S4w_FD0j9NEhW2UpFw9rwy90LY"
#define BACKDOOR_ID			"EMp7Ca7-fpOBIr"
#define SYNC_INTERVAL		5000
#define DEMO_TAG			"Pg-WaVyPzMMMMmGbhP6qAigT"
#define SYNC_SERVER			"SYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNCSYNC"
#endif


#endif