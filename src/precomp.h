//////////////////////////////////////////////////////////////////////////////
//
//  precomp.h
//
//  Precompiled headers.
//
//----------------------------------------------------------------------------
//
//  Copyright 2020 by State University of Catatonia and other Contributors
//
//  This file is provided under a "BSD 3-Clause" open source license.
//  The full text of the license is provided in the "LICENSE.txt" file.
//
//  SPDX-License-Identifier: BSD-3-Clause
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// Requires Windows 10.
#define WINVER		    0x0A00
#define _WIN32_WINNT	WINVER

// Not currently using GDI+ (but may need it later).
#define USING_GDIPLUS FALSE

// Make sure we're building in UNICODE mode.
#ifndef UNICODE
  #define UNICODE
#endif
#ifndef _UNICODE
  #define _UNICODE
#endif

// MSVCRT needs to STFU.
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

// Enable MSVCRT support for memory leak detection.
#ifdef _DEBUG
  #define _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>
#endif

//----------------------------------------------------------------------------
//  Windows headers
//----------------------------------------------------------------------------

#include <windows.h>
#include <strsafe.h>
#include <psapi.h>

//----------------------------------------------------------------------------
//  GDI+ headers
//----------------------------------------------------------------------------

#if USING_GDIPLUS

  #define GDIPVER 0x0110
  #include <gdiplus.h>

  namespace GP = Gdiplus;

  #pragma comment(lib, "gdiplus.lib")

#endif

//----------------------------------------------------------------------------
//  ATL/WTL headers
//----------------------------------------------------------------------------

// Prevent atlmisc.h from including atlstr.h.
#define _WTL_NO_COMPATIBILITY_INCLUDES

/*ATL*/ #include <atlbase.h>
/*ATL*/ // #include <atlstr.h>  // We're using std::wstring instead of CString.
/*WTL*/ #include <atlapp.h>

extern CAppModule _Module;

/*ATL*/ #include <atlwin.h>
/*ATL*/ #include <atltypes.h>
/*WTL*/ #include <atlframe.h>
/*WTL*/ #include <atlcrack.h>
/*WTL*/ #include <atldlgs.h>
/*WTL*/ #include <atlctrls.h>
/*WTL*/ #include <atlctrlx.h>
/*WTL*/ #include <atlmisc.h>

//----------------------------------------------------------------------------
//  C Runtime and C++ STL headers
//----------------------------------------------------------------------------

// Disable Windows macros so we can use std::min/std::max.
#undef min
#undef max

#include <stdio.h>
#include <stdarg.h>

#include <vector>
#include <array>
#include <string>
#include <random>
#include <algorithm>
#include <functional>
#include <filesystem>

namespace fs = std::filesystem;

//----------------------------------------------------------------------------
//  DebugPrint
//----------------------------------------------------------------------------

// ATLTRACE2 adds the source code filename, line number, and trace category
// to the output.  This is fugly and verbose.  Great when you need those
// details, but just a PITA the other 99% of the time.

#define USE_ATLTRACE2_FOR_DEBUGPRINT 0

#ifdef _DEBUG
  #if USE_ATLTRACE_FOR_DEBUGPRINT
    #define DebugPrint(format, ...) ATLTRACE2(atlTraceGeneral, 0, format, ##__VA_ARGS__)
  #else
    void __cdecl DebugPrint(const char* pszFormat, ...);
    void __cdecl DebugPrint(const WCHAR* pszFormat, ...);
  #endif
#else
  #define DebugPrint(format, ...) do {} while (0)
#endif

//----------------------------------------------------------------------------
//  Memory leak detection for C++ new/delete
//----------------------------------------------------------------------------

#ifdef _CRTDBG_MAP_ALLOC

  // Use debug version of operator new for better memory leak reporting.
  // Only want this for our own application code, so need to define it
  // after including the Win32/ATL/WTL/GDI+ headers.

  #undef new
  #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
  #define new DEBUG_NEW

#endif

//----------------------------------------------------------------------------
//  Common Controls manifest dependency
//----------------------------------------------------------------------------

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
