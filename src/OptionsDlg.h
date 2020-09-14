//////////////////////////////////////////////////////////////////////////////
//
//  OptionsDlg.h
//
//  "Options" dialog box.
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

#include "Options.h"
#include "ColorButton.h"

class COptionsDlg : public CDialogImpl<COptionsDlg>
{
    public:

        COptionsDlg()
        {
            // Initialize list of valid Virtual Key codes.

            UINT* pVK = m_ValidVirtKeys;

            for (UINT vk = 'A'; vk <= 'Z'; vk++)
                *pVK++ = vk;

            for (UINT vk = '1'; vk <= '9'; vk++)
                *pVK++ = vk;
            *pVK++ = '0';

            for (UINT vk = VK_F1; vk <= VK_F12; vk++)
                *pVK++ = vk;
        }

        enum { IDD = IDD_OPTIONS };

        // NOTE: We can receive WM_COMMAND notification messages from controls
        //       before we receive WM_INITDIALOG. Command handlers must fail
        //       gracefully if called in this situation.

        BEGIN_MSG_MAP(COptionsDlg)
            MSG_WM_INITDIALOG(OnInitDialog)
            MSG_WM_CTLCOLOREDIT(OnCtlColorEdit)
            COMMAND_ID_HANDLER_EX(IDOK, OnCloseCmd)
            COMMAND_ID_HANDLER_EX(IDCANCEL, OnCloseCmd)
            COMMAND_HANDLER_EX(IDC_INTERVAL_COMBO, CBN_SELCHANGE, OnIntervalComboSelChanged)
            COMMAND_HANDLER_EX(IDC_INTERVAL_HOURS, EN_CHANGE, OnIntervalEditChanged)
            COMMAND_HANDLER_EX(IDC_INTERVAL_MINUTES, EN_CHANGE, OnIntervalEditChanged)
            REFLECT_NOTIFICATIONS()
        END_MSG_MAP()

        BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
        void OnCloseCmd(UINT uNotifyCode, int nID, CWindow wndCtl);
        void OnIntervalComboSelChanged(UINT uNotifyCode, int nID, CWindow wndCtl);
        void OnIntervalEditChanged(UINT uNotifyCode, int nID, CWindow wndCtl);
        HBRUSH OnCtlColorEdit(CDCHandle dc, CEdit edit);

    private:

        void
        InitializeHotKeyControls(
            CHotKeyDefinition& hotKey,
            int CheckboxID, CButton& EnableCheckbox,
            int ModifierID, CComboBox& ModiferCombo,
            int VKID, CComboBox& VKCombo
        );

        void
        UpdateHotKey(
            CHotKeyDefinition& hotKey,
            CButton EnableCheckbox,
            CComboBox ModiferCombo,
            CComboBox VKCombo
        );

    private:

        bool m_CustomIntervalValid;

        CComboBox m_IntervalCombo;
        CEdit m_IntervalHoursEdit;
        CEdit m_IntervalMinutesEdit;
        CUpDownCtrl m_IntervalHoursSpin;
        CUpDownCtrl m_IntervalMinutesSpin;

        CComboBox m_ResizeModeCombo;

        CColorButton m_BackgroundColorSelector;

        std::vector<std::wstring> m_AllPlaylists;
        CComboBox m_SafePlaylistCombo;

        CComboBox m_AutoStartCombo;

        CButton m_NextImageHotkeyEnabled;
        CComboBox m_NextImageHotkeyModifier;
        CComboBox m_NextImageHotkeyVK;

        CButton m_PrevImageHotkeyEnabled;
        CComboBox m_PrevImageHotkeyModifier;
        CComboBox m_PrevImageHotkeyVK;

        CButton m_RandomImageHotkeyEnabled;
        CComboBox m_RandomImageHotkeyModifier;
        CComboBox m_RandomImageHotkeyVK;

        CButton m_SafePlaylistHotkeyEnabled;
        CComboBox m_SafePlaylistHotkeyModifier;
        CComboBox m_SafePlaylistHotkeyVK;

        UINT m_ValidVirtKeys[26 + 10 + 12];
};
