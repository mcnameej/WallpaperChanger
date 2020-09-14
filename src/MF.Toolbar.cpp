//////////////////////////////////////////////////////////////////////////////
//
//  MF.Toolbar.cpp
//
//  CMainFrame toolar functions and notification handlers.
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

//****************************************************************************
//
//  Local functions
//
//****************************************************************************

// Get the minimum toolbar button height (in pixels).
static
int
GetMinimumToolbarButtonHeight(
    HWND hWnd
)
{
    //
    // Get font height.
    //

    CFontHandle font = CWindow(hWnd).GetFont();

    if (font.IsNull())
        font = (HFONT) ::GetStockObject(SYSTEM_FONT);

    CLogFont lf;
    font.GetLogFont(lf);

    int fontHeight = (int) abs(lf.lfHeight);

    //
    // Get combobox height.
    //

    CComboBox combo;
    CRect rcCombo;

    combo.Create(hWnd, NULL, NULL, WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST, 0, 42);
    combo.GetWindowRect(rcCombo);
    combo.DestroyWindow();

    int comboBoxHeight = rcCombo.Height();

    //
    // Calculate minimum toolbar button height.
    //

    int minimumButtonHeight = std::max(fontHeight, comboBoxHeight);

    return minimumButtonHeight;
}

// Add toolbar button.
static
FORCEINLINE
BOOL
AddToolbarButton(
    CToolBarCtrl& toolbar,
    int iCommand,
    int iBitmap,
    BYTE Style = BTNS_BUTTON,
    BYTE State = TBSTATE_ENABLED,
    _U_STRINGorID label = 0U
)
{
    return toolbar.AddButton(iCommand, Style, State, iBitmap, label.m_lpstr, 0);
}

// Add toolbar button placeholder.
// Will be replaced by a control (e.g. combobox).
static
FORCEINLINE
BOOL
AddToolbarPlaceholder(
    CToolBarCtrl& toolbar,
    int iCommand
)
{
    // Assumes an empty bitmap has been added as image #0.
    // Note that TBSTATE_ENABLED is NOT specified, so button is disabled.
    return toolbar.AddButton(iCommand, BTNS_BUTTON, 0, 0, nullptr, 0);
}

// Add toolbar separator.
static
FORCEINLINE
BOOL
AddToolbarSeparator(
    CToolBarCtrl& toolbar
)
{
    return toolbar.AddSeparator();
}

