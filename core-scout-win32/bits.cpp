#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <bits.h>
using namespace std;


VOID BitTransfer(PWCHAR pSource, PWCHAR pDest)
{
	IBackgroundCopyManager* g_XferManager = NULL;  
	HRESULT hr = S_OK;
	HANDLE hTimer = NULL;

	//hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (SUCCEEDED(hr))
	{
		//MessageBox(NULL, L"HRESULT succeeded", L"BUBU", 0);
		hr = CoCreateInstance(__uuidof(BackgroundCopyManager), NULL,
			CLSCTX_LOCAL_SERVER,
			__uuidof(IBackgroundCopyManager),
			(void**) &g_XferManager);
		if (SUCCEEDED(hr))
		{
			//MessageBox(NULL, L"Connected to the BITS service", L"BUBU", 0);
		}
		else 
			return;
	}
	else
		return;

	GUID JobId;
	IBackgroundCopyJob* pJob = NULL;
	hr = g_XferManager->CreateJob(L"WindowsUpdateReaver", BG_JOB_TYPE_DOWNLOAD, &JobId, &pJob);
	if (SUCCEEDED(hr))
	{
		//MessageBox(NULL, L"Job created", L"BUBU", 0);
		hr = pJob->AddFile(pSource, pDest);
		
		if (SUCCEEDED(hr))
		{
			//MessageBox(NULL, L"ile added to job", L"BUBU", 0);
		}
		else
			goto cleanup;
	}
	else
		return;

	HRESULT Resume();
	//MessageBox(NULL, L"Started", L"BUBU", 0);

	BG_JOB_STATE State;
	//HANDLE hTimer = NULL;
	LARGE_INTEGER liDueTime;

	liDueTime.QuadPart = -10000000;  //Poll every 1 second
	hTimer = CreateWaitableTimer(NULL, FALSE, L"WindowsUpdateTimer");
	SetWaitableTimer(hTimer, &liDueTime, 1000, NULL, NULL, 0);

	do
	{
		WaitForSingleObject(hTimer, INFINITE);

		//Use JobStates[State] to set the window text in a user interface.
		hr = pJob->GetState(&State);
		if (FAILED(hr))
		{
			goto cleanup;
			//Handle error
			//MessageBox(NULL, L"FAIL", L"BUBU", 0);
		}

		if (BG_JOB_STATE_TRANSFERRED == State){
			//Call pJob->Complete(); to acknowledge that the transfer is complete
			//and make the file available to the client.
			HRESULT Complete();
			//MessageBox(NULL, L"Job completed", L"BUBU", 0);
		}
		else if (BG_JOB_STATE_ERROR == State || BG_JOB_STATE_TRANSIENT_ERROR == State){
			//Call pJob->GetError(&pError); to retrieve an IBackgroundCopyError interface 
			//pointer which you use to determine the cause of the error.
			//printf("ERROR!\n");
		}
		else if (BG_JOB_STATE_TRANSFERRING == State){
			//Call pJob->GetProgress(&Progress); to determine the number of bytes 
			//and files transferred.
			//MessageBox(NULL, L"Transferring", L"BUBU", 0);
		}
		else if(BG_JOB_STATE_QUEUED == State){
			//MessageBox(NULL, L"QUEUED", L"BUBU", 0);
		}
		else if(BG_JOB_STATE_CONNECTING == State){
			//MessageBox(NULL, L"CONNECTING", L"BUBU", 0);
		}
		else if(BG_JOB_STATE_SUSPENDED == State){
			//MessageBox(NULL, L"BG_JOB_STATE_SUSPENDED", L"BUBU", 0);
			HRESULT Cancel();
			pJob->Resume();
		}
		else if(BG_JOB_STATE_TRANSIENT_ERROR == State){
			//MessageBox(NULL, L"BG_JOB_STATE_TRANSIENT_ERROR", L"BUBU", 0);
		}
		else if(BG_JOB_STATE_ACKNOWLEDGED == State){
			//MessageBox(NULL, L"BG_JOB_STATE_ACKNOWLEDGED", L"BUBU", 0);
		}
		else if(BG_JOB_STATE_CANCELLED == State){
			//MessageBox(NULL, L"BG_JOB_STATE_CANCELLED", L"BUBU", 0);
		}
	}	
	while (BG_JOB_STATE_TRANSFERRED != State && 
		BG_JOB_STATE_ERROR != State &&
		BG_JOB_STATE_TRANSIENT_ERROR != State);

cleanup:
	if (pJob)
		pJob->Complete();
	if (hTimer)
	{
		CancelWaitableTimer(hTimer);
		CloseHandle(hTimer);
	}

	//MessageBox(NULL, L"DONE", L"BUBU", 0);
	return;
}