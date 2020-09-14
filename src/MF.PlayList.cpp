//////////////////////////////////////////////////////////////////////////////
//
//  MF.PlayList.cpp
//
//  CMainFrame playlist functions.
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

#define SHUFFLE_PLAYLIST_ON_LOAD TRUE

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::LoadPlaylist
//
//////////////////////////////////////////////////////////////////////////////

bool CMainFrame::LoadPlaylist(ConstWString playlistName)
{
    fs::path playlistPath = CPlayList::NameToPath(playlistName);

    // Check if playlist file exists.
    // (Sanity check -- never expected to happen!)

    if (!fs::is_regular_file(playlistPath))
    {
        AtlMessageBox(m_hWnd,
                      (std::wstring(L"Playlist file is missing!\n\n") + playlistPath.native()).c_str(),
                      L"Error",
                      MB_ICONERROR | MB_OK);
        return false;
    }

    // Clear the list view because all existing
    // CFileInfo objects are about to be deleted.

    ClearListView();

    // Load new playlist.

    DebugPrint(L"Loading playlist: %s\n", playlistPath.c_str());

    if (!m_PlayList.Load(playlistPath))
    {
        m_PlayList.Save();

        AtlMessageBox(m_hWnd,
                      L"Playlist contains invalid or missing image files.\n"
                      L"These files have been removed from the list.",
                      L"Warning",
                      MB_ICONWARNING | MB_OK);
    }

#if SHUFFLE_PLAYLIST_ON_LOAD
    m_PlayList.Shuffle();
#else
    m_PlayList.Sort();
#endif

    // Update playlist MRU.

    GetAppOptions()->UsePlaylist(m_PlayList.GetPlaylistName());

    // Update window title.

    UpdateWindowTitle();

    // Update playlist combobox in toolbar.
    // Select the newly loaded playlist.

    UpdatePlaylistInToolbar();

    // Change wallpaper if current image isn't included in playlist.

    if (!m_PlayList.empty())
    {
        std::wstring currentWallpaperFile = WallpaperManager.GetCurrentWallpaperFile();

        auto it = std::find_if(m_PlayList.begin(), m_PlayList.end(),
                               [currentWallpaperFile] (CFileInfo* pFile)
                               { return pFile->m_FullPath == currentWallpaperFile; });

        if (it == m_PlayList.end())
        {
#if SHUFFLE_PLAYLIST_ON_LOAD
            // Display first image from the newly loaded playlist.
            // List was shuffled after being loaded, so the "first"
            // image is randomized.
            CFileInfo* pFile = m_PlayList.front();
            ChangeWallpaperImage(pFile->m_FullPath);
#else
            // Display random image from the newly loaded playlist.
            CFileInfo* pFile = m_PlayList.GetRandom();
            ChangeWallpaperImage(pFile->m_FullPath);
#endif
        }
        else
        {
            // "Change" to the image that is currently being displayed.
            // Done for the side effect of starting a new countdown
            // (exiting pause mode if needed) and updating the UI.
            ChangeWallpaperImage((*it)->m_FullPath);
        }
    }
    else
    {
        // Playlist is empty. Display solid color on desktop.
        ChangeWallpaperToSolidColor();
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::ChangeWallpaperImage
//
//////////////////////////////////////////////////////////////////////////////

void CMainFrame::ChangeWallpaperImage(fs::path wallpaperPath)
{
    if (!wallpaperPath.empty())
    {
        // Display wallpaper image on desktop.
        WallpaperManager.SetWallpaper(wallpaperPath);
    }
    else
    {
        // Display solid color on desktop.
        WallpaperManager.SetWallpaperToColor(WallpaperManager.GetBackgroundColor());
    }

    BeginCountdown();

    UpdateMainMenu();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CMainFrame::ShowNextOrPrev
//
//////////////////////////////////////////////////////////////////////////////

CFileInfo* CMainFrame::ShowNextOrPrev(int nID)
{
    // Nothing to do if playlist is empty.
    if (m_PlayList.size() == 0)
        return nullptr;

    // Find the current wallpaper in playlist.
    fs::path currentWallpaperFile = WallpaperManager.GetCurrentWallpaperFile();
    auto it = std::find_if(m_PlayList.begin(), m_PlayList.end(),
                           [currentWallpaperFile] (const CFileInfo* pFile)
                           { return pFile->m_FullPath == currentWallpaperFile; });

    if (nID == ID_WALLPAPER_NEXT)
    {
        if (it == m_PlayList.end())
        {
            // If current wallpaper not found, go to first image in playlist.
            // This happens when you open a different playlist that doesn't
            // contain the currently displayed wallpaper.
            it = m_PlayList.begin();
        }
        else
        {
            // Go to next wallpaper in sequence. Wrap from end to beginning.
            if (++it == m_PlayList.end())
                it = m_PlayList.begin();
        }
    }
    else
    {
        // Go to previous wallpaper in sequence. Wrap from beginning to end.
        if (it == m_PlayList.begin())
            it = m_PlayList.end();
        --it;
    }

    // Display the new wallpaper image.
    ChangeWallpaperImage((*it)->m_FullPath);

    return *it;
}
