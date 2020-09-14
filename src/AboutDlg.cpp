//////////////////////////////////////////////////////////////////////////////
//
//  AboutDlg.cpp
//
//  "About" dialog box.
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

#include "AboutDlg.h"
#include "VersionInfo.h"

BOOL CAboutDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	CenterWindow(GetParent());

    //
    // Create bold font for product name.
    //

    CFontHandle standardFont = GetFont();

    if (!standardFont.IsNull())
    {
        LOGFONT lf;
        standardFont.GetLogFont(&lf);

        lf.lfWeight = FW_BOLD;
        lf.lfHeight = (lf.lfHeight >= 0) ? (lf.lfHeight + 4) : (lf.lfHeight - 4);

        m_BoldFont.CreateFontIndirect(&lf);

        GetDlgItem(IDC_NAME).SetFont(m_BoldFont);
    }

    //
    // Update dialog controls with information from version resource.
    //

    m_CheckForUpdates.SubclassWindow(GetDlgItem(IDC_CHECK_FOR_UPDATES));

    CApplicationVersionInfo AppVersion;

    if (AppVersion.IsOK())
    {
        SetDlgItemText(IDC_NAME, AppVersion.GetProductName());
        SetDlgItemText(IDC_VERSION, (std::wstring(L"Version ") + AppVersion.GetProductVersion()).c_str());
        SetDlgItemText(IDC_COPYRIGHT, AppVersion.GetLegalCopyright());

        int Major, Minor, Build, Patch;
        AppVersion.GetProductVersion(&Major, &Minor, &Build, &Patch);

        auto strURL = Format(LoadStringResourceW(IDS_UPDATE_URL), Major, Minor, Build, Patch);

        m_CheckForUpdates.SetHyperLink(strURL.c_str());
    }

    //
    // Set focus to the OK button (don't want focus on link control).
    //

    GetDlgItem(IDOK).SetFocus();

    return FALSE;  // FALSE == OnInitDialog has manually set the focus.
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}
