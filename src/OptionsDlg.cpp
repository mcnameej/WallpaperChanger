//////////////////////////////////////////////////////////////////////////////
//
//  OptionsDlg.cpp
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

#include "precomp.h"
#include "resource.h"

#include "WallpaperChangerApp.h"

#include "Options.h"
#include "OptionsDlg.h"
#include "WallpaperManager.h"
#include "PlayList.h"

//////////////////////////////////////////////////////////////////////////////
//
//  Static data
//
//////////////////////////////////////////////////////////////////////////////

#define MAX_INTERVAL_HOURS 499

struct IntervalOption
{
    LPCWSTR m_Name;
    int m_Hours;
    int m_Minutes;
};

static IntervalOption s_IntervalOptions[] =
{
    { L"2 minutes",   0,   2 },
    { L"5 minutes",   0,   5 },
    { L"10 minutes",  0,  10 },
    { L"15 minutes",  0,  15 },
    { L"30 minutes",  0,  30 },
    { L"1 hour",      1,   0 },
    { L"6 hours",     6,   0 },
    { L"1 day",      24,   0 }
};

static UINT s_ValidHotkeyModifiers[] =
{
    MOD_CONTROL | MOD_ALT,
    MOD_CONTROL | MOD_SHIFT,
    MOD_CONTROL | MOD_SHIFT | MOD_ALT,
    MOD_WIN,
    MOD_SHIFT | MOD_WIN,
    MOD_ALT | MOD_WIN,
    MOD_CONTROL | MOD_WIN,
    MOD_CONTROL | MOD_ALT | MOD_WIN,
};

//////////////////////////////////////////////////////////////////////////////
//
//  COptionsDlg::OnInitDialog
//
//////////////////////////////////////////////////////////////////////////////

