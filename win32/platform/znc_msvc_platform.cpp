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
#include <stdio.h>
#include <time.h>
#include <string>

//
// localtime_r implementation for Windows (VC 2012 + later)!
//
struct tm* localtime_r(const time_t *timep, struct tm *result)
{
	static __declspec(thread) struct tm l_tm;

	if(timep != nullptr && result != nullptr
		&& localtime_s(&l_tm, timep) == 0)
	{
		memcpy(result, &l_tm, sizeof(struct tm));

		return &l_tm;
	}

	return NULL;
}

//
// gmtime_r implementation for Windows (VC 2012 + later)!
//
struct tm* gmtime_r(const time_t *timep, struct tm *result)
{
	static __declspec(thread) struct tm l_tm;

	if(timep != nullptr && result != nullptr
		&& gmtime_s(&l_tm, timep) == 0)
	{
		memcpy(result, &l_tm, sizeof(struct tm));

		return &l_tm;
	}

	return NULL;
}

//
// ctime_r implementation for Windows (VC 2012 + later)!
//
char* ctime_r(const time_t *timep, char *result)
{
	static __declspec(thread) char buf[30] = {0};

	if(timep != nullptr && result != nullptr
		&& ctime_s(buf, sizeof(buf), timep) == 0)
	{
		memcpy(result, buf, strlen(buf) + 1);

		return buf;
	}

	return NULL;
}

/**
 * get password from console
 * Source: http://svn.openvpn.net/projects/openvpn/obsolete/BETA15/openvpn/io.c
 * License: GNU GPL v2
 * (modified to display * for input)
 **/
std::string getpass(const char *prompt)
{
	HANDLE in = ::GetStdHandle(STD_INPUT_HANDLE);
	HANDLE out = ::GetStdHandle(STD_OUTPUT_HANDLE);
	std::string result;
	DWORD count;

	if(in == INVALID_HANDLE_VALUE || out == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}

	// the total size of the buffer must be less than 64K chars
	if(::WriteConsole(out, prompt, DWORD(strlen(prompt)), &count, NULL))
	{
		int istty = (::GetFileType(in) == FILE_TYPE_CHAR);
		DWORD old_mode;
		char c;

		if(istty)
		{
			if(::GetConsoleMode(in, &old_mode))
			{
				::SetConsoleMode(in, old_mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
			}
			else
			{
				istty = 0;
			}
		}

		while(::ReadConsole(in, &c, 1, &count, NULL) && (c != '\r') && (c != '\n'))
		{
			if(c == '\b')
			{
				if(!result.empty())
				{
					::WriteConsole(out, "\b \b", 3, &count, NULL);
					result.erase(result.end() - 1);
				}
			}
			else
			{
				::WriteConsole(out, "*", 1, &count, NULL);
				result += c ;
			}
		}

		::WriteConsole(out, "\r\n", 2, &count, NULL);

		if(istty)
		{
			::SetConsoleMode(in, old_mode);
		}
	}

	return result;
}


/* Microsoft's strftime does not handle user defined error-prone format input very well, and
according to http://msdn.microsoft.com/en-us/library/fe06s4ak%28VS.71%29.aspx is not supposed to,
so we use this wrapper. */
size_t strftime_validating(char *strDest, size_t maxsize, const char *format, const struct tm *timeptr)
{
	std::string l_safeFormat;
	const char* p = format;

	while(*p)
	{
		if(*p == '%')
		{
			bool l_hash = (*(p + 1) == '#');
			char c = *(p + (l_hash ? 2 : 1));

			switch(c)
			{
			case 'a': case 'A': case 'b': case 'B': case 'c': case 'd': case 'H': case 'I': case 'j': case 'm': case 'M':
			case 'p': case 'S': case 'U': case 'w': case 'W': case 'x': case 'X': case 'y': case 'Y': case 'z': case 'Z':
			case '%':
				// formatting code is fine
				if(l_hash)
				{
					l_safeFormat += *p;
					++p;
				}
				// the current loop run will append % (and maybe #), and the next one will do the actual char.
				break;
			default: // replace bad formatting code with itself, escaped, e.g. "%V" --> "%%V"
				l_safeFormat.append("%%");
				++p;
				break;
			}

			// if c == '%', append it in this run to avoid confusion in the next one (p has already been incremented now!)
			if(c == '%')
			{
				l_safeFormat += *p;
				++p;
			}
		}

		if(*p)
		{
			l_safeFormat += *p;
			++p;
		}
	}

	return strftime(strDest, maxsize, l_safeFormat.c_str(), timeptr);
}

//
// setenv implementation for Windows
//
int setenv(const char *envname, const char *envval, int overwrite)
{
	if (getenv(envname) != nullptr && !overwrite)
	{
		return 1;
	}

	return (::SetEnvironmentVariable(envname, envval) ? 1 : 0);
}

//
// unsetenv implementation for Windows
//
int unsetenv(const char *envname)
{
	return (::SetEnvironmentVariable(envname, NULL) ? 1 : 0);
}
