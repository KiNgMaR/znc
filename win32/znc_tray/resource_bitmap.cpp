/*
* Copyright (C) 2012-2014 ZNC-MSVC
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "stdafx.hpp"
#include "znc_tray.hpp"

CResourceBitmap::CResourceBitmap(int resId) :
	m_hGlobal(NULL)
{
	HRSRC hRes = ::FindResource(g_hInstance, MAKEINTRESOURCE(resId), L"PNG");

	if(!hRes)
	{
		return;
	}

	DWORD dwSize = ::SizeofResource(g_hInstance, hRes);
	const void *pResData = ::LockResource(
		::LoadResource(g_hInstance, hRes));

	if(dwSize < 1 || !pResData)
	{
		return;
	}

	HGLOBAL hGlob = ::GlobalAlloc(GMEM_MOVEABLE, dwSize);

	if(hGlob)
	{
		void *pGlobBuf = ::GlobalLock(hGlob);

		if(pGlobBuf)
		{
			memcpy_s(pGlobBuf, dwSize, pResData, dwSize);

			IStream *pStream;
			if(SUCCEEDED(::CreateStreamOnHGlobal(hGlob, FALSE, &pStream)))
			{
				Gdiplus::Bitmap* gdipBmp = Gdiplus::Bitmap::FromStream(pStream);

				// done its duty here, Bitmap probably keeps a ref:
				pStream->Release();

				if(gdipBmp && gdipBmp->GetLastStatus() == Gdiplus::Ok)
				{
					// it worked!
					m_hGlobal = hGlob;
					m_bmp = std::shared_ptr<Gdiplus::Bitmap>(gdipBmp);

					return; // postpone cleanup to destructor! ;)
				}

				delete gdipBmp;
			}

			::GlobalUnlock(pGlobBuf);
		}

		::GlobalFree(hGlob);
	}
}

CResourceBitmap::~CResourceBitmap()
{
	m_bmp.reset();

	if(m_hGlobal)
	{
		::GlobalUnlock(m_hGlobal);
		::GlobalFree(m_hGlobal);
	}
}
