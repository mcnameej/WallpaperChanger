//////////////////////////////////////////////////////////////////////////////
//
//  MF.TrayIcon.cpp
//
//  CMainFrame tray icon functions and notification handlers.
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
//  CMainFrame::OnTrayIconMsg
//
//////////////////////////////////////////////////////////////////////////////

// Handle WM_TRAYICON messages.
LRESULT CMainFrame::OnTrayIconMsg(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam)
{
    // We may be called recursively if user clicks on tray icon faster
    // than we can process the messages.  Just ignore recursive calls.

    static long s_InProgress = 0;

    if (InterlockedIncrement(&s_InProgress) == 1)
    {
        switch (LOWORD(lParam))
        {
            case WM_LBUTTONDBLCLK:
            {
                DebugPrintCmdSpew("Tray icon: LBUTTONDBLCLK\n");
                OnAppOpen(0, 0, NULL);
                break;
            }

            case WM_CONTEXTMENU:
            {
                DebugPrintCmdSpew("Tray icon: WM_CONTEXTMENU\n");

                CMenu contextMenu = AtlLoadMenu(IDM_TRAY);
                CMenuHandle menuItems = contextMenu.GetSubMenu(0);

                menuItems.SetMenuDefaultItem(ID_APP_OPEN);

                if (IsPaused())
                    menuItems.CheckMenuItem(ID_WALLPAPER_PAUSE, MF_BYCOMMAND | MF_CHECKED);

                if (GetAppOptions()->m_SafePlaylist.empty())
                    menuItems.EnableMenuItem(ID_WALLPAPER_SAFE, MF_BYCOMMAND | MF_DISABLED);

                CTrayIconMenuHelper trayIconMenu;
                trayIconMenu.Create();
                trayIconMenu.DoPopupMenu(m_hWnd, menuItems);
                trayIconMenu.Destroy();

                break;
            }
        }
    }

    InterlockedDecrement(&s_InProgress);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnTaskbarRestarted
//
//////////////////////////////////////////////////////////////////////////////

// Handle message sent by shell when taskbar is restarted.
LRESULT CMainFrame::OnTaskbarRestarted(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // Sanity check: Make sure we obtained the taskbar restart message.
    // We don't want to mistakenly handle WM_NULL messages here (uMsg == 0).
    if (m_TrayIcon.m_TaskbarRestartMessage != 0)
    {
        SetMsgHandled(FALSE);
        return 0;
    }

    DebugPrintCmdSpew("Tray icon: Restart\n");

    m_TrayIcon.Destroy();
    m_TrayIcon.Create();

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnAppOpen
//
//////////////////////////////////////////////////////////////////////////////

// Handle ID_APP_OPEN command.
LRESULT CMainFrame::OnAppOpen(UINT uNotifyCode, int nID, CWindow wndCtl)
{
    DebugPrintCmdSpew("ID_APP_OPEN\n");

    if (IsIconic())
    {
        // Restore minimized window.
        ShowWindow(SW_RESTORE);
    }
    else if (!IsWindowVisible())
    {
        // Show hidden window.
        ShowWindow(SW_SHOW);
    }

    // Assume we will activate the main frame window.
    HWND hWindowToActivate = m_hWnd;

    // If a modal dialog is open, activate the dialog, not frame window.
    if (!IsWindowEnabled())
    {
        HWND hOpenDialog = GetWindow(GW_ENABLEDPOPUP);
        if (hOpenDialog != NULL)
            hWindowToActivate = hOpenDialog;
    }

    // Activate the window and give it focus.
    ::SetForegroundWindow(hWindowToActivate);
    ::BringWindowToTop(hWindowToActivate);
    ::SetFocus(hWindowToActivate);

    return 0;
}