BOOL COptionsDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    CenterWindow(GetParent());

    //
    // Initialize all control objects.
    //

    m_IntervalCombo       = GetDlgItem(IDC_INTERVAL_COMBO);
    m_IntervalHoursEdit   = GetDlgItem(IDC_INTERVAL_HOURS);
    m_IntervalMinutesEdit = GetDlgItem(IDC_INTERVAL_MINUTES);
    m_IntervalHoursSpin   = GetDlgItem(IDC_HOURS_SPIN);
    m_IntervalMinutesSpin = GetDlgItem(IDC_MINUTES_SPIN);
    m_ResizeModeCombo     = GetDlgItem(IDC_RESIZE_MODE_COMBO);
    m_SafePlaylistCombo   = GetDlgItem(IDC_SAFE_PLAYLIST_COMBO);
    m_AutoStartCombo      = GetDlgItem(IDC_AUTOSTART_COMBO);

    m_BackgroundColorSelector.SubclassWindow(GetDlgItem(IDC_BG_COLOR_COMBO));

    //
    // Initialize change interval.
    //

    // Initialize combobox with "standard" intervals.

    int intervalHours = std::min(GetAppOptions()->m_ChangeInterval / 60, MAX_INTERVAL_HOURS);
    int intervalMinutes = GetAppOptions()->m_ChangeInterval % 60;

    for (auto const& interval: s_IntervalOptions)
    {
        int item = m_IntervalCombo.AddString(interval.m_Name);
        if (intervalHours == interval.m_Hours && intervalMinutes == interval.m_Minutes)
            m_IntervalCombo.SetCurSel(item);
    }
    int item = m_IntervalCombo.AddString(L"Custom");
    if (m_IntervalCombo.GetCurSel() == -1)
        m_IntervalCombo.SetCurSel(item);

    // Initialize edit controls with with "custom" interval.

    m_CustomIntervalValid = GetAppOptions()->m_ChangeInterval != 0;

    m_IntervalHoursEdit.SetLimitText(4);
    m_IntervalMinutesEdit.SetLimitText(2);

    m_IntervalHoursSpin.SetRange(0, MAX_INTERVAL_HOURS);
    m_IntervalMinutesSpin.SetRange(0, 59);

    SetDlgItemInt(IDC_INTERVAL_HOURS, intervalHours);
    SetDlgItemInt(IDC_INTERVAL_MINUTES, intervalMinutes);

    // Enable/disable custom interval edit controls based on combobox selection.

    OnIntervalComboSelChanged(0, 0, NULL);

    //
    // Initialize resize mode.
    //

    m_ResizeModeCombo.AddString(L"Center");     // RESIZE_Center
    m_ResizeModeCombo.AddString(L"Tile");       // RESIZE_Tile
    m_ResizeModeCombo.AddString(L"Stretch");    // RESIZE_Stretch
    m_ResizeModeCombo.AddString(L"Fit");        // RESIZE_Fit
    m_ResizeModeCombo.AddString(L"Fill");       // RESIZE_Fill

    m_ResizeModeCombo.SetCurSel(GetWallpaperManager()->GetResizeMode());

    //
    // Initialize background color.
    //

    m_BackgroundColorSelector.SetColor(GetWallpaperManager()->GetBackgroundColor());
    // m_BackgroundColorSelector.SetDefaultColor(GetWallpaperManager()->GetBackgroundColor());
    m_BackgroundColorSelector.SetDefaultText(L"");

    //
    // Initialize safe playlist combo.
    //

    m_SafePlaylistCombo.AddString(L"(None)");
    m_SafePlaylistCombo.SetCurSel(0);

    m_AllPlaylists = CPlayList::GetAllPlaylists();

    for (const auto& playlistName: m_AllPlaylists)
    {
        int idx = m_SafePlaylistCombo.AddString(playlistName.c_str());

        if (playlistName == GetAppOptions()->m_SafePlaylist)
            m_SafePlaylistCombo.SetCurSel(idx);
    }

    //
    // Initialize autostart.
    //

    m_AutoStartCombo.AddString(L"Do not start automatically");
    m_AutoStartCombo.AddString(L"Start at every login");
    m_AutoStartCombo.SetCurSel(GetApp()->IsAutoStartEnabled() ? 1 : 0);

    //
    // Initialize hotkeys.
    //

    InitializeHotKeyControls(GetAppOptions()->m_NextImageHotkey,
                             IDC_NEXT_HOTKEY_ENABLED, m_NextImageHotkeyEnabled,
                             IDC_NEXT_HOTKEY_MODIFIER, m_NextImageHotkeyModifier,
                             IDC_NEXT_HOTKEY_VK, m_NextImageHotkeyVK);

    InitializeHotKeyControls(GetAppOptions()->m_PrevImageHotkey,
                             IDC_PREV_HOTKEY_ENABLED, m_PrevImageHotkeyEnabled,
                             IDC_PREV_HOTKEY_MODIFIER, m_PrevImageHotkeyModifier,
                             IDC_PREV_HOTKEY_VK, m_PrevImageHotkeyVK);

    InitializeHotKeyControls(GetAppOptions()->m_RandomImageHotkey,
                             IDC_RANDOM_HOTKEY_ENABLED, m_RandomImageHotkeyEnabled,
                             IDC_RANDOM_HOTKEY_MODIFIER, m_RandomImageHotkeyModifier,
                             IDC_RANDOM_HOTKEY_VK, m_RandomImageHotkeyVK);

    InitializeHotKeyControls(GetAppOptions()->m_SafePlaylistHotkey,
                             IDC_SAFE_HOTKEY_ENABLED, m_SafePlaylistHotkeyEnabled,
                             IDC_SAFE_HOTKEY_MODIFIER, m_SafePlaylistHotkeyModifier,
                             IDC_SAFE_HOTKEY_VK, m_SafePlaylistHotkeyVK);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
//  COptionsDlg::OnCloseCmd
//
//////////////////////////////////////////////////////////////////////////////

