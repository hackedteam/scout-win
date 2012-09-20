#include <Windows.h>


#include "main.h"

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
	MessageBox(NULL, L"Scouting :)", L"Scouting :)", 0);

	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SyncThreadFunction, NULL, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
}