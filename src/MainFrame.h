//////////////////////////////////////////////////////////////////////////////
//
//  MainFrame.h
//
//  CMainFrame class declaration.
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

#include "TrayIcon.h"
#include "PlayList.h"
#include "CountdownTimer.h"
#include "ImageListCache.h"
#include "WallpaperManager.h"
#include "ToolBarHelper.h"

// Print debug messages as each command is processed.
// Set to 1 to enable the spew or 0 to disable it.
// Always disabled for Release builds.
#define ENABLE_CMD_SPEW 0

#if ENABLE_CMD_SPEW
  #define DebugPrintCmdSpew(format, ...) DebugPrint(format, ##__VA_ARGS__)
#else
  #define DebugPrintCmdSpew(format, ...) do {} while (0)
#endif

// void OnHotKeyIDHandlerEX(UINT /*unused1*/, int nID, CWindow /*unused2*/)
#define HOTKEY_ID_HANDLER_EX(id, func) \
    if ((uMsg == WM_HOTKEY) && (id == wParam)) \
    { \
        this->SetMsgHandled(TRUE); \
        func(0, (int)wParam, (HWND)NULL); \
        lResult = 0; \
        if(this->IsMsgHandled()) \
            return TRUE; \
    }

// void OnTimerID(UINT_PTR nID)
#define TIMER_ID_HANDLER_EX(id, func) \
    if ((uMsg == WM_TIMER) && (id == wParam)) \
    { \
        this->SetMsgHandled(TRUE); \
        func((UINT_PTR)wParam); \
        lResult = 0; \
        if(this->IsMsgHandled()) \
            return TRUE; \
    }

// Class name for the frame window. Doesn't need to be a GUID,
// but does need to be a globally unique string. Used in WinMain()
// to find an existing instance of the application and activate it,
// rather than starting a new instance.
#define FRAME_WND_CLASS_NAME L"{E37BBFBA-168F-4EF8-B799-3BDF269F1C90}"