void COptionsDlg::OnCloseCmd(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    if (nID == IDOK)
    {
        //
        // Update change interval.
        //

        int selection = m_IntervalCombo.GetCurSel();

        if (selection >= 0 && selection < ARRAYSIZE(s_IntervalOptions))
        {
            GetAppOptions()->m_ChangeInterval = s_IntervalOptions[selection].m_Hours * 60 +
                                                s_IntervalOptions[selection].m_Minutes;
        }
        else if (selection == ARRAYSIZE(s_IntervalOptions))
        {
            int changeInterval = std::min(GetDlgItemInt(IDC_INTERVAL_HOURS), (UINT)MAX_INTERVAL_HOURS) * 60 +
                                 GetDlgItemInt(IDC_INTERVAL_MINUTES);
            if (changeInterval != 0)
                GetAppOptions()->m_ChangeInterval = changeInterval;
        }

        //
        // Update resize mode.
        //

        selection = m_ResizeModeCombo.GetCurSel();

        if (selection != -1)
            GetWallpaperManager()->SetResizeMode((WallpaperResizeMode)selection);

        //
        // Update background color.
        //

        GetWallpaperManager()->SetBackgroundColor(m_BackgroundColorSelector.GetColor());

        //
        // Update safe playlist.
        //

        selection = m_SafePlaylistCombo.GetCurSel();

        if (selection == 0)
        {
            // User seelected the "(None)" option.
            GetAppOptions()->m_SafePlaylist.clear();
        }
        else
        {
            // User selected a playlist.
            GetAppOptions()->m_SafePlaylist = m_AllPlaylists[selection-1];
        }

        //
        // Update hotkeys.
        //

        UpdateHotKey(GetAppOptions()->m_NextImageHotkey,
                     m_NextImageHotkeyEnabled,
                     m_NextImageHotkeyModifier,
                     m_NextImageHotkeyVK);

        UpdateHotKey(GetAppOptions()->m_PrevImageHotkey,
                     m_PrevImageHotkeyEnabled,
                     m_PrevImageHotkeyModifier,
                     m_PrevImageHotkeyVK);

        UpdateHotKey(GetAppOptions()->m_RandomImageHotkey,
                     m_RandomImageHotkeyEnabled,
                     m_RandomImageHotkeyModifier,
                     m_RandomImageHotkeyVK);

        UpdateHotKey(GetAppOptions()->m_SafePlaylistHotkey,
                     m_SafePlaylistHotkeyEnabled,
                     m_SafePlaylistHotkeyModifier,
                     m_SafePlaylistHotkeyVK);

        //
        // Save options to registry.
        //

        GetAppOptions()->SaveToRegistry();

        GetWallpaperManager()->SaveToRegistry();

        //
        // Update autostart.
        //

        int originalAutoStartSelection = GetApp()->IsAutoStartEnabled() ? 1 : 0;
        int newAutoStartSelection = m_AutoStartCombo.GetCurSel();

        if (newAutoStartSelection != originalAutoStartSelection)
        {
            if (newAutoStartSelection == 0)
                GetApp()->DisableAutoStart();
            else
                GetApp()->EnableAutoStart();
        }
    }

    EndDialog(nID);
}

//////////////////////////////////////////////////////////////////////////////
//
//  COptionsDlg::OnIntervalComboSelChanged
//
//////////////////////////////////////////////////////////////////////////////

