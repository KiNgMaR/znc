/*
 * Copyright (C) 2004-2013 ZNC-MSVC
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

#include "znc_msvc.h"
#include "win_utils.h"
#include <stdio.h>
#include <fcntl.h>
#ifdef HAVE_LIBSSL
#include <openssl/crypto.h>
#endif

const char *ZNC_VERSION_EXTRA =
#ifdef _WIN64
	"-Win-x64";
#else
	"-Win";
#endif

bool CZNCWin32Helpers::ms_serviceMode = false;


int CZNCWin32Helpers::RuntimeStartUp()
{
	CWinUtils::EnforceDEP();
	CWinUtils::HardenHeap();
	CWinUtils::HardenDLLSearchPath();

	// this prevents open()/read() and friends from stripping \r from files... simply
	// adding _O_BINARY to the modes doesn't seem to be enough for some reason...
	// if we don't do this, Template.cpp will break because it uses string.size() for
	// file position calculations.
	if (_set_fmode(_O_BINARY) != 0) {
		return -1;
	}

	if (!setlocale(LC_CTYPE, "C")) {
		return -2;
	}

	icu::Locale uloc = icu::Locale::createCanonical("C");
	UErrorCode ec = U_ZERO_ERROR;
	icu::Locale::setDefault(uloc, ec);

	if (ec != U_ZERO_ERROR) {
		return -3;
	}

#ifdef HAVE_LIBSSL
	if (!CRYPTO_malloc_init()) {
		return -9;
	}
#endif

	::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	return 0;
}