// Main application frame window.
class CMainFrame
    :
    public CFrameWindowImpl<CMainFrame>,
    public CToolBarHelper<CMainFrame>,
    public CUpdateUI<CMainFrame>,
    public CMessageFilter,
    public CIdleHandler
{
    public:

        // Default ctor.
        CMainFrame();

        DECLARE_FRAME_WND_CLASS(FRAME_WND_CLASS_NAME, IDR_MAINFRAME)

        BEGIN_UPDATE_UI_MAP(CMainFrame)
            UPDATE_ELEMENT(ID_PLAYLIST_REMOVE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
            UPDATE_ELEMENT(ID_WALLPAPER_CHANGE, UPDUI_MENUPOPUP)
            UPDATE_ELEMENT(ID_WALLPAPER_NEXT, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
            UPDATE_ELEMENT(ID_WALLPAPER_PREV, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
            UPDATE_ELEMENT(ID_WALLPAPER_RANDOM, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
            UPDATE_ELEMENT(ID_WALLPAPER_SAFE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
            UPDATE_ELEMENT(ID_WALLPAPER_PAUSE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
            UPDATE_ELEMENT(ID_APP_LICENSE, UPDUI_MENUPOPUP)
            UPDATE_ELEMENT(ID_DEFAULT_PANE, UPDUI_STATUSBAR)
        END_UPDATE_UI_MAP()

        BEGIN_MSG_MAP(CMainFrame)
            MSG_WM_CREATE(OnCreate)
            MSG_WM_DESTROY(OnDestroy)
            MSG_WM_CLOSE(OnClose)
            MSG_WM_ENDSESSION(OnEndSession)
            MSG_WM_SIZE(OnSize)
            MSG_WM_SHOWWINDOW(OnShowWindow)
            MSG_WM_INITMENUPOPUP(OnInitMenuPopup)
            MSG_WM_DROPFILES(OnDropFiles)

            TIMER_ID_HANDLER_EX(ID_COUNTDOWN_TIMER, OnCountdownTimer)
            TIMER_ID_HANDLER_EX(ID_UI_UPDATE_TIMER, OnUserInterfaceUpdateTimer)

#ifdef _DEBUG
            MSG_WM_COMMAND(OnCommand)
            MSG_WM_HOTKEY(OnHotKey)
#endif

            COMMAND_ID_HANDLER_EX(ID_APP_OPTIONS, OnAppOptions)
            COMMAND_ID_HANDLER_EX(ID_APP_CLOSE, OnAppClose)
            COMMAND_ID_HANDLER_EX(ID_APP_EXIT, OnAppExit)

            COMMAND_ID_HANDLER_EX(ID_APP_ABOUT, OnAppAbout)
            COMMAND_ID_HANDLER_EX(ID_APP_LICENSE, OnViewLicense)
            COMMAND_ID_HANDLER_EX(ID_APP_README, OnViewReadMe)
            COMMAND_ID_HANDLER_EX(ID_APP_CHECK_FOR_UPDATES, OnCheckForUpdates)

            COMMAND_ID_HANDLER_EX(ID_PLAYLIST_MANAGER, OnPlaylistManager)
            COMMAND_ID_HANDLER_EX(ID_PLAYLIST_ADD, OnAddImage)
            COMMAND_ID_HANDLER_EX(ID_PLAYLIST_REMOVE, OnRemoveImage)
            COMMAND_ID_HANDLER_EX(ID_PLAYLIST_SHUFFLE, OnShuffle)
            COMMAND_ID_HANDLER_EX(ID_PLAYLIST_CURRENT, OnCurrent)
            COMMAND_RANGE_HANDLER_EX(ID_PLAYLIST_RECENT_1, ID_PLAYLIST_RECENT_5, OnOpenRecent)
            COMMAND_ID_HANDLER_EX(ID_EDIT_SELECT_ALL, OnSelectAll)

            COMMAND_ID_HANDLER_EX(ID_WALLPAPER_CHANGE, OnShowSelected)
            COMMAND_ID_HANDLER_EX(ID_WALLPAPER_NEXT, OnShowNextPrev)
            COMMAND_ID_HANDLER_EX(ID_WALLPAPER_PREV, OnShowNextPrev)
            COMMAND_ID_HANDLER_EX(ID_WALLPAPER_RANDOM, OnShowRandom)
            COMMAND_ID_HANDLER_EX(ID_WALLPAPER_SAFE, OnShowSafe)
            COMMAND_ID_HANDLER_EX(ID_WALLPAPER_PAUSE, OnPauseChanges)

            HOTKEY_ID_HANDLER_EX(ID_WALLPAPER_NEXT, OnShowNextPrev)
            HOTKEY_ID_HANDLER_EX(ID_WALLPAPER_PREV, OnShowNextPrev)
            HOTKEY_ID_HANDLER_EX(ID_WALLPAPER_RANDOM, OnShowRandom)
            HOTKEY_ID_HANDLER_EX(ID_WALLPAPER_SAFE, OnShowSafe)

            COMMAND_ID_HANDLER_EX(ID_OPEN_IMAGE_FILE, OnOpenImage)
            COMMAND_ID_HANDLER_EX(ID_EDIT_IMAGE_FILE, OnEditImage)
            COMMAND_ID_HANDLER_EX(ID_OPEN_IMAGE_FOLDER, OnOpenContainingFolder)

            NOTIFY_HANDLER_EX(ID_PLAYLIST_VIEW, LVN_GETEMPTYMARKUP, OnGetEmptyMarkup)
            NOTIFY_HANDLER_EX(ID_PLAYLIST_VIEW, LVN_GETDISPINFO, OnGetDispInfo)
            NOTIFY_HANDLER_EX(ID_PLAYLIST_VIEW, LVN_GETINFOTIP, OnGetInfoTip)
            NOTIFY_HANDLER_EX(ID_PLAYLIST_VIEW, LVN_ITEMCHANGED, OnItemChanged)
            NOTIFY_HANDLER_EX(ID_PLAYLIST_VIEW, NM_RCLICK, OnRightClick)
            NOTIFY_HANDLER_EX(ID_PLAYLIST_VIEW, NM_DBLCLK, OnDoubleClick)
            NOTIFY_HANDLER_EX(ID_PLAYLIST_VIEW, NM_RETURN, OnReturn)

            COMMAND_HANDLER_EX(IDC_PLAYLISTS, CBN_DROPDOWN, OnToolbarPlaylistComboDropDown)
            COMMAND_HANDLER_EX(IDC_PLAYLISTS, CBN_SELCHANGE, OnToolbarPlaylistComboSelChange)

            MESSAGE_HANDLER_EX(WM_TRAYICON, OnTrayIconMsg)
            MESSAGE_HANDLER_EX(m_TrayIcon.m_TaskbarRestartMessage, OnTaskbarRestarted)
            COMMAND_ID_HANDLER_EX(ID_APP_OPEN, OnAppOpen)

            CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
            CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
    		CHAIN_MSG_MAP(CToolBarHelper<CMainFrame>)
        END_MSG_MAP()

        //
        //  Misc message handlers.
        //

        LRESULT OnCreate(LPCREATESTRUCT /*lpCreateStruct*/);
        void OnDestroy();
        void OnClose();
        void OnEndSession(BOOL bEnding, UINT uLogOff);
        void OnSize(UINT nType, CSize size);
        void OnShowWindow(BOOL bShow, UINT nStatus);
        void OnInitMenuPopup(CMenuHandle menuPopup, UINT nIndex, BOOL bSysMenu);
        void OnDropFiles(HDROP hDrop);

        // CMessageFilter implementation.
        virtual BOOL PreTranslateMessage(MSG* pMsg) override;

        // CIdleHandler implementation.
        virtual BOOL OnIdle() override;

        //
        //  Main Frame functions.
        //

        // Register all our hotkeys and update the "Wallpaper" menu.
        void RegisterAllHotkeys();
        void _UpdateHotkey(CMenuHandle menu, int ID, class CHotKeyDefinition& hotKey);

        // Update window title to include name of loaded playlist.
        void UpdateWindowTitle();

        // Update main menu based on current state.
        void UpdateMainMenu();

        // Track frame window visibility.
        void UpdateWindowVisibility(bool isVisible);

        //
        //  Playlist functions.
        //

        // Load playlist.
        bool LoadPlaylist(ConstWString playlistName);

        // Change the wallpaper image and start countdown timer.
        void ChangeWallpaperImage(fs::path wallpaperPath);

        // Show solid color on desktop because no images are available.
        FORCEINLINE void ChangeWallpaperToSolidColor()
        {
            ChangeWallpaperImage(fs::path());
        }

        // Show next or previous wallpaper image.
        CFileInfo* ShowNextOrPrev(int nID);

        //
        //  Countdown timer functions.
        //

        // Handle WM_TMER message for ID_COUNTDOWN_TIMER.
        void OnCountdownTimer(UINT_PTR nID);

        // Handle WM_TMER message for ID_UI_UPDATE_TIMER.
        void OnUserInterfaceUpdateTimer(UINT_PTR nID);

        // Set the countdown timer interval.
        void SetCountdownTimerInterval();

        // Begin countdown to next wallpaper change.
        void BeginCountdown();

        // Show current countdown timer in user interface.
        void ShowCountdownTimer();

        // Have timed wallpaper changes been paused?
        FORCEINLINE bool IsPaused()
        {
            return GetAppOptions()->m_Paused;
        }

        // Pause timed wallpaper changes.
        void PauseTimedChanges();

        // Resume timed wallpaer changes.
        void ResumeTimedChanges();

        // Exit from paused state.
        // Only intended to be called by BeginCountdown().
        // All others should call ResumeTimedChanges().
        void ExitPausedState();

        // Update user interface based on current paused state.
        void UpdateForPausedState();

        // Start the user interface update timer.
        // (Only if window is visible and we're not paused.)
        void StartUserInterfaceUpdateTimer();

        // Stop the user interface update timer.
        void StopUserInterfaceUpdateTimer();

        //
        //  ListView functions.
        //

        // Called by OnCreate to create the ListView window.
        void CreateListView();

        // Clear ListView.
        void ClearListView();

        // Populate ListView with items from playlist.
        void PopulateListView(const fs::path& selectedFile = fs::path());

        // Add file to ListView.
        int AddFileToListView(CFileInfo* pFile, int nItem = INT_MAX);

        // Begin ListView update.
        void BeginListViewUpdate();

        // End ListView update.
        void EndListViewUpdate(int iSelectedItem);

        // Force all listview items to be redrawn.
        // Use after image resize mode changes.
        void RefreshListView();

        // Get the currently selected ListView item.
        // If multiple items are selected, returns the one with focus.
        int GetCurrentSelectedItem();

        // Get the currently selected file.
        // If multiple files are selected, returns the one with focus.
        fs::path GetCurrentSelectedFile();

        // Get the ListView item for the currently displayed wallpaper image.
        // Returns -1 if list doesn't contain current wallpaper image file.
        int GetCurrentWallpaperItem();

        // Get the ListView item for a particular wallpaper image.
        int GetWallpaperItem(CFileInfo* pFile);

        // Get the CFileInfo associated with a ListView item.
        FORCEINLINE CFileInfo* GetListViewItemData(int iItem)
        {
            return (CFileInfo*) m_ListView.GetItemData(iItem);
        }

        //
        //  ListView notification handlers.
        //

        LRESULT OnGetEmptyMarkup(NMHDR* phdr);
        LRESULT OnGetDispInfo(NMHDR* phdr);
        LRESULT OnGetInfoTip(NMHDR* phdr);
        LRESULT OnItemChanged(NMHDR* phdr);
        LRESULT OnRightClick(NMHDR* phdr);
        LRESULT OnDoubleClick(NMHDR* phdr);
        LRESULT OnReturn(NMHDR* phdr);

        //
        //  Toolbar functions.
        //

        // Initialize the toolbar. Called from OnCreate().
        void InitializeToolbar();

        // Update list of playlists in the toolbar.
        void UpdatePlaylistInToolbar();

        //
        //  Toolbar notification handlers.
        //

        // Handle CBN_DROPDOWN from playlist combobox in toolbar.
        LRESULT OnToolbarPlaylistComboDropDown(UINT uNotifyCode, int nID, CWindow wndCtl);

        // Handle CBN_SELCHANGE from playlist combobox in toolbar.
        LRESULT OnToolbarPlaylistComboSelChange(UINT uNotifyCode, int nID, CWindow wndCtl);

        //
        //  Tray icon notification handlers.
        //

        LRESULT OnTrayIconMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
        LRESULT OnTaskbarRestarted(UINT uMsg, WPARAM wParam, LPARAM lParam);
        LRESULT OnAppOpen(UINT uNotifyCode, int nID, CWindow wndCtl);

        //
        //  Menu command handlers (implemented in MenuCommands.cpp).
        //

#ifdef _DEBUG
        void OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl);
        void OnHotKey(int nHotKeyID, UINT uModifiers, UINT uVirtKey);
#endif

        LRESULT OnAppOptions(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnAppClose(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnAppExit(UINT uNotifyCode, int nID, CWindow wndCtl);

        LRESULT OnAppAbout(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnViewLicense(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnViewReadMe(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnCheckForUpdates(UINT uNotifyCode, int nID, CWindow wndCtl);

        LRESULT OnPlaylistManager(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnAddImage(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnRemoveImage(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnShuffle(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnCurrent(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnOpenRecent(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnSelectAll(UINT uNotifyCode, int nID, CWindow wndCtl);

        LRESULT OnShowSelected(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnShowNextPrev(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnShowRandom(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnShowSafe(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnPauseChanges(UINT uNotifyCode, int nID, CWindow wndCtl);

        LRESULT OnOpenImage(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnEditImage(UINT uNotifyCode, int nID, CWindow wndCtl);
        LRESULT OnOpenContainingFolder(UINT uNotifyCode, int nID, CWindow wndCtl);

    private:

        // Wallpaper Manager.
        CWallpaperManager& WallpaperManager;

    private:

        // Is the frame window visible? (Visible = !Minimized && !Hidden)
        // Timer-based UI updates are stopped when window isn't visible.
        bool m_WindowIsVisible;

        CPlayList m_PlayList;

        CListViewCtrl m_ListView;

        CTrayIcon m_TrayIcon;

        CToolBarCtrl m_Toolbar;
        CImageListManaged m_ToolbarImages;
        CComboBox m_Toolbar_PlaylistCombo;
        CStatic m_Toolbar_CountdownTimer;

        CMenuHandle m_PlaylistMenu;

        // We only need a small cache.  Larger caches don't
        // improve performance, and can burn a lot of memory.
        static const int ImageListCacheSize = 4;

        CImageListCache<ImageListCacheSize> m_ImageListCache;

        CImageListManaged m_ImageList;

        CSize m_ThumbnailSize;

        CCountdownTimer m_CountdownTimer;

        bool m_UserInterfaceUpdateTimerStarted;
};
