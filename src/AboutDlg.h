//////////////////////////////////////////////////////////////////////////////
//
//  AboutDlg.h
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

#pragma once

class CAboutDlg : public CDialogImpl<CAboutDlg>
{
    public:
	    enum { IDD = IDD_ABOUTBOX };

	    BEGIN_MSG_MAP(CAboutDlg)
            MSG_WM_INITDIALOG(OnInitDialog)
		    COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		    COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	    END_MSG_MAP()

        BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	    LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    private:

        CHyperLink m_CheckForUpdates;
        CFont m_BoldFont;
};