//****************************************************************************
//
//  Toolbar functions
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::InitializeToolbar
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::InitializeToolbar()
{
    //
    // Get size of toolbar images.
    //

    int tbImageWidth  = 32; // ::GetSystemMetrics(SM_CXSMICON);
    int tbImageHeight = 32; // ::GetSystemMetrics(SM_CYSMICON);

    //
    // Create image list.
    //

    const int expectedNumberOfButtons = 11;

    m_ToolbarImages.Create(tbImageWidth,
                           tbImageHeight,
                           ILC_COLOR32 | ILC_MASK,
                           expectedNumberOfButtons + 1,
                           1);

    // Add an empty bitmap as the first image.
    // Used for placeholder buttons.
    {
        CBitmap emptyBitmap;
        emptyBitmap.CreateBitmap(tbImageWidth, tbImageHeight, 1, 32, nullptr);
        int index = m_ToolbarImages.Add(emptyBitmap, (COLORREF) 0);
        ATLASSERT(index == 0);
    }

    //
    // Create toolbar.
    //

    RECT rcToolbar = { 0, 0, 100, 100 };

    m_Toolbar.Create(m_hWnd, rcToolbar, NULL,
                     ATL_SIMPLE_TOOLBAR_PANE_STYLE_EX,
                     TBSTYLE_EX_MIXEDBUTTONS);

    m_Toolbar.SetButtonStructSize();

    m_Toolbar.SetImageList(m_ToolbarImages);

    UIAddToolBar(m_Toolbar);

    //
    // Set toolbar button size.
    // Enforce minimum size and set spacing.
    //

    SIZE buttonSize =
    {
        /* X */ tbImageWidth,
        /* Y */ std::max(tbImageHeight, GetMinimumToolbarButtonHeight(m_hWnd))
    };

    m_Toolbar.SetButtonSize(buttonSize);

    TBMETRICS tbMetrics = { sizeof(TBMETRICS) };
    tbMetrics.dwMask = TBMF_BUTTONSPACING;
    tbMetrics.cxButtonSpacing = 4;
    tbMetrics.cyButtonSpacing = 0;
    m_Toolbar.SetMetrics(&tbMetrics);

    //
    // Add toolbar buttons.
    //

    // Lambda to add toolbar button using an icon as the image.
    auto AddButton =
        [this, tbImageWidth, tbImageHeight]
        (UINT ID, BYTE style = BTNS_BUTTON)
        {
            HICON hIcon = AtlLoadIconImage(ID, LR_SHARED, tbImageWidth, tbImageHeight);
            int imageIndex = hIcon ? m_ToolbarImages.AddIcon(hIcon) : 0;
            return AddToolbarButton(m_Toolbar, ID, imageIndex, style);
        };

    AddButton(ID_WALLPAPER_PREV);
    AddButton(ID_WALLPAPER_NEXT);
    AddButton(ID_WALLPAPER_RANDOM);
    AddButton(ID_PLAYLIST_CURRENT);
    AddToolbarPlaceholder(m_Toolbar, IDC_COUNTDOWN_TIMER);
    AddButton(ID_WALLPAPER_PAUSE, BTNS_CHECK);

    AddToolbarSeparator(m_Toolbar);

    AddButton(ID_PLAYLIST_ADD);
    AddButton(ID_PLAYLIST_REMOVE);
    AddButton(ID_PLAYLIST_SHUFFLE);

    // AddToolbarSeparator(m_Toolbar);

    AddButton(ID_WALLPAPER_SAFE);
    AddToolbarPlaceholder(m_Toolbar, IDC_PLAYLISTS);

    AddToolbarSeparator(m_Toolbar);

    AddButton(ID_APP_OPTIONS);
    AddButton(ID_APP_ABOUT);

    //
    // Create rebar and add toolbar too it.
    //
    // Rebar provides slightly better UI than raw toolbar,
    // and is also required by CreateToolbarComboBox().
    //

    CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);

    AddSimpleReBarBand(m_Toolbar);

    //
    // Add controls to the toolbar.
    //
    // Must be called AFTER toolbar is added to rebar.
    //

    m_Toolbar_CountdownTimer = CreateToolbarStaticText(m_Toolbar, IDC_COUNTDOWN_TIMER, 10);

    m_Toolbar_PlaylistCombo = CreateToolbarComboBox(m_Toolbar, IDC_PLAYLISTS, 16);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::UpdatePlaylistInToolbar
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdatePlaylistInToolbar()
{
    // Get all playlists.

    auto allPlaylists = CPlayList::GetAllPlaylists();

    // Populate the combobox in toolbar.

    m_Toolbar_PlaylistCombo.ResetContent();

    for (const auto& playlistName: allPlaylists)
    {
        int idx = m_Toolbar_PlaylistCombo.AddString(playlistName.c_str());

        if (playlistName == m_PlayList.GetPlaylistName())
            m_Toolbar_PlaylistCombo.SetCurSel(idx);
    }

    m_Toolbar_PlaylistCombo.AddString(L"Playlist Manager...");

    if (m_Toolbar_PlaylistCombo.GetCurSel() == -1)
        m_Toolbar_PlaylistCombo.SetCurSel(0);
}

//****************************************************************************
//
//  Toolbar notification handlers
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnToolbarPlaylistComboDropDown
//
//////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnToolbarPlaylistComboDropDown(UINT /*uNotifyCode*/, int /*nID*/, CWindow wndCtl)
{
    DebugPrintCmdSpew("OnToolbarPlaylistComboDropDown\n");

    if (wndCtl == m_Toolbar_PlaylistCombo)
    {
        UpdatePlaylistInToolbar();
    }
    else
    {
        SetMsgHandled(FALSE);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnToolbarPlaylistComboSelChange
//
//////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnToolbarPlaylistComboSelChange(UINT /*uNotifyCode*/, int /*nID*/, CWindow wndCtl)
{
    DebugPrintCmdSpew("OnToolbarPlaylistComboSelChange\n");

    if (wndCtl == m_Toolbar_PlaylistCombo)
    {
        int nIndex = m_Toolbar_PlaylistCombo.GetCurSel();

        if (nIndex >= 0)
        {
            // Check if the last item in combobox was selected.
            if (nIndex == m_Toolbar_PlaylistCombo.GetCount() - 1)
            {
                // Last item is always "Playlist Manager..."
                PostMessage(WM_COMMAND, ID_PLAYLIST_MANAGER, 0);
            }
            else
            {
                // Get playlist name from combobox.
                ATL::CTempBuffer<TCHAR, _WTL_STACK_ALLOC_THRESHOLD> strItemText;
                strItemText.Allocate(m_Toolbar_PlaylistCombo.GetLBTextLen(nIndex) + 1);
                m_Toolbar_PlaylistCombo.GetLBText(nIndex, strItemText);

                // Load new playlist.
                LoadPlaylist((LPWSTR)strItemText);
                PopulateListView(WallpaperManager.GetCurrentWallpaperFile());
            }

            // Set focus back to the main window
            // (take it away from the toolbar combobox).
            this->SetFocus();
        }
    }
    else
    {
        SetMsgHandled(FALSE);
    }

    return 0;
}

