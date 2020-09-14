//////////////////////////////////////////////////////////////////////////////
//
//  TryIcon.h
//
//  CTrayIcon and CTrayIconMenuHelper classes.
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

//////////////////////////////////////////////////////////////////////////////
//
//  Tray icon notification message
//
//////////////////////////////////////////////////////////////////////////////

#define WM_TRAYICON (WM_USER + 1)

//////////////////////////////////////////////////////////////////////////////
//
//  CTrayIcon class
//
//////////////////////////////////////////////////////////////////////////////

class CTrayIcon
{
    public:

        // Construct CTrayIcon object.
        CTrayIcon()
        {
            m_IsCreated = false;

            m_NotifyData = { sizeof(m_NotifyData) };

            m_NotifyData.uID = 1;

            m_NotifyData.uCallbackMessage = WM_TRAYICON;

            m_NotifyData.uVersion = NOTIFYICON_VERSION_4;

            StringCchCopyW(m_NotifyData.szTip,
                           ARRAYSIZE(m_NotifyData.szTip),
                           AtlGetStringPtr(IDR_MAINFRAME));

            // Don't use NIF_GUID. Windows will associate the GUID with the full
            // path of the executable the first time it is used. Renaming or moving
            // the executable after that breaks the tray icon, and there's no easy
            // way to fix it.

            // Get message that shell broadcasts when taskbar is restarted.
            m_TaskbarRestartMessage = ::RegisterWindowMessageW(L"TaskbarCreated");
        }

        // Destroy CTrayIcon object.
        ~CTrayIcon()
        {
            Destroy();
        }

        // No copy ctor.
        CTrayIcon(const CTrayIcon&) = delete;

        // No copy assignment.
        CTrayIcon& operator=(const CTrayIcon&) = delete;

    public:

        // Create the tray icon.
        BOOL
        Create(
            HWND hNotifyWnd = NULL
        )
        {
            // Must have HWND as parameter or saved from prior Create() call.
            if (!hNotifyWnd && !m_NotifyData.hWnd)
            {
                ATLASSERT(false);
                return FALSE;
            }

            // If there is an existing tray icon,
            // destroy it before re-creating it.
            if (m_IsCreated)
                Destroy();

            m_NotifyData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;

            m_NotifyData.dwState = 0;
            m_NotifyData.dwStateMask = 0;

            // m_NotifyData.hIcon = AtlLoadIcon(IDR_MAINFRAME);
            m_NotifyData.hIcon = AtlLoadIconImage(IDR_MAINFRAME,
                                                  LR_DEFAULTCOLOR,
                                                  GetSystemMetrics(SM_CXSMICON),
                                                  GetSystemMetrics(SM_CYSMICON));

            if (hNotifyWnd != NULL)
                m_NotifyData.hWnd = hNotifyWnd;

            BOOL bOK = ::Shell_NotifyIconW(NIM_ADD, &m_NotifyData);

            if (bOK)
                ::Shell_NotifyIconW(NIM_SETVERSION, &m_NotifyData);

            // m_NotifyData.hIcon is only needed for NIM_ADD.
            // Windows makes a copy, so we can destroy the icon we loaded.
            ::DestroyIcon(m_NotifyData.hIcon);
            m_NotifyData.hIcon = NULL;

            m_IsCreated = bOK;

            return bOK;
        }

        // Destroy the tray icon.
        void
        Destroy()
        {
            if (m_IsCreated)
            {
                ::Shell_NotifyIconW(NIM_DELETE, &m_NotifyData);
                m_IsCreated = false;
            }
        }

        // Show the tray icon.
        BOOL
        Show(
            bool showIcon = true
        )
        {
            if (!m_IsCreated)
                return FALSE;

            m_NotifyData.uFlags = NIF_STATE;

            if (showIcon)
                m_NotifyData.dwState &= ~NIS_HIDDEN;
            else
                m_NotifyData.dwState |= NIS_HIDDEN;

            m_NotifyData.dwStateMask = NIS_HIDDEN;

            return ::Shell_NotifyIconW(NIM_MODIFY, &m_NotifyData);
        }

        // Hide the tray icon.
        BOOL
        Hide()
        {
            return Show(false);
        }

    public:

        bool m_IsCreated;

        NOTIFYICONDATAW m_NotifyData;

        UINT m_TaskbarRestartMessage;
};

//////////////////////////////////////////////////////////////////////////////
//
//  CTrayIconMenuHelper class
//
//////////////////////////////////////////////////////////////////////////////

using CTrayIconMenuHelperTraits = CWinTraits<WS_POPUP | WS_DISABLED, 0>;

// Kludge to work around TrackPopupMenu() braindamage.
class CTrayIconMenuHelper : public CWindowImpl<CTrayIconMenuHelper, CWindow, CTrayIconMenuHelperTraits>
{
    public:

        DECLARE_WND_CLASS_EX(L"Tray Icon Menu Helper", 0, -1)

        DECLARE_EMPTY_MSG_MAP()

        // Create hidden window to help with tray icon menu.
        HWND
        Create()
        {
            using BaseClass = CWindowImpl<CTrayIconMenuHelper, CWindow, CTrayIconMenuHelperTraits>;
            CRect rcEmpty(0,0,0,0);
            return BaseClass::Create(NULL, rcEmpty, L"");
        }

        // Destroy tray icon menu helper.
        void
        Destroy()
        {
            if (m_hWnd)
                DestroyWindow();
        }

        // Display popup menu and allow user to make a selection.
        // Doesn't return until user makes selection or cancels menu.
        // Posts WM_COMMAND to hAppWnd with the selected menu item.
        int
        DoPopupMenu(
            HWND hAppWnd,
            HMENU hMenu,
            UINT nFlags = TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON
        )
        {
            ATLASSERT(::IsWindow(m_hWnd));
            ATLASSERT(::IsWindow(hAppWnd));
            ATLASSERT(hMenu != NULL);

            // Make our hidden window the "foreground window".
            // I know this sounds like a strange thing to do,
            // but it is required by the TrackPopupMenu API.
            ::SetForegroundWindow(m_hWnd);

            // Display popup menu and allow user to make a selection.
            POINT pt;
            ::GetCursorPos(&pt);
            BOOL result = ::TrackPopupMenu(hMenu,
                                           nFlags | TPM_NONOTIFY | TPM_RETURNCMD | TPM_NOANIMATION,
                                           pt.x, pt.y,
                                           0,
                                           m_hWnd,
                                           NULL);

            if (result != 0)
            {
                // Post WM_COMMAND to app window with selected menu item.
                ::PostMessageW(hAppWnd, WM_COMMAND, (WPARAM) result, 0);
            }
            else
            {
                // No menu item was selected, but we need to post some
                // message to the app window (bug in TrackPopupMenu API).
                ::PostMessageW(hAppWnd, WM_NULL, 0, 0);
            }

            return result;
        }
};
