//////////////////////////////////////////////////////////////////////////////
//
//  MF.ListView.cpp
//
//  CMainFrame ListView functions and notification handlers.
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
//  ListView functions
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::CreateListView
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::CreateListView()
{
    const DWORD dwStyle =
        WS_CHILD |
        WS_VISIBLE |
        WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN |
        LVS_ICON |
        LVS_SHOWSELALWAYS |
        LVS_SHAREIMAGELISTS |
        0;

    const DWORD dwExtStyle =
        WS_EX_CLIENTEDGE |
        0;

    const DWORD dwLvsExtStyle =
        LVS_EX_AUTOAUTOARRANGE |
        // LVS_EX_BORDERSELECT |
        LVS_EX_DOUBLEBUFFER |
        LVS_EX_INFOTIP |
        0;

    m_hWndClient = m_ListView.Create(m_hWnd, rcDefault, NULL, dwStyle, dwExtStyle, ID_PLAYLIST_VIEW);

    m_ListView.SetViewType(LVS_ICON);

    m_ListView.SetExtendedListViewStyle(dwLvsExtStyle, dwLvsExtStyle);

    m_ListView.SetOutlineColor(RGB(0,0,0));

    m_ListView.InsertColumn(0, TEXT("Name"), LVCFMT_LEFT, 300);

    // Our thumbnail height is always 128 pixels.
    // The width varies based on the main screen
    // aspect ratio. Explorer varies both width
    // and height, which sometimes looks fugly.

    int cxDesktop = GetSystemMetrics(SM_CXSCREEN);
    int cyDesktop = GetSystemMetrics(SM_CYSCREEN);
    double displayRatio = (double) cxDesktop / (double) cyDesktop;
    m_ThumbnailSize = { (int) std::round(128.0 * displayRatio), 128 };

    m_ListView.SetIconSpacing(m_ThumbnailSize.cx + 26,
                              m_ThumbnailSize.cy + 48);

    m_ImageList.Create(m_ThumbnailSize.cx,
                       m_ThumbnailSize.cy,
                       ILC_COLOR32,
                       ImageListCacheSize,
                       1);

    m_ListView.SetImageList(m_ImageList, LVSIL_NORMAL);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::ClearListView
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::ClearListView()
{
    m_ListView.DeleteAllItems();

    m_ImageListCache.Clear();

    UISetText(ID_DEFAULT_PANE, NULL);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::PopulateListView
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::PopulateListView(const fs::path& selectedFile /*= fs::path()*/)
{
    BeginListViewUpdate();

    ClearListView();

    int iSelectedItem = -1;

    for (CFileInfo* pFile: m_PlayList)
    {
        // Add file to listview.
        int iItem = AddFileToListView(pFile);

        // Remember the new listview item number for the selected file.
        if (iSelectedItem == -1 && !selectedFile.empty() && pFile->m_FullPath == selectedFile)
            iSelectedItem = iItem;
    }

    if (iSelectedItem == -1)
        iSelectedItem  = 0;

    EndListViewUpdate(iSelectedItem);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::AddFileToListView
//
//////////////////////////////////////////////////////////////////////////////

int CMainFrame::AddFileToListView(CFileInfo* pFile, int nItem /*= INT_MAX*/)
{
    return m_ListView.InsertItem(LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,
                                 nItem,
                                 LPSTR_TEXTCALLBACK,
                                 0, 0,
                                 I_IMAGECALLBACK,
                                 (LPARAM) pFile);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::BeginListViewUpdate
//  CMainFrame::EndListViewUpdate
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::BeginListViewUpdate()
{
    m_ListView.SetRedraw(FALSE);
}

void CMainFrame::EndListViewUpdate(int iSelectedItem)
{
    if (iSelectedItem != -1)
    {
        m_ListView.SelectItem(iSelectedItem);
        m_ListView.EnsureVisible(iSelectedItem, FALSE);
    }

    m_ListView.SetRedraw(TRUE);

    m_ListView.RedrawWindow(NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::RefreshListView
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::RefreshListView()
{
    m_ImageListCache.Clear();

    m_ListView.RedrawItems(0, m_ListView.GetItemCount()-1);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::GetCurrentSelectedItem
//
//////////////////////////////////////////////////////////////////////////////

int CMainFrame::GetCurrentSelectedItem()
{
    return m_ListView.GetNextItem(-1, m_ListView.GetSelectedCount() == 1 ? LVNI_SELECTED : LVNI_FOCUSED);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::GetCurrentSelectedFile
//
//////////////////////////////////////////////////////////////////////////////

fs::path CMainFrame::GetCurrentSelectedFile()
{
    int iItem = GetCurrentSelectedItem();

    if (iItem == -1)
        return fs::path();
    else
        return GetListViewItemData(iItem)->m_FullPath;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::GetCurrentWallpaperItem
//
//////////////////////////////////////////////////////////////////////////////

int CMainFrame::GetCurrentWallpaperItem()
{
    fs::path currentWallpaperFile = WallpaperManager.GetCurrentWallpaperFile();

    if (currentWallpaperFile.empty())
        return -1;

    // Find current wallpaper in the playlist,
    // then find the corresponding ListView item.

    auto it = std::find_if(m_PlayList.begin(), m_PlayList.end(),
                           [currentWallpaperFile] (CFileInfo* pFile)
                           { return pFile->m_FullPath == currentWallpaperFile; });

    if (it != m_PlayList.end())
        return GetWallpaperItem(*it);
    else
        return -1;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::GetWallpaperItem
//
//////////////////////////////////////////////////////////////////////////////

int CMainFrame::GetWallpaperItem(CFileInfo* pFile)
{
    if (pFile)
    {
        LVFINDINFOW findInfo = { LVFI_PARAM, NULL, (LPARAM) pFile, {0,0}, 0 };
        return m_ListView.FindItem(&findInfo, -1);
    }
    return -1;
}

//****************************************************************************
//
//  ListView notification handlers
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnGetEmptyMarkup
//
//////////////////////////////////////////////////////////////////////////////

// Handle LVN_GETEMPTYMARKUP notifications.
LRESULT CMainFrame::OnGetEmptyMarkup(NMHDR* phdr)
{
    NMLVEMPTYMARKUP* pNotify = (NMLVEMPTYMARKUP*) phdr;

    pNotify->dwFlags = EMF_CENTERED;

    StringCbCopyW(pNotify->szMarkup,
                  sizeof(pNotify->szMarkup),
                  L"Playlist is empty.");

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnGetDispInfo
//
//////////////////////////////////////////////////////////////////////////////

// Handle LVN_GETDISPINFO notifications.
LRESULT CMainFrame::OnGetDispInfo(NMHDR* phdr)
{
    NMLVDISPINFO* pNotify = (NMLVDISPINFO*) phdr;
    CFileInfo* pFile = (CFileInfo*) pNotify->item.lParam;

    if (pNotify->item.mask & LVIF_TEXT)
    {
        static WCHAR wszItemName[MAX_PATH];

        ::StringCbCopyW(wszItemName,
                        sizeof(wszItemName),
                        pFile->GetDisplayName());

        pNotify->item.pszText = wszItemName;

        // DebugPrintCmdSpew(L"OnGetDispInfo: TEXT [%d] = %s\n", pNotify->item.iItem, pNotify->item.pszText);
    }

    if (pNotify->item.mask & LVIF_IMAGE)
    {
        int* pImageListIndex = nullptr;

        if (m_ImageListCache.Lookup(pFile, &pImageListIndex))
        {
            // Image is already in the cache.

            pNotify->item.iImage = *pImageListIndex;

            // DebugPrintCmdSpew("OnGetDispInfo: CACHED [%d] iImage = %d\n", pNotify->item.iItem, pNotify->item.iImage);
        }
        else
        {
            // Image isn't in the cache. Add it.

            CBitmap thumbnailBitmap = pFile->GetLargeThumbnail();

            if (thumbnailBitmap.IsNull())
            {
                ATLASSERT(false);
                return 0;
            }

            ResizeWallpaperBitmap(&thumbnailBitmap.m_hBitmap,
                                  m_ThumbnailSize,
                                  WallpaperManager.GetResizeMode(),
                                  GetSysColor(COLOR_WINDOW));

            if (*pImageListIndex == -1)
            {
                *pImageListIndex = m_ImageList.Add(thumbnailBitmap);
                pNotify->item.iImage = *pImageListIndex;
                // DebugPrintCmdSpew("OnGetDispInfo: ADD [%d] iImage = %d\n", pNotify->item.iItem, pNotify->item.iImage);
            }
            else
            {
                m_ImageList.Replace(*pImageListIndex, thumbnailBitmap, NULL);
                pNotify->item.iImage = *pImageListIndex;
                // DebugPrintCmdSpew("OnGetDispInfo: REPLACE [%d] iImage = %d\n", pNotify->item.iItem, pNotify->item.iImage);
            }
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnGetInfoTip
//
//////////////////////////////////////////////////////////////////////////////

// Handle LVN_GETINFOTIP notifications.
LRESULT CMainFrame::OnGetInfoTip(NMHDR* phdr)
{
    // IMPORTANT: lParam is not set -- use GetListViewItemData() with iItem.
    NMLVGETINFOTIP* pNotify = (NMLVGETINFOTIP*) phdr;
    CFileInfo* pFile = GetListViewItemData(pNotify->iItem);

    // DebugPrintCmdSpew("OnGetInfoTip: iItem = %d\n", pNotify->iItem);

    // Show full path in the tooltip.
    ::StringCchCopyW(pNotify->pszText,
                     pNotify->cchTextMax,
                     pFile->m_FullPath.c_str());

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnItemChanged
//
//////////////////////////////////////////////////////////////////////////////

// Handle LVN_ITEMCHANGED notifications.
LRESULT CMainFrame::OnItemChanged(NMHDR* phdr)
{
    NMLISTVIEW* pNotify = (NMLISTVIEW*) phdr;

    // Show information about selection in status bar.

    int selectedCount = m_ListView.GetSelectedCount();

    if (selectedCount == 1)
    {
        CFileInfo* pFile = GetListViewItemData(m_ListView.GetNextItem(-1, LVNI_SELECTED));
        UISetText(ID_DEFAULT_PANE, (std::wstring(L"Selected: ") + std::wstring(pFile->m_FullPath)).c_str());
    }
    else if (selectedCount > 1)
    {
        UISetText(ID_DEFAULT_PANE, Format(L"%d images selected\n", selectedCount).c_str());
    }
    else
    {
        UISetText(ID_DEFAULT_PANE, L"No images selected\n");
    }

    UIEnable(ID_PLAYLIST_REMOVE, (BOOL)(m_ListView.GetSelectedCount() > 0));

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnRightClick
//
//////////////////////////////////////////////////////////////////////////////

// Handle NM_RCLICK notifications from ListView.
LRESULT CMainFrame::OnRightClick(NMHDR* phdr)
{
    // IMPORTANT: Only iItem, iSubItem, and ptAction are valid. lParam is not set.
    NMITEMACTIVATE* pNotify = (NMITEMACTIVATE*) phdr;

    DebugPrintCmdSpew("ListView: NM_RCLICK iItem = %d\n", pNotify->iItem);

    // iItem will be -1 if the right click occurs in a "white space" area
    // not occupied by any listview item.  Any other value indicates which
    // item was right clicked.  The clicked item will be selected, but other
    // items may be too.  Need to enumerate the selection to get them all.

    CMenu contextMenu;
    CMenuHandle menuItems;

    if (pNotify->iItem == -1)
    {
        contextMenu.LoadMenu(IDM_VIEW);
        menuItems = contextMenu.GetSubMenu(0);
    }
    else
    {
        contextMenu.LoadMenu(IDM_ITEM);
        menuItems = contextMenu.GetSubMenu(0);
        menuItems.SetMenuDefaultItem(ID_WALLPAPER_CHANGE);
    }

    ClientToScreen(&pNotify->ptAction);

    menuItems.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_NOANIMATION,
                               pNotify->ptAction.x,
                               pNotify->ptAction.y,
                               m_hWnd);

    ::PostMessageW(m_hWnd, WM_NULL, 0, 0);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnDoubleClick
//
//////////////////////////////////////////////////////////////////////////////

// Handle NM_DBLCLK notifications from ListView.
LRESULT CMainFrame::OnDoubleClick(NMHDR* phdr)
{
    // IMPORTANT: Only iItem, iSubItem, and ptAction are valid. lParam is not set.
    NMITEMACTIVATE* pNotify = (NMITEMACTIVATE*) phdr;

    DebugPrintCmdSpew("ListView: NM_DBLCLK iItem = %d\n", pNotify->iItem);

    // iItem will be -1 if the double click occurs in a "white space" area
    // not occupied by any listview item.  Any other iItem value indicates
    // which item was double clicked.  However, there may be other selected
    // items -- need to enumerate the selection to get all of them.

    // Only handle double click if a single item is selected.
    if (m_ListView.GetSelectedCount() == 1)
    {
        ATLASSERT(pNotify->iItem == m_ListView.GetNextItem(-1, LVNI_SELECTED));
        ::PostMessageW(m_hWnd, WM_COMMAND, ID_WALLPAPER_CHANGE, 0);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::OnReturn
//
//////////////////////////////////////////////////////////////////////////////

// Handle NM_RETURN notifications.
LRESULT CMainFrame::OnReturn(NMHDR* phdr)
{
    DebugPrintCmdSpew("ListView: NM_RETURN\n");

    // Only handle RETURN if a single item is selected.
    if (m_ListView.GetSelectedCount() == 1)
        ::PostMessageW(m_hWnd, WM_COMMAND, ID_WALLPAPER_CHANGE, 0);

    return 0;
}
