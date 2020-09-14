//////////////////////////////////////////////////////////////////////////////
//
//  CPlayListNameDlg.h
//
//  Playlist Name dialog box.
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

// Dialog used by Playlist Manager for "New" and "Rename".
class CPlayListNameDlg : public CDialogImpl<CPlayListNameDlg>
{
    public:

        // Construct playlist name dialog.
        CPlayListNameDlg(
            const std::vector<std::wstring>& AllPlaylists,
            ConstWString pwszTitle,
            ConstWString pwszHeading,
            ConstWString pwszPlaylistName = L""
        )
            :
            CDialogImpl<CPlayListNameDlg>(),
            m_AllPlaylists(AllPlaylists),
            m_pwszTitle(pwszTitle),
            m_pwszHeading(pwszHeading),
            m_PlaylistName(pwszPlaylistName)
        {
        }

        // Retrieve playlist name after DoModal() returns IDOK.
        const std::wstring& GetName()
        {
            return m_PlaylistName;
        }

    public:

        enum { IDD = IDD_PLAYLIST_NAME };

        BEGIN_MSG_MAP(CPlayListNameDlg)
            MSG_WM_INITDIALOG(OnInitDialog)
            COMMAND_ID_HANDLER_EX(IDOK, OnCloseCmd)
            COMMAND_ID_HANDLER_EX(IDCANCEL, OnCloseCmd)
            COMMAND_HANDLER_EX(IDC_NAME_EDIT, EN_CHANGE, OnEditChange)
        END_MSG_MAP()

        // Handle WM_INITDIALOG message.
        BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
        {
            CenterWindow(GetParent());

            SetWindowTextW(m_pwszTitle);
            SetDlgItemText(IDC_NAME_HEADING, m_pwszHeading);
            SetDlgItemText(IDC_NAME_EDIT, m_PlaylistName.c_str());

            // Limit playlist name to 64 characters,
            // unless the name is already larger than that.
            UINT maxNameLength = std::max((UINT)m_PlaylistName.size(), 64U);
            CEdit(GetDlgItem(IDC_NAME_EDIT)).SetLimitText(maxNameLength);

            return TRUE;
        }

        // Handle OK and Cancel buttons.
        void OnCloseCmd(UINT uNotifyCode, int nID, CWindow wndCtl)
        {
            if (nID == IDOK)
            {
                std::wstring newName = TrimWhitespace(GetWindowTextStringW(GetDlgItem(IDC_NAME_EDIT)));

                if (newName == m_PlaylistName)
                {
                    // Name hasn't changed. Return IDCLOSE instead of IDOK.
                    nID = IDCLOSE;
                }
                else
                {
                    // Name has changed. Return IDOK.
                    m_PlaylistName = std::move(newName);
                }
            }

            EndDialog(nID);
        }

        // Handle changes to edit control.
        void OnEditChange(UINT uNotifyCode, int nID, CWindow wndCtl)
        {
            std::wstring newName = TrimWhitespace(GetWindowTextStringW(GetDlgItem(IDC_NAME_EDIT)));

            bool newNameIsOK = true;

            if (newName.empty() ||
                newName.find_first_of(L"\\/:*?<>|\"") != std::wstring::npos)
            {
                // Name can't be empty, consist entirely of whitespace,
                // or include invalid filename characters.
                newNameIsOK = false;
            }
            else if (_wcsicmp(newName.c_str(), m_PlaylistName.c_str()) != 0)
            {
                // Name can't be a duplicate of another playlist name.
                for (const auto& playlistName: m_AllPlaylists)
                {
                    if (_wcsicmp(newName.c_str(), playlistName.c_str()) == 0)
                    {
                        newNameIsOK = false;
                        break;
                    }
                }
            }

            GetDlgItem(IDOK).EnableWindow(newNameIsOK);
        }

    private:

        const std::vector<std::wstring>& m_AllPlaylists;
        LPCWSTR m_pwszTitle;
        LPCWSTR m_pwszHeading;
        std::wstring m_PlaylistName;
};
