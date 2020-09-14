//////////////////////////////////////////////////////////////////////////////
//
//  MainFrame.cpp
//
//  CMainFrame message handlers and misc functions.
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

#include "Options.h"
#include "MainFrame.h"

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame ctor
//
//////////////////////////////////////////////////////////////////////////////

CMainFrame::CMainFrame()
    :
    // Syntactic Sugar to make access to wallpaper manager more natural.
    WallpaperManager(*(GetApp()->GetWallpaperManager())),

    // Initialize timer-related fields.
    m_CountdownTimer(),
    m_UserInterfaceUpdateTimerStarted(false),

    // Window isn't visible until explicitly shown.
    m_WindowIsVisible(false)
{
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnCreate
//
//////////////////////////////////////////////////////////////////////////////

// Handle WM_CREATE message.
LRESULT CMainFrame::OnCreate(LPCREATESTRUCT /*lpCreateStruct*/)
{
    DebugPrintCmdSpew("WM_CREATE\n");

    // Remember whether we started up in paused state.
    bool startedInPausedState = GetAppOptions()->m_Paused;
    GetAppOptions()->m_Paused = false;

    // Remember the handle of the Playlist menu.
    // OnInitMenuPopup() needs it.
    m_PlaylistMenu = ::GetSubMenu(GetMenu(), 1);

    // Initialize countdown timer.
    // DOES NOT actually start the timer.
    m_CountdownTimer.Initialize(m_hWnd, ID_COUNTDOWN_TIMER);
    SetCountdownTimerInterval();

    // Create toolbar (actually rebar with toolbar band).
    InitializeToolbar();

    // Create status bar.
    CreateSimpleStatusBar();
    UIAddStatusBar(m_hWndStatusBar);

    // Create list view (initializes m_ListView and sets m_hWndClient).
    CreateListView();

    // Load last used playlist (initializes m_PlayList).
    LoadPlaylist(GetAppOptions()->GetRecentPlaylistName(0));

    // LoadPlaylist() will have started the countdown timer.
    // Pause the countdown if we started up in paused state.
    if (startedInPausedState)
        PauseTimedChanges();

    // Populate the list view.
    PopulateListView(WallpaperManager.GetCurrentWallpaperFile());

    // Register hotkeys and update menu items.
    RegisterAllHotkeys();

    // Update menu items (enable/disable and check).
    UpdateMainMenu();

    // Find LICENSE.txt file in same directory as application EXE.
    fs::path licenseFile = GetExecutablePath();
    licenseFile.replace_filename(L"LICENSE.txt");
    UIEnable(ID_APP_LICENSE, fs::is_regular_file(licenseFile));

    // Create tray icon.
    m_TrayIcon.Create(m_hWnd);

    // Register as drag-and-drop target.
    // Applies to all child windows too (e.g. the ListView),
    // but the WM_DROPFILES message is sent to frame window.
    DragAcceptFiles(TRUE);

    // Enable PreTranslateMessage() and OnIdle() callbacks.
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->AddMessageFilter(this);
    pLoop->AddIdleHandler(this);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnDestroy
//
//////////////////////////////////////////////////////////////////////////////

// Handle WM_DESTROY message.
void CMainFrame::OnDestroy()
{
    DebugPrintCmdSpew("WM_DESTROY\n");

    // Stop PreTranslateMessage() and OnIdle() callbacks.
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->RemoveMessageFilter(this);
    pLoop->RemoveIdleHandler(this);

    // Stop being a drop target.
    DragAcceptFiles(FALSE);

    // Remove tray icon.
    m_TrayIcon.Destroy();

    // Stop timers.
    m_CountdownTimer.Stop();
    StopUserInterfaceUpdateTimer();

    // Unregister hotkeys.
    ::UnregisterHotKey(m_hWnd, ID_WALLPAPER_NEXT);
    ::UnregisterHotKey(m_hWnd, ID_WALLPAPER_PREV);
    ::UnregisterHotKey(m_hWnd, ID_WALLPAPER_RANDOM);
    ::UnregisterHotKey(m_hWnd, ID_WALLPAPER_SAFE);

    // Save our window size and position.
    GetApp()->SaveWindowPlacement(m_hWnd);

    // Stop the message loop and exit the application.
    ::PostQuitMessage(0);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnClose
//
//////////////////////////////////////////////////////////////////////////////

// Handle WM_CLOSE message.
void CMainFrame::OnClose()
{
    DebugPrintCmdSpew("WM_CLOSE\n");

    // Hide the window instead of closing it.
    ShowWindow(SW_HIDE);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnEndSession
//
//////////////////////////////////////////////////////////////////////////////

// Handle WM_ENDSESSION message.
void CMainFrame::OnEndSession(BOOL bEnding, UINT uLogOff)
{
    DebugPrintCmdSpew("WM_ENDSESSION: bEnding = %s, uLogOff = 0x%08X\n", bEnding ? "TRUE" : "FALSE", uLogOff);

    if (bEnding)
        DestroyWindow();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnSize
//
//////////////////////////////////////////////////////////////////////////////

// Handle WM_SIZE message.
void CMainFrame::OnSize(UINT nType, CSize size)
{
    DebugPrintCmdSpew("WM_SIZE: nType = %u, size = %ld x %ld\n", nType, size.cx, size.cy);

    // Keep track of main window visibility, so we can stop the UI Update
    // timer when window is hidden.  Avoids wasting CPU cycles on updates
    // that nobody can see.

    if (nType == SIZE_RESTORED || nType == SIZE_MAXSHOW || nType == SIZE_MAXIMIZED)
    {
        UpdateWindowVisibility(true);
    }
    else if (nType == SIZE_MINIMIZED)
    {
        UpdateWindowVisibility(false);
    }

    SetMsgHandled(FALSE);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnShowWindow
//
//////////////////////////////////////////////////////////////////////////////

// Handle WM_SHOWWINDOW message.
void CMainFrame::OnShowWindow(BOOL bShow, UINT /*nStatus*/)
{
    DebugPrintCmdSpew("WM_SHOWWINDOW: %s\n", bShow ? "TRUE" : "FALSE");

    // Keep track of main window visibility, so we can stop the UI Update
    // timer when window is hidden.  Avoids wasting CPU cycles on updates
    // that nobody can see.

    UpdateWindowVisibility(bShow);

    SetMsgHandled(FALSE);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnInitMenuPopup
//
//////////////////////////////////////////////////////////////////////////////

// Handle WM_INITMENUPOPUP message.
void CMainFrame::OnInitMenuPopup(CMenuHandle menuPopup, UINT nIndex, BOOL bSysMenu)
{
    // Update MRU list in Playlist menu.
    if (menuPopup == m_PlaylistMenu && !bSysMenu)
    {
        DebugPrintCmdSpew("WM_INITMENUPOPUP: \"Playlist\"\n");

        // Remove all the existing recent playlist menu items.
        for (int idx = 0; idx < GetAppOptions()->MaxRecentPlaylists; idx++)
            RemoveMenu(menuPopup, ID_PLAYLIST_RECENT_1 + idx, MF_BYCOMMAND);

        // Add recent playlist menu items.
        for (int idx = 0; idx < GetAppOptions()->m_RecentPlaylists.size(); idx++)
        {
            WCHAR wszMenuText[128];

            std::wstring strPlaylistName = fs::path(GetAppOptions()->m_RecentPlaylists[idx]).stem();

            StringCbPrintfW(wszMenuText,
                            sizeof(wszMenuText),
                            L"&%d. %s",
                            idx + 1,
                            strPlaylistName.c_str());

            AppendMenuW(menuPopup,
                        MF_STRING,
                        ID_PLAYLIST_RECENT_1 + idx,
                        wszMenuText);
        }
    }

    // Let other handlers process WM_INITMENUPOPUP too.
    SetMsgHandled(FALSE);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnDropFiles
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnDropFiles(HDROP hDrop)
{
    UINT fileCount = ::DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);

    // DebugPrintCmdSpew("WM_DROPFILES: %u files\n", fileCount);

    BeginListViewUpdate();

    int iFirstAddedItem = -1;

    // Process each dropped file.
    for (UINT idx = 0; idx < fileCount; idx++)
    {
        // Get file path.
        UINT pathLength = ::DragQueryFileW(hDrop, idx, NULL, 0);
        std::wstring strPath;
        strPath.resize(pathLength);
        ::DragQueryFileW(hDrop, idx, strPath.data(), pathLength + 1);

        // Make sure "file" is really a file and not a directory.
        if (!fs::is_regular_file(strPath))
            continue;

        // Verify that file is a valid image.
        if (!IsValidImageFile(strPath))
            continue;

        // Add file to playlist.
        auto it = m_PlayList.Add(strPath, false);
        if (it == m_PlayList.end())
            continue;  // File is already in playlist.

        // Add file to listview.
        int iItem = AddFileToListView(*it);

        // Remember the first added file.
        if (iFirstAddedItem == -1)
            iFirstAddedItem = iItem;

        DebugPrintCmdSpew("WM_DROPFILES: [%u] %ws\n", idx, strPath.c_str());
    }

    EndListViewUpdate(iFirstAddedItem);

    // Save the updated playlist (but only if a file was added).
    if (iFirstAddedItem != -1)
        m_PlayList.Save();

    // Free memory for drag-and-drop.
    ::DragFinish(hDrop);

    // Activate our window after having files dropped on us.
    OnAppOpen(0, 0, nullptr);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::PreTranslateMessage
//
//////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
    // Let frame window base class handle translation.
    return CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnIdle
//
//////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::OnIdle()
{
    UIUpdateToolBar();
    UIUpdateStatusBar();
    return FALSE;
}

//****************************************************************************
//
//  Main Frame functions.
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::RegisterAllHotkeys
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::RegisterAllHotkeys()
{
    // Register hotkeys and update the main menu.

    _UpdateHotkey(GetMenu(), ID_WALLPAPER_NEXT, GetAppOptions()->m_NextImageHotkey);
    _UpdateHotkey(GetMenu(), ID_WALLPAPER_PREV, GetAppOptions()->m_PrevImageHotkey);
    _UpdateHotkey(GetMenu(), ID_WALLPAPER_RANDOM, GetAppOptions()->m_RandomImageHotkey);
    _UpdateHotkey(GetMenu(), ID_WALLPAPER_SAFE, GetAppOptions()->m_SafePlaylistHotkey);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::_UpdateHotkey
//
//////////////////////////////////////////////////////////////////////////////

// Register hotkey and update menu.
void CMainFrame::_UpdateHotkey(CMenuHandle menu, int ID, CHotKeyDefinition& hotKey)
{
    // Register the hotkey (or unregister it if hotkey is disabled).

    bool isEnabled = hotKey.Register(m_hWnd, ID);

    // Update menu item to include hotkey string
    // (or remove if hotkey is disabled).

    std::wstring fullText = GetMenuItemStringW(menu, ID, MF_BYCOMMAND);

    if (!fullText.empty())
    {
        std::wstring newText = fullText.substr(0, fullText.find(L'\t'));

        if (isEnabled)
        {
            newText += L'\t';
            newText += hotKey.ToString();
        }

        menu.ModifyMenuW(ID, MF_BYCOMMAND | MF_STRING, ID, newText.c_str());
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::UpdateWindowTitle
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateWindowTitle()
{
    // Playlist first, then application name.
    std::wstring title = m_PlayList.GetPlaylistName();
    title += L" - ";
    title += LoadStringResourceW(IDR_MAINFRAME);
    SetWindowTextW(title.c_str());
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::UpdateMainMenu
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateMainMenu()
{
    UISetCheck(ID_WALLPAPER_PAUSE, IsPaused());

    UIEnable(ID_WALLPAPER_CHANGE, !m_PlayList.empty());
    UIEnable(ID_WALLPAPER_NEXT,   !m_PlayList.empty());
    UIEnable(ID_WALLPAPER_PREV,   !m_PlayList.empty());
    UIEnable(ID_WALLPAPER_RANDOM, !m_PlayList.empty());

    UIEnable(ID_WALLPAPER_SAFE, !GetAppOptions()->m_SafePlaylist.empty());
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::UpdateWindowVisibility
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateWindowVisibility(bool isVisible)
{
    // Don't do anything unless visibility changes.
    if (isVisible == m_WindowIsVisible)
        return;

    m_WindowIsVisible = isVisible;

    if (isVisible)
    {
        // Display the current time remaining.
        ShowCountdownTimer();

        // Update the countdown display every second.
        StartUserInterfaceUpdateTimer();
    }
    else
    {
        // Stop UI timer.
        StopUserInterfaceUpdateTimer();

        // Empty the process working set.
        ::SetProcessWorkingSetSize(::GetCurrentProcess(), (SIZE_T) -1, (SIZE_T) -1);
    }
}
