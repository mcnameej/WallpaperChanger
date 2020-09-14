//////////////////////////////////////////////////////////////////////////////
//
//  MF.Commands.cpp
//
//  CMainFrame menu command functions.
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

#include "MainFrame.h"
#include "OptionsDlg.h"
#include "AboutDlg.h"
#include "PlayListManagerDlg.h"
#include "VersionInfo.h"

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnCommand
//
//////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

// Receive WM_COMMAND messages and print parameters for debugging.
void CMainFrame::OnCommand(UINT uNotifyCode, int nID, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("WM_COMMAND: ID = %d, NotifyCode = %u\n", nID, uNotifyCode);
    SetMsgHandled(FALSE);
}

#endif

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnHotkey
//
//////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

// Receive WM_HOTKEY messages and print parameters for debugging.
void CMainFrame::OnHotKey(int nHotKeyID, UINT uModifiers, UINT uVirtKey)
{
    DebugPrintCmdSpew("WM_HOTKEY: ID = %d, VirtKey = 0x%02X, Modifiers = 0x%04X\n", nHotKeyID, uVirtKey, uModifiers);
    SetMsgHandled(FALSE);
}

#endif


//****************************************************************************
//
//  File menu
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnAppOptions
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_APP_OPTIONS command.
LRESULT CMainFrame::OnAppOptions(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_APP_OPTIONS\n");

    int originalChangeInterval = GetAppOptions()->m_ChangeInterval;

    if (COptionsDlg().DoModal(m_hWnd) == IDOK)
    {
        // Hotkeys may have changed, so re-register them.
        RegisterAllHotkeys();

        // Begin new countdown if interval was changed.
        if (originalChangeInterval != GetAppOptions()->m_ChangeInterval)
        {
            SetCountdownTimerInterval();

            if (!IsPaused())
                BeginCountdown();
        }

        // Update main menu (and toolbar buttons).
        UpdateMainMenu();

        // Refresh listview.
        RefreshListView();
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnAppClose
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_APP_CLOSE command.
LRESULT CMainFrame::OnAppClose(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    // ID_APP_CLOSE hides the main frame window, but leaves application running.
    // User can re-open the main frame via the tray icon.

    DebugPrintCmdSpew("ID_APP_CLOSE\n");

    OnClose();  // <-- Actually hides the window; doesn't destroy it.

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnAppExit
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_APP_EXIT command.
LRESULT CMainFrame::OnAppExit(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    // ID_APP_EXIT quits the application.

    DebugPrintCmdSpew("ID_APP_EXIT\n");

    DestroyWindow();

    return 0;
}


//****************************************************************************
//
//  Playlist menu
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnPlaylistManager
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_PLAYLIST_MANAGER command.
LRESULT CMainFrame::OnPlaylistManager(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_PLAYLIST_MANAGER\n");

    CPlayListManagerDlg playlistManager(m_PlayList);

    if (playlistManager.DoModal(m_hWnd) == IDOK)
    {
        // Load new playlist.
        LoadPlaylist(playlistManager.GetNewPlayListName());
        PopulateListView(WallpaperManager.GetCurrentWallpaperFile());
    }
    else
    {
        // Update certain UI elements even if not changing playlist.
        // The current playlist may have been renamed.
        UpdateWindowTitle();
        UpdatePlaylistInToolbar();
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnAddImage
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_PLAYLIST_ADD command.
LRESULT CMainFrame::OnAddImage(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_PLAYLIST_ADD\n");

    static const WCHAR wszFilter[] =
        L"Image Files (*.bmp;*.jpg;*.jpeg;*.png)\0" L"*.bmp;*.jpg;*.jpeg;*.png\0"
        // L"All Files (*.*)\0" L"*.*\0"
        L"\0";

    CMultiFileDialog fileOpenDlg(NULL,
                                 NULL,
                                 OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                                 wszFilter);

    fileOpenDlg.m_ofn.lpstrTitle = L"Add Images To Playlist";

    if (fileOpenDlg.DoModal(m_hWnd) == IDOK)
    {
        int directoryLength = fileOpenDlg.GetDirectory(NULL, 0);
        if (directoryLength)
        {
            std::wstring strDirectory;
            strDirectory.resize(directoryLength - 1);
            fileOpenDlg.GetDirectory(strDirectory.data(), directoryLength);
            fs::path directoryPath = std::move(strDirectory);

            BeginListViewUpdate();

            int iFirstAddedItem = -1;

            for (LPCWSTR pwszFileName = fileOpenDlg.GetFirstFileName();
                 pwszFileName != NULL;
                 pwszFileName = fileOpenDlg.GetNextFileName())
            {
                fs::path fullPath = directoryPath / pwszFileName;

                // Only add files, not subdirectories.
                if (!fs::is_regular_file(fullPath))
                    continue;

                // Verify that file is a valid image.
                if (!IsValidImageFile(fullPath))
                    continue;

                // Add file to playlist.
                auto it = m_PlayList.Add(fullPath, false);
                if (it == m_PlayList.end())
                    continue;  // File is already in playlist.

                // Add file to listview.
                int iItem = AddFileToListView(*it);

                // Remember the first added file.
                if (iFirstAddedItem == -1)
                    iFirstAddedItem = iItem;
            }

            EndListViewUpdate(iFirstAddedItem);

            // Save the updated playlist
            // (only if a file was added).
            if (iFirstAddedItem != -1)
                m_PlayList.Save();
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnRemoveImage
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_PLAYLIST_REMOVE command.
LRESULT CMainFrame::OnRemoveImage(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_PLAYLIST_REMOVE\n");

    BeginListViewUpdate();

    fs::path currentWallpaperFile = WallpaperManager.GetCurrentWallpaperFile();
    bool deletedCurrentWallpaper = false;

    int iItem, iStart = -1, iItemToSelect = -1;

    while ((iItem = m_ListView.GetNextItem(iStart, LVNI_SELECTED)) != -1)
    {
        if (iItemToSelect == -1)
            iItemToSelect = iItem;

        iStart = m_ListView.GetNextItem(iItem, LVNI_PREVIOUS);

        CFileInfo* pFile = GetListViewItemData(iItem);

        if (pFile->m_FullPath == currentWallpaperFile)
            deletedCurrentWallpaper = true;

        m_ListView.DeleteItem(iItem);

        m_PlayList.Remove(pFile);
    }

    if (iItemToSelect != -1)
    {
        if (iItemToSelect >= m_ListView.GetItemCount())
            iItemToSelect = m_ListView.GetItemCount() - 1;
    }

    EndListViewUpdate(iItemToSelect);

    if (deletedCurrentWallpaper)
    {
        // If we deleted the current wallpaper, display whatever is selected now.
        // If the list is empty, the net result will be a solid color desktop background.
        ChangeWallpaperImage(GetCurrentSelectedFile());
    }

    // Save the updated playlist (but only if a file was removed).
    if (iItemToSelect != -1)
        m_PlayList.Save();

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnShuffle
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_PLAYLIST_SHUFFLE command.
LRESULT CMainFrame::OnShuffle(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_PLAYLIST_SHUFFLE\n");

    fs::path selectedFile = GetCurrentSelectedFile();

    m_PlayList.Shuffle();

    m_PlayList.Save();

    PopulateListView(selectedFile);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnCurrent
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_PLAYLIST_CURRENT command.
LRESULT CMainFrame::OnCurrent(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_PLAYLIST_CURRENT\n");

    int iItem = GetCurrentWallpaperItem();

    if (iItem != -1)
        m_ListView.SelectItem(iItem);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnOpenRecent
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_PLAYLIST_RECENT_X command.
LRESULT CMainFrame::OnOpenRecent(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    int nRecentList = nID - ID_PLAYLIST_RECENT_1;

    DebugPrintCmdSpew("ID_PLAYLIST_RECENT_%d\n", nRecentList + 1);

    std::wstring playlistName = GetAppOptions()->GetRecentPlaylistName(nRecentList);

    if (!playlistName.empty())
    {
        // Load selected playlist.
        LoadPlaylist(playlistName);
        PopulateListView(WallpaperManager.GetCurrentWallpaperFile());
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnSelectAll
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_EDIT_SELECT_ALL command.
LRESULT CMainFrame::OnSelectAll(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_EDIT_SELECT_ALL\n");

    m_ListView.SelectAllItems(true);

    return 0;
}


//****************************************************************************
//
//  Wallpaper menu
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnShowSelected
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_WALLPAPER_CHANGE command.
LRESULT CMainFrame::OnShowSelected(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_WALLPAPER_CHANGE\n");

    // Only change wallpaper if a single item is selected.
    if (m_ListView.GetSelectedCount() == 1)
    {
        ChangeWallpaperImage(GetCurrentSelectedFile());
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnShowNextPrev
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_WALLPAPER_NEXT and ID_WALLPAPER_PREV commands.
LRESULT CMainFrame::OnShowNextPrev(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_WALLPAPER_NEXT/PREV\n");

    // Show next/previous wallpaper.
    CFileInfo* pFile = ShowNextOrPrev(nID);

    // Select new wallpaper in list view.
    m_ListView.SelectItem(GetWallpaperItem(pFile));

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnShowRandom
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_WALLPAPER_RANDOM command.
LRESULT CMainFrame::OnShowRandom(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_WALLPAPER_RANDOM\n");

    // Only change wallpaper if playlist isn't empty.
    if (!m_PlayList.empty())
    {
        // Pick a new wallpaper image at random.
        CFileInfo* pFile = m_PlayList.GetRandom();

        // Display the new wallpaper image.
        ChangeWallpaperImage(pFile->m_FullPath);

        // Select new wallpaper in list view.
        m_ListView.SelectItem(GetWallpaperItem(pFile));
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnShowSafe
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_WALLPAPER_SAFE command.
LRESULT CMainFrame::OnShowSafe(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_WALLPAPER_SAFE\n");

    if (!GetAppOptions()->m_SafePlaylist.empty())
    {
        // Load "Safe For Work" playlist.
        LoadPlaylist(GetAppOptions()->m_SafePlaylist);
        PopulateListView(WallpaperManager.GetCurrentWallpaperFile());
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnPauseChanges
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_WALLPAPER_PAUSE command.
LRESULT CMainFrame::OnPauseChanges(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_WALLPAPER_PAUSE\n");

    // Toggle pause mode.

    if (IsPaused())
    {
        ResumeTimedChanges();
    }
    else
    {
        PauseTimedChanges();
    }

    UpdateMainMenu();

    return 0;
}


//****************************************************************************
//
//  Help menu
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnAppAbout
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_APP_ABOUT command.
LRESULT CMainFrame::OnAppAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_APP_ABOUT\n");

    CAboutDlg().DoModal(m_hWnd);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnViewLicense
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_APP_LICENSE command.
LRESULT CMainFrame::OnViewLicense(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_APP_LICENSE\n");

    fs::path licenseFile = GetExecutablePath();
    licenseFile.replace_filename(L"LICENSE.txt");
    if (fs::is_regular_file(licenseFile))
    {
        ::ShellExecuteW(m_hWnd, L"open", licenseFile.c_str(), NULL, NULL, SW_NORMAL);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnViewReadMe
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_APP_README command.
LRESULT CMainFrame::OnViewReadMe(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_APP_README\n");

    fs::path readmeFile = GetExecutablePath();
    readmeFile.replace_filename(L"README.html");
    if (fs::is_regular_file(readmeFile))
    {
        ::ShellExecuteW(m_hWnd, L"open", readmeFile.c_str(), NULL, NULL, SW_NORMAL);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnCheckForUpdates
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_APP_CHECK_FOR_UPDATES command.
LRESULT CMainFrame::OnCheckForUpdates(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_APP_CHECK_FOR_UPDATES\n");

    CApplicationVersionInfo AppVersion;

    if (AppVersion.IsOK())
    {
        int Major, Minor, Build, Patch;
        AppVersion.GetProductVersion(&Major, &Minor, &Build, &Patch);

        auto strURL = Format(LoadStringResourceW(IDS_UPDATE_URL), Major, Minor, Build, Patch);

        ::ShellExecuteW(m_hWnd, L"open", strURL.c_str(), NULL, NULL, SW_NORMAL);
    }

    return 0;
}


//****************************************************************************
//
//  Context (right click) menu
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnOpenImage
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_OPEN_IMAGE_FILE command.
LRESULT CMainFrame::OnOpenImage(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_OPEN_IMAGE_FILE\n");

    fs::path fullPath = GetCurrentSelectedFile();

    if (!fullPath.empty())
        ::ShellExecuteW(m_hWnd, L"open", fullPath.c_str(), NULL, NULL, SW_NORMAL);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnEditImage
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_EDIT_IMAGE_FILE command.
LRESULT CMainFrame::OnEditImage(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_EDIT_IMAGE_FILE\n");

    fs::path fullPath = GetCurrentSelectedFile();

    if (!fullPath.empty())
        ::ShellExecuteW(m_hWnd, L"edit", fullPath.c_str(), NULL, NULL, SW_NORMAL);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnOpenContainingFolder
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_OPEN_IMAGE_FOLDER command.
LRESULT CMainFrame::OnOpenContainingFolder(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    DebugPrintCmdSpew("ID_OPEN_IMAGE_FOLDER\n");

    fs::path fullPath = GetCurrentSelectedFile();

    if (!fullPath.empty())
    {
        CComHeapPtr<ITEMIDLIST> pidl;
        SFGAOF unused;
        if (SUCCEEDED(::SHParseDisplayName(fullPath.c_str(), nullptr, &pidl, 0, &unused)))
        {
            ::SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
        }
    }

    return 0;
}
