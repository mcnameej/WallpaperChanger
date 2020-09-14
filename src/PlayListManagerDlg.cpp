//////////////////////////////////////////////////////////////////////////////
//
//  PlayListManagerDlg.cpp
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

#include "precomp.h"
#include "resource.h"

#include "WallpaperChangerApp.h"

#include "PlayList.h"
#include "PlayListManagerDlg.h"
#include "PlayListNameDlg.h"

//////////////////////////////////////////////////////////////////////////////
//
//  CPlayListManagerDlg::OnInitDialog
//
//////////////////////////////////////////////////////////////////////////////

// Handle WM_INITDIALOG message.
BOOL CPlayListManagerDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    CenterWindow(GetParent());

    m_ListOfPlaylists = GetDlgItem(IDC_PLAYLISTS);
    m_ButtonsEnabled = true;

    m_AllPlaylists = CPlayList::GetAllPlaylists();

    for (const auto& playlistName: m_AllPlaylists)
    {
        int idx = m_ListOfPlaylists.AddString(playlistName.c_str());

        if (playlistName == m_CurrentPlayList.GetPlaylistName())
            m_ListOfPlaylists.SetCurSel(idx);
    }

    if (m_ListOfPlaylists.GetCurSel() == -1)
        m_ListOfPlaylists.SetCurSel(0);

    OnListSelChange(0, 0, NULL);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CPlayListManagerDlg::OnCloseCmd
//
//////////////////////////////////////////////////////////////////////////////

