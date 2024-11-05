#pragma once

#include <string>

#ifdef _WIN32
// various Window platforms:
// - Multi_Byte character set or UNICODE
// - x86 or x64 architecture
// - MSVC or MINGW compiler
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <tchar.h>
#define swprintf swprintf_s

#ifdef UNICODE
#define tstring wstring
#define tostream wostream
#define tistream wistream
#else
#define tstring string
#define tostream ostream
#define tistream istream
#endif

#ifndef __MINGW32__
#define NOMINMAX
#if _MSC_VER <= 1500
#define nullptr NULL
#define noexcept
#define snprintf sprintf_s
#endif // _MSC_VER <= 1500
#endif
// needs to be included above sql.h for windows
#include <windows.h>

#else
// Non-Windows PLatforms
#define tstring string
#define _tstoi atoi
#define _T
#define tostream ostream
#define tistream istream
#endif
