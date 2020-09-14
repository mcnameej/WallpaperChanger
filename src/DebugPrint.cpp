//////////////////////////////////////////////////////////////////////////////
//
//  DebugPrint.cpp
//
//  Debug spew.
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

#include "precomp.h"

#if defined(_DEBUG) && !defined(DebugPrint)

void __cdecl DebugPrint(const char* format, ...)
{
    char szBuffer[512];

    va_list args;
    va_start(args, format);
    ::StringCbVPrintfA(szBuffer, sizeof(szBuffer), format, args);
    va_end(args);

    ::OutputDebugStringA(szBuffer);
}

void __cdecl DebugPrint(const WCHAR* format, ...)
{
    WCHAR szBuffer[512];

    va_list args;
    va_start(args, format);
    ::StringCbVPrintfW(szBuffer, sizeof(szBuffer), format, args);
    va_end(args);

    ::OutputDebugStringW(szBuffer);
}

#endif
