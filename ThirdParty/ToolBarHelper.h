#pragma once

//
//  https://www.codeproject.com/Articles/16737/WTL-Toolbar-Helper
//
//  Copyright 2006 Rob Caldecott
//
//  CPOL License... https://www.codeproject.com/info/cpol10.aspx
//

// Modified for the Wallpaper Changer project
// on 08/23/2020 by John McNamee <jpm@microwiz.com>.
//
// (1) Added blank lines between code blocks.
// (2) Fixed for 64-bit.
// (3) Made use of CString optional.
// (4) Don't depend on frame window using a Command Bar.
// (5) Let application window handle CBN_SELCHANGE.
// (6) Make combobox slightly narrower than button.

#define HANDLE_CBN_SELCHANGE_IN_TOOLBARHELPER FALSE
#define HANDLE_CBN_CLOSEUP_IN_TOOLBARHELPER   TRUE

// Define various toolbar button styles in case they are missing
#ifndef TBSTYLE_EX_MIXEDBUTTONS
#define TBSTYLE_EX_MIXEDBUTTONS         0x00000008
#endif

#ifndef BTNS_SHOWTEXT
#define BTNS_SHOWTEXT               0x0040
#endif

#define ATL_SIMPLE_TOOLBAR_PANE_STYLE_EX    (ATL_SIMPLE_TOOLBAR_PANE_STYLE|TBSTYLE_LIST)

/// Class used to expose useful toolbar functionality to a WTL CFrameWnd-derived class
template <class T>
class CToolBarHelper
{
private:

    /// Wrapper class for the Win32 TBBUTTONINFO structure.
    class CTBButtonInfo : public TBBUTTONINFO
    {
    public:
        /// Constructor
        CTBButtonInfo(DWORD dwInitialMask = 0)
        {
            memset(this, 0, sizeof(TBBUTTONINFO));
            cbSize = sizeof(TBBUTTONINFO);
            dwMask = dwInitialMask;
        }
    };

    /// Wrapper class for the Win32 TBBUTTON structure.
    class CTBButton : public TBBUTTON
    {
    public:
        /// Constructor
        CTBButton()
        {
            memset(this, 0, sizeof(TBBUTTON));
        }
    };

private:

    CFont m_fontCombo;          ///< Font to use for comboboxes

    CSimpleMap<UINT, UINT> m_mapMenu;   ///< Map of command IDs -> menu IDs

public:

    /// Message map
    BEGIN_MSG_MAP(CToolBarHelper<T>)
        NOTIFY_CODE_HANDLER(TBN_DROPDOWN, OnToolbarDropDown)
#if HANDLE_CBN_SELCHANGE_IN_TOOLBARHELPER
        COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnToolbarComboSelChange)
#endif
#if HANDLE_CBN_CLOSEUP_IN_TOOLBARHELPER
        COMMAND_CODE_HANDLER(CBN_CLOSEUP, OnToolbarComboClosed)
