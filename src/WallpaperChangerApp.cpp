//////////////////////////////////////////////////////////////////////////////
//
//  WallpaperChangerApp.cpp
//
//  Wallpaper Changer application.
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
#include "resource.h"

#include "WallpaperChangerApp.h"

#include "WallpaperManager.h"
#include "Options.h"
#include "MainFrame.h"
#include "TrayIcon.h"
#include "AboutDlg.h"

//////////////////////////////////////////////////////////////////////////////
//
//  wWinMain
//
//////////////////////////////////////////////////////////////////////////////

// Windows application entry point.
int
WINAPI
wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPWSTR pwszCmdLine,
    _In_ int nCmdShow
)
{
#ifdef _CRTDBG_MAP_ALLOC
    // Enable heap checks and leak detection.
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // Run the application.
    CWallpaperChangerApp theApp;
    return theApp.WinMain(hInstance, pwszCmdLine, nCmdShow);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerApp ctor/dtor
//
//////////////////////////////////////////////////////////////////////////////

CWallpaperChangerApp::CWallpaperChangerApp()
    :
    CApplicationBase(L"Wallpaper Changer"),
    m_pOptions(nullptr),
    m_pWallpaperManager(nullptr),
    m_pMainFrame(nullptr)
{
    // NOTE:
    // m_pOptions and m_pWallpaperManager are set in Initialize().
    // m_pMainFrame is set in CreateMainWindow().
}

CWallpaperChangerApp::~CWallpaperChangerApp()
{
    // m_pWallpaperManager and m_pOptions are deleted in Terminate().
    // Can't do it here because CWallpaperChangerApp will be destroyed
    // after Terminate() shuts down COM and other things we depend on.
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerApp::ParseCmdLine
//
//////////////////////////////////////////////////////////////////////////////

int
CWallpaperChangerApp::ParseCmdLine(
    int argc,
    WCHAR** argv
)
{
    // Call base class to perform standard command line parsing.

    int exitCode = CApplicationBase::ParseCmdLine(argc, argv);

    if (exitCode != 0)
        return exitCode;

    // Find existing instance of our application (if any).

    HWND hCurrentWindow = ::FindWindowW(FRAME_WND_CLASS_NAME, NULL);

    // Check for our own options.

    for (int arg = 1; arg < argc; arg++)
    {
        LPCWSTR pwszArg = argv[arg];

        // Skip any leading slashes or dashes.
        // Allows options to be specified as a switch or bare word.
        while (*pwszArg == L'/' || *pwszArg == L'-')
            pwszArg++;

        if (_wcsicmp(pwszArg, L"tray") == 0)
        {
            // Start with main window hidden and only showing tray icon.
            m_nCmdShow = SW_HIDE;
        }
        else if (_wcsicmp(pwszArg, L"exit") == 0)
        {
            // Exit the running application.
            if (hCurrentWindow != NULL)
                ::PostMessageW(hCurrentWindow, WM_COMMAND, ID_APP_EXIT, 0);
            exitCode = EXIT_WITH_SUCCESS;
            break;
        }
        else if (_wcsicmp(pwszArg, L"autostart=on") == 0)
        {
            // Start automatically at login.
            EnableAutoStart();
            exitCode = EXIT_WITH_SUCCESS;
            break;
        }
        else if (_wcsicmp(pwszArg, L"autostart=off") == 0)
        {
            // Do not start at login.
            DisableAutoStart();
            exitCode = EXIT_WITH_SUCCESS;
            break;
        }
        else
        {
            DebugPrint(L"Invalid command line parameter: %s\n", argv[arg]);
            exitCode = 1;
            break;
        }
    }

    // Activate the existing instance of our application.

    if (exitCode == 0 && hCurrentWindow != NULL)
    {
        if (m_nCmdShow == SW_HIDE)
        {
            // Don't hide current window if it's disabled.
            // Being disabled means a modal dialog is open.
            if (::IsWindowEnabled(hCurrentWindow))
                ::PostMessageW(hCurrentWindow, WM_CLOSE, 0, 0);
        }
        else
        {
            ::SetForegroundWindow(hCurrentWindow);
            ::PostMessageW(hCurrentWindow, WM_COMMAND, ID_APP_OPEN, 0);
        }
        exitCode = EXIT_WITH_SUCCESS;
    }

    return exitCode;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerApp::Initialize
//
//////////////////////////////////////////////////////////////////////////////

int
CWallpaperChangerApp::Initialize()
{
    DebugPrint("Wallpaper Changer started%s\n", (m_nCmdShow == SW_HIDE) ? " in tray" : "");

    // Call base class to perform standard initialization.

    int exitCode = CApplicationBase::Initialize();

    if (exitCode != 0)
        return exitCode;

    // Get the application registry key,
    // creating the key if needed.

    if (!GetAppRegistryKey().IsOpen())
    {
        FatalErrorMessageBox(L"Can't create App Data registry key");
        return -1;
    }

    // Get the application data directory,
    // creating the directory if needed.

    if (GetAppDataDirectory().empty())
    {
        FatalErrorMessageBox(L"Can't create App Data directory");
        return -1;
    }

    // Load global options from the registry.
    // NOTE: This will also create the default playlists if none exist.

    m_pOptions = new CWallpaperChangerOptions;
    m_pOptions->LoadFromRegistry();

    // Initialize wallpaper manager.

    m_pWallpaperManager = new CWallpaperManager;

    // Register to be stopped/restarted by Windows Installer.

    ::RegisterApplicationRestart(L"/tray", RESTART_NO_CRASH | RESTART_NO_HANG);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerApp::Terminate
//
//////////////////////////////////////////////////////////////////////////////

void
CWallpaperChangerApp::Terminate()
{
    // Unregister with Windows Installer.

    ::UnregisterApplicationRestart();

    // Delete objects we created in Initialize().

    delete m_pWallpaperManager;
    m_pWallpaperManager = nullptr;

    delete m_pOptions;
    m_pOptions = nullptr;

    // Call base class to perform standard termination.

    CApplicationBase::Terminate();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerApp::CreateMainWindow
//
//////////////////////////////////////////////////////////////////////////////

bool
CWallpaperChangerApp::CreateMainWindow()
{
    // Load the saved window placement.
    int nCmdShow;
    CRect rcSaved, rcAdjusted;
    CRect* pWindowPosition = nullptr;
    if (LoadWindowPlacement(nCmdShow, rcSaved))
    {
        pWindowPosition = AdjustWindowPlacement(nCmdShow, rcSaved, rcAdjusted);
    }

    // Create frame window object (not Win32 window).
    m_pMainFrame = new CMainFrame();

    // Create Win32 window.
    if (!m_pMainFrame || !m_pMainFrame->CreateEx(NULL, pWindowPosition))
    {
        DeleteMainWindow();
        FatalErrorMessageBox(L"Main window creation failed!");
        return false;
    }

    // Set the window's "normal" location to saved location.
    if (nCmdShow != SW_NORMAL && !rcSaved.IsRectEmpty())
    {
        WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
        m_pMainFrame->GetWindowPlacement(&placement);
        placement.rcNormalPosition = rcSaved;
        placement.showCmd = SW_HIDE;
        m_pMainFrame->SetWindowPlacement(&placement);
    }

    // Show the window.
    m_pMainFrame->ShowWindow(nCmdShow);

    return true;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerApp::DeleteMainWindow
//
//////////////////////////////////////////////////////////////////////////////

void
CWallpaperChangerApp::DeleteMainWindow()
{
    if (m_pMainFrame)
    {
        if (m_pMainFrame->m_hWnd && ::IsWindow(m_pMainFrame->m_hWnd))
            m_pMainFrame->DestroyWindow();

        delete m_pMainFrame;

        m_pMainFrame = nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerApp::IsAutoStartEnabled
//  CWallpaperChangerApp::EnableAutoStart
//  CWallpaperChangerApp::DisableAutoStart
//
//////////////////////////////////////////////////////////////////////////////

static CRegistryKey GetRunKey()
{
    static const WCHAR s_wszRunKeyName[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    return CRegistryKey(HKEY_CURRENT_USER, s_wszRunKeyName);
}

// Is application set to start at login?
bool
CWallpaperChangerApp::IsAutoStartEnabled()
{
    return GetRunKey().ValueExists(m_sAppShortName);
}

// Automatically start the application at login.
void
CWallpaperChangerApp::EnableAutoStart()
{
    std::wstring cmdline;
    cmdline += QuoteIfNeeded(GetExecutablePath());
    cmdline += L" ";
    cmdline += L"/tray";

    GetRunKey().Write(m_sAppShortName, cmdline);
}

// Do not start the application at login.
void
CWallpaperChangerApp::DisableAutoStart()
{
    GetRunKey().DeleteValue(m_sAppShortName);
}