void COptionsDlg::OnIntervalComboSelChanged(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    if (!m_IntervalCombo.m_hWnd)
        return;

    int selection = m_IntervalCombo.GetCurSel();

    if (selection != -1)
    {
        bool bIsCustom = (selection == ARRAYSIZE(s_IntervalOptions));

        m_IntervalHoursEdit.EnableWindow(bIsCustom);
        m_IntervalMinutesEdit.EnableWindow(bIsCustom);

        // Handle changing from invalid custom interval to a standard interval.
        if (!bIsCustom && !m_CustomIntervalValid)
        {
            SetDlgItemInt(IDC_INTERVAL_HOURS, s_IntervalOptions[selection].m_Hours);
            SetDlgItemInt(IDC_INTERVAL_MINUTES, s_IntervalOptions[selection].m_Minutes);
            OnIntervalEditChanged(0, 0, NULL);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  COptionsDlg::OnIntervalEditChanged
//
//////////////////////////////////////////////////////////////////////////////

void COptionsDlg::OnIntervalEditChanged(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    if (!m_IntervalHoursEdit.m_hWnd || !m_IntervalMinutesEdit.m_hWnd)
        return;

    // Don't allow custom interval to be set to zero,
    // or for hours to be set above maximum.
    UINT hours = GetDlgItemInt(IDC_INTERVAL_HOURS);
    UINT minutes = GetDlgItemInt(IDC_INTERVAL_MINUTES);
    if ((!hours && !minutes) || (hours > MAX_INTERVAL_HOURS))
    {
        if (m_CustomIntervalValid)
        {
            // Change to invalid.
            m_CustomIntervalValid = false;
            m_IntervalHoursEdit.RedrawWindow();
            m_IntervalMinutesEdit.RedrawWindow();
            GetDlgItem(IDOK).EnableWindow(FALSE);
        }
    }
    else
    {
        if (!m_CustomIntervalValid)
        {
            // Change to valid.
            m_CustomIntervalValid = true;
            m_IntervalHoursEdit.RedrawWindow();
            m_IntervalMinutesEdit.RedrawWindow();
            GetDlgItem(IDOK).EnableWindow(TRUE);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  COptionsDlg::OnCtlColorEdit
//
//////////////////////////////////////////////////////////////////////////////

HBRUSH COptionsDlg::OnCtlColorEdit(CDCHandle dc, CEdit edit)
{
    if (edit == m_IntervalHoursEdit || edit == m_IntervalMinutesEdit)
    {
        if (!m_CustomIntervalValid)
        {
            COLORREF errorBackgroundColor = RGB(255,0,0); // Red background for error.
            dc.SetBkColor(errorBackgroundColor);
            dc.SetDCBrushColor(errorBackgroundColor);
            dc.SetTextColor(RGB(255,255,255));
            return (HBRUSH) ::GetStockObject(DC_BRUSH);
        }
    }

    // Use default color.
    SetMsgHandled(FALSE);
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////
//
//  COptionsDlg::InitializeHotKeyControls
//
//////////////////////////////////////////////////////////////////////////////

void
COptionsDlg::InitializeHotKeyControls(
    CHotKeyDefinition& hotKey,
    int CheckboxID, CButton& EnableCheckbox,
    int ModifierID, CComboBox& ModiferCombo,
    int VKID, CComboBox& VKCombo
)
{
    EnableCheckbox = GetDlgItem(CheckboxID);

    EnableCheckbox.SetCheck(hotKey.m_Enabled);

    ModiferCombo = GetDlgItem(ModifierID);

    for (auto modifierBits: s_ValidHotkeyModifiers)
    {
        std::wstring strModifier = CHotKeyDefinition(0xFFFF, modifierBits).ToString();

        int idx = ModiferCombo.AddString(strModifier.c_str());

        if (hotKey.m_Modifiers == modifierBits)
            ModiferCombo.SetCurSel(idx);
    }

    VKCombo = GetDlgItem(VKID);

    for (auto vk: m_ValidVirtKeys)
    {
        std::wstring strVirtKey = CHotKeyDefinition(vk, 0).ToString();

        int idx = VKCombo.AddString(strVirtKey.c_str());

        if (hotKey.m_VirtKey == vk)
            VKCombo.SetCurSel(idx);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  COptionsDlg::UpdateHotKey
//
//////////////////////////////////////////////////////////////////////////////

void
COptionsDlg::UpdateHotKey(
    CHotKeyDefinition& hotKey,
    CButton EnableCheckbox,
    CComboBox ModiferCombo,
    CComboBox VKCombo
)
{
    hotKey.m_Enabled = EnableCheckbox.GetCheck();

    int selection = ModiferCombo.GetCurSel();
    if (selection >= 0 && selection < ARRAYSIZE(s_ValidHotkeyModifiers))
        hotKey.m_Modifiers = s_ValidHotkeyModifiers[selection];

    selection = VKCombo.GetCurSel();
    if (selection >= 0 && selection < ARRAYSIZE(m_ValidVirtKeys))
        hotKey.m_VirtKey = m_ValidVirtKeys[selection];
}
