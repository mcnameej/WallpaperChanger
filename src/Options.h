//////////////////////////////////////////////////////////////////////////////
//
//  Options.h
//
//  CWallpaperChangerOptions class.
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

#include "HotKey.h"

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerOptions
//
//////////////////////////////////////////////////////////////////////////////

// Wallpaper Changer global application options.
// These are not the only options -- other classes
// (e.g. CWallpaperManager) handle their own options,
// including persisting them in the registry.
class CWallpaperChangerOptions
{
    public:

        // Maximum number of recently open playlists.
        static const int MaxRecentPlaylists = 5;

        // Recently open playlists.
        std::vector<std::wstring> m_RecentPlaylists;

        // How often should wallpaper be changed? (minutes)
        int m_ChangeInterval;

        // Have wallpaper changes been paused?
        bool m_Paused;

        // "Safe For Work" playlist.
        std::wstring m_SafePlaylist;

        // Hotkey for "Next Image".
        CHotKeyDefinition m_NextImageHotkey;

        // Hotkey for "Previous Image".
        CHotKeyDefinition m_PrevImageHotkey;

        // Hotkey for "Random Image".
        CHotKeyDefinition m_RandomImageHotkey;

        // Hotkey for "Safe Playlist".
        CHotKeyDefinition m_SafePlaylistHotkey;

    public:

        CWallpaperChangerOptions();

        void ResetToDefaults();

        void LoadFromRegistry();

        void SaveToRegistry();

        std::wstring
        GetRecentPlaylistName(
            int idx
        );

        void
        UsePlaylist(
            const std::wstring& playlistName
        );
};
