//////////////////////////////////////////////////////////////////////////////
//
//  PlayListManagerDlg.h
//
//  Playlist Manager dialog box.
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

class CPlayListManagerDlg : public CDialogImpl<CPlayListManagerDlg>
{
    public:

        CPlayListManagerDlg(
             class CPlayList& currentPlayList
        )
            :
            m_CurrentPlayList(currentPlayList),
            m_ButtonsEnabled(false)
        {
        }

        std::wstring GetNewPlayListName()
        {
            return m_NewPlayList;
        }

    public:

        enum { IDD = IDD_PLAYLIST_MANAGER };

        // NOTE: We can receive WM_COMMAND notification messages from controls
        //       before we receive WM_INITDIALOG. Command handlers must fail
        //       gracefully if called in this situation.

        BEGIN_MSG_MAP(CPlayListManagerDlg)
            MSG_WM_INITDIALOG(OnInitDialog)
            COMMAND_ID_HANDLER_EX(IDCLOSE, OnCloseCmd)
            COMMAND_ID_HANDLER_EX(IDCANCEL, OnCloseCmd)
            COMMAND_ID_HANDLER_EX(IDC_OPEN, OnOpenPlaylist)
            COMMAND_ID_HANDLER_EX(IDC_NEW, OnNewPlaylist)
            COMMAND_ID_HANDLER_EX(IDC_DELETE, OnDeletePlaylist)
            COMMAND_ID_HANDLER_EX(IDC_RENAME, OnRenamePlaylist)
            COMMAND_HANDLER_EX(IDC_PLAYLISTS, LBN_SELCHANGE, OnListSelChange)
            COMMAND_HANDLER_EX(IDC_PLAYLISTS, LBN_DBLCLK, OnOpenPlaylist)
        END_MSG_MAP()

        BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
        void OnCloseCmd(UINT uNotifyCode, int nID, CWindow wndCtl);
        void OnOpenPlaylist(UINT uNotifyCode, int nID, CWindow wndCtl);
        void OnNewPlaylist(UINT uNotifyCode, int nID, CWindow wndCtl);
        void OnDeletePlaylist(UINT uNotifyCode, int nID, CWindow wndCtl);
        void OnRenamePlaylist(UINT uNotifyCode, int nID, CWindow wndCtl);
        void OnListSelChange(UINT uNotifyCode, int nID, CWindow wndCtl);

    private:

        class CPlayList& m_CurrentPlayList;

        std::vector<std::wstring> m_AllPlaylists;

        CListBox m_ListOfPlaylists;

        bool m_ButtonsEnabled;

        std::wstring m_NewPlayList;
};