// Handle Close button (IDCLOSE). Also called for ESC key (IDCANCEL).
void CPlayListManagerDlg::OnCloseCmd(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    EndDialog(IDCLOSE);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CPlayListManagerDlg::OnOpenPlaylist
//
//////////////////////////////////////////////////////////////////////////////

// Handle Open button (IDC_OPEN).
void CPlayListManagerDlg::OnOpenPlaylist(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    int idx = m_ListOfPlaylists.GetCurSel();
    if (idx == -1)
        return;

    m_NewPlayList = m_AllPlaylists[idx];

    EndDialog(IDOK);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CPlayListManagerDlg::OnNewPlaylist
//
//////////////////////////////////////////////////////////////////////////////

// Handle New button (IDC_NEW).
void CPlayListManagerDlg::OnNewPlaylist(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CPlayListNameDlg newDlg(m_AllPlaylists,
                            L"New",
                            L"New playlist name...");

    if (newDlg.DoModal(m_hWnd) == IDOK)
    {
        fs::path newPath = CPlayList::NameToPath(newDlg.GetName());

        // Sanity check: Make sure there isn't already a playlist with same name.
        if (fs::exists(newPath))
        {
            // This really should never happen.
            // Name selection dialog should prevent duplicate.
            MessageBeep(MB_ICONERROR);
            DebugPrint(L"Unexpected playlist file: %s\n", newPath.c_str());
            return;
        }

        // Create empty playlist file.
        CPlayList().Save(newPath);

        // Open the new playlist.
        m_NewPlayList = newPath;
        EndDialog(IDOK);
    }
    else
    {
        GotoDlgCtrl(m_ListOfPlaylists);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  CPlayListManagerDlg::OnDeletePlaylist
//
//////////////////////////////////////////////////////////////////////////////

// Handle Delete button (IDC_DELETE).
void CPlayListManagerDlg::OnDeletePlaylist(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    int idx = m_ListOfPlaylists.GetCurSel();
    if (idx == -1)
        return;

    if (AtlMessageBox(m_hWnd,
                      L"Are you sure you want to delete the playlist?\n\n"
                      L"\"Delete or Delete Not. There is no Undo.\"\n",
                      L"I've got a bad feeling about this!",
                      MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON2) == IDYES)
    {
        std::wstring playlistName = m_AllPlaylists[idx];

        fs::path playlistPath = CPlayList::NameToPath(playlistName);
        std::error_code ec;
        fs::remove(playlistPath, ec);
        if (ec)
        {
            // This really should never happen.
            MessageBeep(MB_ICONERROR);
            DebugPrint(L"Delete failed: %s\n", playlistPath.c_str());
        }
        else
        {
            m_AllPlaylists.erase(m_AllPlaylists.begin() + idx);

            int count = m_ListOfPlaylists.DeleteString(idx);

            m_ListOfPlaylists.SetCurSel(std::min(idx, count - 1));

            OnListSelChange(0, 0, NULL);

            // Remove playlist from MRU list (if needed).
            auto it = std::find(GetAppOptions()->m_RecentPlaylists.begin(),
                                GetAppOptions()->m_RecentPlaylists.end(),
                                playlistName);
            if (it != GetAppOptions()->m_RecentPlaylists.end())
            {
                GetAppOptions()->m_RecentPlaylists.erase(it);
                GetAppOptions()->SaveToRegistry();
            }
        }
    }

    GotoDlgCtrl(m_ListOfPlaylists);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CPlayListManagerDlg::OnRenamePlaylist
//
//////////////////////////////////////////////////////////////////////////////

// Handle Rename button (IDC_RENAME).
void CPlayListManagerDlg::OnRenamePlaylist(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    int idx = m_ListOfPlaylists.GetCurSel();
    if (idx == -1)
        return;

    CPlayListNameDlg renameDlg(m_AllPlaylists,
                               L"Rename",
                               L"Edit playlist name...",
                               m_AllPlaylists[idx]);

    if (renameDlg.DoModal(m_hWnd) == IDOK)
    {
        // NOTE: IDOK implies that oldName != newName.
        // We'll get IDCLOSE if name is unchanged.

        std::wstring oldName = m_AllPlaylists[idx];
        std::wstring newName = renameDlg.GetName();

        fs::path oldPath = CPlayList::NameToPath(oldName);
        fs::path newPath = CPlayList::NameToPath(newName);

        std::error_code ec;
        fs::rename(oldPath, newPath, ec);
        if (ec)
        {
            // This really should never happen.
            MessageBeep(MB_ICONERROR);
            DebugPrint(L"Rename failed!\n");
            DebugPrint(L"From \"%s\"\n", oldPath.c_str());
            DebugPrint(L"To   \"%s\"\n", newPath.c_str());
        }
        else
        {
            m_AllPlaylists[idx] = newName;

            std::sort(m_AllPlaylists.begin(), m_AllPlaylists.end());

            auto result = std::find(m_AllPlaylists.begin(), m_AllPlaylists.end(), newName);
            ATLASSERT(result != m_AllPlaylists.end());
            int newIdx = (int)(result - m_AllPlaylists.begin());

            // Listbox can't modify existing string. Must delete/insert.
            int count = m_ListOfPlaylists.DeleteString(idx);
            idx = m_ListOfPlaylists.InsertString((newIdx < count) ? newIdx : -1, newName.c_str());

            m_ListOfPlaylists.SetCurSel(idx);

            // Update current playlist if we just renamed it.
            if (m_CurrentPlayList.GetPlaylistName() == oldName)
                m_CurrentPlayList.SetPlaylistName(newName);

            // Update "Safe" playlist if we just renamed it.
            if (GetAppOptions()->m_SafePlaylist == oldName)
            {
                GetAppOptions()->m_SafePlaylist = newName;
                GetAppOptions()->SaveToRegistry();
            }

            // Update playlist name in MRU list (if needed).
            auto it = std::find(GetAppOptions()->m_RecentPlaylists.begin(),
                                GetAppOptions()->m_RecentPlaylists.end(),
                                oldName);
            if (it != GetAppOptions()->m_RecentPlaylists.end())
            {
                *it = newName;
                GetAppOptions()->SaveToRegistry();
            }
        }
    }

    GotoDlgCtrl(m_ListOfPlaylists);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CPlayListManagerDlg::OnListSelChange
//
//////////////////////////////////////////////////////////////////////////////

// Handle listbox selection change.
void CPlayListManagerDlg::OnListSelChange(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    int idx = m_ListOfPlaylists.GetCurSel();

    bool bEnable = (idx != -1);

    if (bEnable != m_ButtonsEnabled)
    {
        m_ButtonsEnabled = bEnable;

        GetDlgItem(IDC_OPEN).EnableWindow(bEnable);
        GetDlgItem(IDC_DELETE).EnableWindow(bEnable);
        GetDlgItem(IDC_RENAME).EnableWindow(bEnable);

        SendMessage(DM_SETDEFID, bEnable ? IDC_OPEN : IDC_NEW);
    }

    if (idx != -1)
    {
        // Don't allow the current playlist to be deleted.
        GetDlgItem(IDC_DELETE).EnableWindow(m_AllPlaylists[idx] != m_CurrentPlayList.GetPlaylistName());
    }
}