#endif
    END_MSG_MAP()

    //////////////////////////////////////////////////////////////////////////
    //
    //  Toolbar button text functions
    //
    //////////////////////////////////////////////////////////////////////////

    /// Add text to a toolbar button
    void AddToolbarButtonText(HWND hWndToolBar, UINT nID, LPCTSTR lpsz)
    {
        // Use built-in WTL toolbar wrapper class
        CToolBarCtrl toolbar(hWndToolBar);

        // Set extended style
        if ((toolbar.GetExtendedStyle() & TBSTYLE_EX_MIXEDBUTTONS) != TBSTYLE_EX_MIXEDBUTTONS)
            toolbar.SetExtendedStyle(toolbar.GetExtendedStyle() | TBSTYLE_EX_MIXEDBUTTONS);

        // Get the button index
        int nIndex = toolbar.CommandToIndex(nID);
        CTBButton tb;
        toolbar.GetButton(nIndex, &tb);
        int nStringID = toolbar.AddStrings(lpsz);

        // Alter the button style
        tb.iString = nStringID;
        tb.fsStyle |= BTNS_AUTOSIZE|BTNS_SHOWTEXT;

        // Delete and re-insert the button
        toolbar.DeleteButton(nIndex);
        toolbar.InsertButton(nIndex, &tb);
    }

    /// Add resource string to a toolbar button
    void AddToolbarButtonText(HWND hWndToolBar, UINT nID, UINT nStringID)
    {
#ifdef __ATLSTR_H__
        CString str;
        if (str.LoadString(nStringID))
            AddToolbarButtonText(hWndToolBar, nID, str);
#else
        LPTSTR pStringData;
        int length = ::LoadString(ModuleHelper::GetResourceInstance(), nID, (LPTSTR) &pStringData, 0);
        if (length)
        {
            // Need to copy into a buffer so we can guarantee NUL termination.
            // Raw string resource doesn't provide that guarantee.
    		ATL::CTempBuffer<TCHAR, _WTL_STACK_ALLOC_THRESHOLD> str;
            memcpy(str.Allocate(length + 1), pStringData, length * sizeof(TCHAR));
            str[length] = 0;
            AddToolbarButtonText(hWndToolBar, nID, str);
        }
#endif
    }

    /// Add text to a toolbar button (using tooltip text)
    void AddToolbarButtonText(HWND hWndToolBar, UINT nID)
    {
        TCHAR sz[256];
        if (AtlLoadString(nID, sz, 256) > 0)
        {
            // Add the text following the '\n'
            TCHAR* psz = _tcsrchr(sz, '\n');
            if (psz != NULL)
            {
                // Skip to first character of the tooltip
                psz++;

                // The tooltip text may include the accelerator, i.e.
                //  Open (Ctrl+O)
                // So look for an open brace
                TCHAR* pBrace = _tcschr(psz, '(');
                if (pBrace != NULL)
                    *(pBrace - 1) = '\0';
                AddToolbarButtonText(hWndToolBar, nID, psz);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //  Drop down menu functions
    //
    //////////////////////////////////////////////////////////////////////////

    /// Modify a toolbar button to have a drop-down button
    void AddToolBarDropDownMenu(HWND hWndToolBar, UINT nButtonID, UINT nMenuID)
    {
        ATLASSERT(hWndToolBar != NULL);
        ATLASSERT(nButtonID > 0);

        // Use built-in WTL toolbar wrapper class
        CToolBarCtrl toolbar(hWndToolBar);

        // Add the necessary style bit (TBSTYLE_EX_DRAWDDARROWS) if
        // not already present
        if ((toolbar.GetExtendedStyle() & TBSTYLE_EX_DRAWDDARROWS) != TBSTYLE_EX_DRAWDDARROWS)
            toolbar.SetExtendedStyle(toolbar.GetExtendedStyle() | TBSTYLE_EX_DRAWDDARROWS);

        // Get existing button style
        CTBButtonInfo tbi(TBIF_STYLE);
        if (toolbar.GetButtonInfo(nButtonID, &tbi) != -1)
        {
            // Modify the button
            tbi.fsStyle |= BTNS_DROPDOWN;
            toolbar.SetButtonInfo(nButtonID, &tbi);

            // We need to remember that this menu ID is associated with the button ID
            // so use a basic map for this.
            m_mapMenu.Add(nButtonID, nMenuID);
        }
    }

    // Handle TBN_DROPDOWN notification
    LRESULT OnToolbarDropDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
    {
        // Get the toolbar data
        NMTOOLBAR* ptb = reinterpret_cast<NMTOOLBAR*>(pnmh);

        // See if the button ID has an asscociated menu ID
        UINT nMenuID = m_mapMenu.Lookup(ptb->iItem);
        if (!nMenuID)
        {
            // This TBN_DROPDOWN isn't for us!
            bHandled = FALSE;
            return 0;
        }

        // Get the toolbar control
        CToolBarCtrl toolbar(pnmh->hwndFrom);

        // Get the button rect
        CRect rect;
        toolbar.GetItemRect(toolbar.CommandToIndex(ptb->iItem), &rect);

        // Create a point
        CPoint pt(rect.left, rect.bottom);

        // Map the points
        toolbar.MapWindowPoints(HWND_DESKTOP, &pt, 1);

        // Load the menu
        CMenu menu;
        if (menu.LoadMenu(nMenuID))
        {
            CMenuHandle menuPopup = menu.GetSubMenu(0);
            ATLASSERT(menuPopup != NULL);

            T* pT = static_cast<T*>(this);

            // Allow the menu items to be initialised (for example,
            // new items could be added here for example)
            pT->PrepareToolBarMenu(nMenuID, menuPopup);

            // Display the menu
#if 1
            // Display standard popup menu, without command bar enhancements
            menuPopup.TrackPopupMenuEx(TPM_RIGHTBUTTON|TPM_VERTICAL|TPM_NOANIMATION, pt.x, pt.y, pT->m_hWnd);
#else
            // Use command bar TrackPopupMenu method means menu icons are displayed
            pT->m_CmdBar.TrackPopupMenu(menuPopup, TPM_RIGHTBUTTON|TPM_VERTICAL, pt.x, pt.y);
#endif
        }

        return 0;
    }

    /// Override this to allow the menu items to be enabled/checked/etc.
    virtual void PrepareToolBarMenu(UINT /*nMenuID*/, HMENU /*hMenu*/)
    {
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //  Combobox functions
    //
    //////////////////////////////////////////////////////////////////////////

    /// Create a combobox on a toolbar
    /// NOTE: Toolbar must have already been added to rebar before calling this function.
    /// Otherwise the toolbar window size won't have been computed yet.
    HWND CreateToolbarComboBox(HWND hWndToolBar, UINT nID, UINT nWidth = 16, UINT nHeight = 16,
                   DWORD dwComboStyle = CBS_DROPDOWNLIST)
    {
        T* pT = static_cast<T*>(this);

        // Use built-in WTL toolbar wrapper class
        CToolBarCtrl toolbar(hWndToolBar);

        // Get the size of the combobox font
        CreateComboFont();
        CSize sizeFont = GetComboFontSize();

        // Compute the width and height
        UINT cx = (nWidth + 8) * sizeFont.cx;
        UINT cy = nHeight * sizeFont.cy;

        // Set the button width
        CTBButtonInfo tbi(TBIF_SIZE|TBIF_STATE|TBIF_STYLE);

        // Make sure the underlying button is disabled
        tbi.fsState = 0;

        // BTNS_SHOWTEXT will allow the button size to be altered
        tbi.fsStyle = BTNS_SHOWTEXT;
        tbi.cx = static_cast<WORD>(cx);

        // Update toolbar button
        toolbar.SetButtonInfo(nID, &tbi);

        // Get the index of the toolbar button
        int nIndex = toolbar.CommandToIndex(nID);

        // Get the button rect
        CRect rc;
        toolbar.GetItemRect(nIndex, &rc);
        rc.bottom = cy;

        // Make combobox slightly narrower than button so there's
        // space between the combo and whatever comes next.
        rc.right -= 4;

        // Create the combobox
        DWORD dwStyle = WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_TABSTOP|dwComboStyle;
        CComboBox combo;
        combo.Create(pT->m_hWnd, rc, NULL, dwStyle, 0, nID);
        combo.SetFont(m_fontCombo);
        combo.SetParent(toolbar);

        // The combobox might not be centred vertically, and we won't know the
        // height until it has been created.  Get the size now and see if it
        // needs to be moved.
        CRect rectToolBar;
        CRect rectCombo;
        toolbar.GetClientRect(&rectToolBar);
        combo.GetWindowRect(&rectCombo);

        // Get the different between the heights of the toolbar and
        // the combobox
        int nDiff = rectToolBar.Height() - rectCombo.Height();

        // If there is a difference, then move the combobox
        if (nDiff > 1)
        {
            toolbar.ScreenToClient(&rectCombo);
            combo.MoveWindow(rectCombo.left, rc.top + (nDiff / 2), rectCombo.Width(), rectCombo.Height());
        }

        return combo;
    }

    // Update toolbar height so it is tall enough to add a combobox.
    void UpdateToolbarHeightForCombobox(HWND hWndToolBar, DWORD dwComboStyle = CBS_DROPDOWNLIST)
    {
        T* pT = static_cast<T*>(this);

        // Create the combobox (hidden window).
        DWORD dwStyle = WS_CHILD|WS_VSCROLL|dwComboStyle;
        CComboBox combo;
        combo.Create(pT->m_hWnd, NULL, NULL, dwStyle, 0, 42);

        // Get size of combobox.
        CRect rectCombo;
        combo.GetWindowRect(rectCombo);

        // Destroy combobox window.
        combo.DestroyWindow();

        // Minimum toolbar height is combobox height plus small margin.
        long minimumToolbarHeight = rectCombo.Height() + 4;

        // Ensure that toolbar height is sufficient for combobox.
        SIZE toolbarButtonSize;
        CToolBarCtrl(hWndToolBar).GetButtonSize(toolbarButtonSize);
        if (toolbarButtonSize.cy < minimumToolbarHeight)
        {
            toolbarButtonSize.cy = minimumToolbarHeight;
            CToolBarCtrl(hWndToolBar).SetButtonSize(toolbarButtonSize);
        }
    }

#if HANDLE_CBN_SELCHANGE_IN_TOOLBARHELPER

    // Handle CBN_SELCHANGE from toolbar combobox.
    LRESULT OnToolbarComboSelChange(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
    {
        T* pT = static_cast<T*>(this);

        // Get the newly selected item index
        CComboBox combo(hWndCtl);
        int nSel = combo.GetCurSel();

        // Get the item text
#ifdef __ATLSTR_H__
        CString strItemText;
        combo.GetLBText(nSel, strItemText);
#else
 		ATL::CTempBuffer<TCHAR, _WTL_STACK_ALLOC_THRESHOLD> strItemText;
        strItemText.Allocate(combo.GetLBTextLen(nSel) + 1);
        combo.GetLBText(nSel, strItemText);
#endif

        // Get the item data
        DWORD_PTR dwItemData = combo.GetItemData(nSel);

        // Call special function to handle the selection change
        pT->OnToolbarComboSelection(combo, wID, nSel, strItemText, dwItemData);

        // Set focus to the main window
        pT->SetFocus();

        return TRUE;
    }

    // Override this to handle combobox selection changes
    virtual void OnToolbarComboSelection(HWND hWndCombo, UINT nID, int nSel, LPCTSTR lpszText, DWORD_PTR dwItemData)
    {
    }

#endif

#if HANDLE_CBN_CLOSEUP_IN_TOOLBARHELPER

    // Handle CBN_CLOSEUP from toolbar combobox.
    LRESULT OnToolbarComboClosed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
    {
        T* pT = static_cast<T*>(this);

        // Set focus back to the main window.
        // Don't want it to remain on combobox.
        if (GetFocus() == hWndCtl)
            pT->SetFocus();

        return TRUE;
    }

#endif

private:

    /// Create the font to use for comboboxes
    void CreateComboFont()
    {
        if (m_fontCombo == NULL)
        {
            NONCLIENTMETRICS ncm;
            ncm.cbSize = sizeof(NONCLIENTMETRICS);
            ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);

            // Create menu font
            m_fontCombo.CreateFontIndirect(&ncm.lfMenuFont);
            ATLASSERT(m_fontCombo != NULL);
        }
    }

    /// Get the size of the default GUI font
    CSize GetComboFontSize()
    {
        ATLASSERT(m_fontCombo != NULL);

        // We need a temporary DC
        const T* pT = static_cast<const T*>(this);
        CClientDC dc(pT->m_hWnd);

        // Select in the menu font
        CFontHandle fontOld = dc.SelectFont(m_fontCombo);

        // Get the font size
        TEXTMETRIC tm;
        dc.GetTextMetrics(&tm);

        // Done with the font
        dc.SelectFont(fontOld);

        // Return the width and height
        return CSize(tm.tmAveCharWidth, tm.tmHeight + tm.tmExternalLeading);
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //  Edit Control functions
    //
    //////////////////////////////////////////////////////////////////////////

public:

    /// Create an edit control on a toolbar
    /// NOTE: Toolbar must have already been added to rebar before calling this function.
    /// Otherwise the toolbar window size won't have been computed yet.
    HWND CreateToolbarEditCtrl(HWND hWndToolBar, UINT nID, UINT nWidth = 16, UINT nHeight = 1,
                   DWORD dwEditStyle = ES_READONLY | ES_CENTER)
    {
        T* pT = static_cast<T*>(this);

        // Use built-in WTL toolbar wrapper class
        CToolBarCtrl toolbar(hWndToolBar);

        // Get the size of the combobox font.
        // We'll use same font for edit control.
        CreateComboFont();
        CSize sizeFont = GetComboFontSize();

        // Compute the width and height
        UINT cx = (nWidth * sizeFont.cx) + 8;
        UINT cy = (nHeight * sizeFont.cy) + 8;

        // Set the button width
        CTBButtonInfo tbi(TBIF_SIZE|TBIF_STATE|TBIF_STYLE);

        // Make sure the underlying button is disabled
        tbi.fsState = 0;

        // BTNS_SHOWTEXT will allow the button size to be altered
        tbi.fsStyle = BTNS_SHOWTEXT;
        tbi.cx = static_cast<WORD>(cx);

        // Update toolbar button
        toolbar.SetButtonInfo(nID, &tbi);

        // Get the index of the toolbar button
        int nIndex = toolbar.CommandToIndex(nID);

        // Get the button rect
        CRect rc;
        toolbar.GetItemRect(nIndex, &rc);
        rc.bottom = cy;

        // Make edit control slightly narrower than button so there's
        // space between the edit and whatever comes next.
        rc.right -= 4;

        // Create the edit control
        DWORD dwStyle = WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|dwEditStyle;
        CEdit editCtrl;
        editCtrl.Create(pT->m_hWnd, rc, NULL, dwStyle, 0, nID);
        editCtrl.SetFont(m_fontCombo);
        editCtrl.SetParent(toolbar);

        // The control might not be centred vertically, and we won't know the
        // height until it has been created.  Get the size now and see if it
        // needs to be moved.
        CRect rectToolBar;
        CRect rectControl;
        toolbar.GetClientRect(&rectToolBar);
        editCtrl.GetWindowRect(&rectControl);

        // Get the different between the heights of the toolbar and
        // the control
        int nDiff = rectToolBar.Height() - rectControl.Height();

        // If there is a difference, then move the combobox
        if (nDiff > 1)
        {
            toolbar.ScreenToClient(&rectControl);
            editCtrl.MoveWindow(rectControl.left, rc.top + (nDiff / 2), rectControl.Width(), rectControl.Height());
        }

        return editCtrl;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //  Static Text functions
    //
    //////////////////////////////////////////////////////////////////////////

public:

    /// Create a static text control on a toolbar
    /// NOTE: Toolbar must have already been added to rebar before calling this function.
    /// Otherwise the toolbar window size won't have been computed yet.
    HWND CreateToolbarStaticText(HWND hWndToolBar, UINT nID, UINT nWidth = 16, UINT nHeight = 1,
                   DWORD dwStaticStyle = SS_CENTER)
    {
        T* pT = static_cast<T*>(this);

        // Use built-in WTL toolbar wrapper class
        CToolBarCtrl toolbar(hWndToolBar);

        // Get the size of the combobox font.
        // We'll use same font for static text.
        CreateComboFont();
        CSize sizeFont = GetComboFontSize();

        // Compute the width and height
        UINT cx = (nWidth * sizeFont.cx) + 8;
        UINT cy = (nHeight * sizeFont.cy) + 8;

        // Set the button width
        CTBButtonInfo tbi(TBIF_SIZE|TBIF_STATE|TBIF_STYLE);

        // Make sure the underlying button is disabled
        tbi.fsState = 0;

        // BTNS_SHOWTEXT will allow the button size to be altered
        tbi.fsStyle = BTNS_SHOWTEXT;
        tbi.cx = static_cast<WORD>(cx);

        // Update toolbar button
        toolbar.SetButtonInfo(nID, &tbi);

        // Get the index of the toolbar button
        int nIndex = toolbar.CommandToIndex(nID);

        // Get the button rect
        CRect rc;
        toolbar.GetItemRect(nIndex, &rc);
        rc.bottom = cy;

        // Make static control slightly narrower than button so there's
        // space between the edit and whatever comes next.
        rc.right -= 4;

        // Create the static control
        DWORD dwStyle = WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|dwStaticStyle;
        CStatic staticCtrl;
        staticCtrl.Create(pT->m_hWnd, rc, NULL, dwStyle, 0, nID);
        staticCtrl.SetFont(m_fontCombo);
        staticCtrl.SetParent(toolbar);

        // The control might not be centred vertically, and we won't know the
        // height until it has been created.  Get the size now and see if it
        // needs to be moved.
        CRect rectToolBar;
        CRect rectControl;
        toolbar.GetClientRect(&rectToolBar);
        staticCtrl.GetWindowRect(&rectControl);

        // Get the different between the heights of the toolbar and
        // the control
        int nDiff = rectToolBar.Height() - rectControl.Height();

        // If there is a difference, then move the combobox
        if (nDiff > 1)
        {
            toolbar.ScreenToClient(&rectControl);
            staticCtrl.MoveWindow(rectControl.left, rc.top + (nDiff / 2), rectControl.Width(), rectControl.Height());
        }

        return staticCtrl;
    }
};
