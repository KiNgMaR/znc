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

#pragma once

// keep HINSTANCE around
extern HINSTANCE g_hInstance;
// obtained through RegisterWindowMessage:
extern UINT WM_TASKBARCREATED;

#include "win_utils.h"

class CResourceBitmap
{
public:
	CResourceBitmap(int resId);
	virtual ~CResourceBitmap();

	std::shared_ptr<Gdiplus::Bitmap> GetBmp() const { return m_bmp; }
	bool Loaded() const { return !!m_bmp; }
protected:
	std::shared_ptr<Gdiplus::Bitmap> m_bmp;
	HGLOBAL m_hGlobal;
};

#define WM_TRAYICONEVENT (WM_USER + 9)

#define WMC_CLOSE_TRAY (WM_APP + 1)
#define WMC_SHOW_CONTROL_WIN (WM_APP + 2)
