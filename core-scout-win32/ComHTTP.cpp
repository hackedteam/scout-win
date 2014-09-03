/*
#include "ComHTTP.h"

int ComHTTPConnect()
{
	HRESULT hr;
	IWebBrowser2* pWebBrowser = NULL;

	hr = CoCreateInstance(CLSID_InternetExplorer, NULL, 
	  CLSCTX_SERVER, IID_IWebBrowser2, (LPVOID*)&pWebBrowser);
    
	if (SUCCEEDED (hr) && (pWebBrowser != NULL))
	{
		pWebBrowser->put_Visible (VARIANT_TRUE);
		// OK, we created a new IE Window and made it visible
		// You can use pWebBrowser object to do whatever you want to do!
	}
	else
	{
		// Failed to create a new IE Window.
		// Check out pWebBrowser object and
		// if it is not NULL (should never happen), the release it!
		if (pWebBrowser)
			pWebBrowser->Release ();
	}
}
*/