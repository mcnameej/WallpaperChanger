//////////////////////////////////////////////////////////////////////////////
//
//  WallpaperChangerApp.h
//
//  CWallpaperChangerApp class.
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

#include "AppBase.h"

// Wallpaper Changer application.
class CWallpaperChangerApp : public CApplicationBase
{
    public:

        CWallpaperChangerApp();

        ~CWallpaperChangerApp();

        // Parse the command line.
        virtual
        int
        ParseCmdLine(
            int argc,
            WCHAR** argv
        )
        override;

        // Initialize the application.
        virtual
        int
        Initialize()
        override;

        // Terminate the application.
        virtual
        void
        Terminate()
        override;

        // Create and show the main frame window.
        virtual
        bool
        CreateMainWindow()
        override;

        // Delete the main frame window.
        virtual
        void
        DeleteMainWindow()
        override;

    public:

        class CWallpaperChangerOptions*
        GetAppOptions()
        {
            return m_pOptions;
        }

        class CWallpaperManager*
        GetWallpaperManager()
        {
            return m_pWallpaperManager;
        }

        // Are we set to run at login?
        bool
        IsAutoStartEnabled();

        // Enable running the application at login.
        void
        EnableAutoStart();

        // Disable running the application at login.
        void
        DisableAutoStart();

    private:

        class CWallpaperChangerOptions* m_pOptions;

        class CWallpaperManager* m_pWallpaperManager;

        class CMainFrame* m_pMainFrame;
};

// Get CWallpaperChanger application object.
FORCEINLINE
CWallpaperChangerApp*
GetApp()
{
    return (CWallpaperChangerApp*) CApplicationBase::GetApp();
}

// Get Wallpaper Changer global options.
FORCEINLINE
class CWallpaperChangerOptions*
GetAppOptions()
{
    return GetApp()->GetAppOptions();
}

// Get Wallpaper Manager object.
FORCEINLINE
class CWallpaperManager*
GetWallpaperManager()
{
    return GetApp()->GetWallpaperManager();
}
