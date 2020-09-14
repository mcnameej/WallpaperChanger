//////////////////////////////////////////////////////////////////////////////
//
//  AppBase.h
//
//  CApplicationBase class.
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

#include "Util.h"
#include "Registry.h"

// Base class for WTL applications.
class CApplicationBase
{
    public:

        CApplicationBase(
            LPCWSTR pwszAppShortName = L"Random Application",
            LPCWSTR pwszVendorShortName = L""
        );

        virtual
        ~CApplicationBase();

        // The root of all evil.
        virtual
        int
        WinMain(
            HINSTANCE hInstance,
            LPWSTR pwszCmdLine,
            int nCmdShow
        );

        // Parse the command line.
        virtual
        int
        ParseCmdLine(
            int argc,
            WCHAR** argv
        );

        // Initialize the application.
        virtual
        int
        Initialize();

        // Terminate the application.
        virtual
        void
        Terminate();

        // Create the main window and run message loop until application quits.
        virtual
        int
        Run();

        // Create and show the main application window.
        virtual
        bool
        CreateMainWindow()
        = 0;

        // Delete the main application window object.
        // The window will already be closed and the HWND destroyed.
        virtual
        void
        DeleteMainWindow()
        = 0;

    public:

        // Save main window position and size.
        void
        SaveWindowPlacement(
            HWND hMainWindow
        );

        // Load the saved window position and size.
        bool
        LoadWindowPlacement(
            int& nCmdShow,
            CRect& rcSaved
        );

        // Adjust the data returned by LoadWindowPlacement().
        CRect*
        AdjustWindowPlacement(
            int& nCmdShow,
            CRect& rcSaved,
            CRect& rcCreate
        );

        // Get application settings registry key (HKXX:Software/Vendor/AppName).
        // Best pratice is to use HKCU in prference to HKLM, unless there's an
        // actual need to use global data. Applications must run elevated to
        // write to HKLM.
        CRegistryKey
        GetAppRegistryKey(
            bool UseMachineHive = false  // true == HKLM, false == HKCU
        )
        {
            return CRegistryKey(GetAppRegistryHive(UseMachineHive),
                                GetAppRegistryKeyPath());
        }

        // Get application settings registry key (HKXX:Software/Vendor/AppName/SubKey).
        CRegistryKey
        GetAppRegistrySubKey(
            LPCWSTR pwzSubKey,
            bool UseMachineHive = false  // true == HKLM, false == HKCU
        )
        {
            return CRegistryKey(GetAppRegistryHive(UseMachineHive),
                                GetAppRegistryKeyPath(pwzSubKey));
        }

        // Get directory for our application data files.
        fs::path&
        GetAppDataDirectory();

        // Get path to a file in the application data directory.
        fs::path
        GetAppDataFilePath(
            ConstWString filename
        );

        // Display fatal error message box.
        void
        FatalErrorMessageBox(
            ConstWString errorMessage
        );

        // Get a pointer to the one and only application object.
        static
        CApplicationBase*
        GetApp()
        {
            return s_pAppObject;
        }

    protected:

        // Get registry hive (HKLM or HKCU).
        HKEY
        GetAppRegistryHive(
            bool UseMachineHive
        )
        {
            return UseMachineHive ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
        }

        // Get path to application registry key.
        std::wstring
        GetAppRegistryKeyPath(
            LPCWSTR pwzSubKey = nullptr
        );

        // Return this from ParseCmdLine() or Initialize() to end program with zero exit code.
        static const int EXIT_WITH_SUCCESS = -42;

    public:

        HINSTANCE m_hInstance;
        LPWSTR m_pwszCmdLine;
        int m_nCmdShow;

    protected:

        static CApplicationBase* s_pAppObject;

        std::wstring m_sVendorShortName;
        std::wstring m_sAppShortName;

#if USING_GDIPLUS
        ULONG_PTR m_gdiplusToken;
#endif

        bool m_AppDataDirInitDone;
        fs::path m_AppDataDir;
};
