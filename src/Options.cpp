//////////////////////////////////////////////////////////////////////////////
//
//  Options.cpp
//
//  CWallpaperChangerOptions class implementation.
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
#include "PlayList.h"

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerOptions ctor
//
//////////////////////////////////////////////////////////////////////////////

CWallpaperChangerOptions::CWallpaperChangerOptions()
{
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerOptions::ResetToDefaults
//
//////////////////////////////////////////////////////////////////////////////

void CWallpaperChangerOptions::ResetToDefaults()
{
    m_RecentPlaylists.clear();
    m_ChangeInterval = 5;
    m_Paused = false;
    m_SafePlaylist.clear();
    m_NextImageHotkey    = { 'N', MOD_CONTROL | MOD_ALT };
    m_PrevImageHotkey    = { 'P', MOD_CONTROL | MOD_ALT };
    m_RandomImageHotkey  = { 'R', MOD_CONTROL | MOD_ALT };
    m_SafePlaylistHotkey = { 'B', MOD_CONTROL | MOD_ALT };
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerOptions::LoadFromRegistry
//
//////////////////////////////////////////////////////////////////////////////

void CWallpaperChangerOptions::LoadFromRegistry()
{
    ResetToDefaults();

    CRegistryKey appKey = GetApp()->GetAppRegistryKey();

    CBoolResult result;
    std::wstring strHotKey;

    result |= appKey.Read(L"RecentPlaylists", m_RecentPlaylists);

    result |= appKey.Read(L"ChangeInterval", m_ChangeInterval);
    result |= appKey.Read(L"Paused", m_Paused);
    result |= appKey.Read(L"SafePlaylist", m_SafePlaylist);

    result |= (appKey.Read(L"Hotkey.Next", strHotKey)   && m_NextImageHotkey.Parse(strHotKey));
    result |= (appKey.Read(L"Hotkey.Prev", strHotKey)   && m_PrevImageHotkey.Parse(strHotKey));
    result |= (appKey.Read(L"Hotkey.Random", strHotKey) && m_RandomImageHotkey.Parse(strHotKey));
    result |= (appKey.Read(L"Hotkey.Safe", strHotKey)   && m_SafePlaylistHotkey.Parse(strHotKey));

    // Validate that playlists in MRU list exist.
    for (auto it = m_RecentPlaylists.begin(); it != m_RecentPlaylists.end(); )
    {
        fs::path playlistPath = CPlayList::NameToPath(it->c_str());
        if (fs::is_regular_file(playlistPath))
        {
            ++it;
        }
        else
        {
            DebugPrint(L"WallpaperChanger: Invalid MRU playlist: %s\n", it->c_str());
            it = m_RecentPlaylists.erase(it);
            result |= false;
        }
    }

    // Validate that "Safe For Work" playlist exists.
    if (!m_SafePlaylist.empty())
    {
        if (!fs::is_regular_file(CPlayList::NameToPath(m_SafePlaylist)))
        {
            DebugPrint(L"WallpaperChanger: Invalid safe playlist: %s\n", m_SafePlaylist.c_str());
            m_SafePlaylist.clear();
            result |= false;
        }
    }

    // If MRU list is empty, add first playlist found (in alphabetical order).
    // If no playlists were found, create the default playlists.
    // Having an empty MRU list creates a bad UX because CMainFrame::OnCreate()
    // tries to load the top MRU playlist. It is very confusing if that fails.
    if (m_RecentPlaylists.empty())
    {
        auto allPlaylists = CPlayList::GetAllPlaylists();
        if (!allPlaylists.empty())
        {
            DebugPrint(L"WallpaperChanger: Playlist MRU is empty\n");
            DebugPrint(L"WallpaperChanger: Adding \"%s\"\n", allPlaylists[0].c_str());
            m_RecentPlaylists.push_back(allPlaylists[0]);
        }
        else
        {
            DebugPrint(L"WallpaperChanger: Creating default playlists\n");
            CPlayList::CreateDefaultPlaylists();
        }
    }

    // Enforce maximum size of MRU list.
    if (m_RecentPlaylists.size() > MaxRecentPlaylists)
    {
        m_RecentPlaylists.resize(MaxRecentPlaylists);
        result |= false;
    }

    if (!result)
        SaveToRegistry();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerOptions::SaveToRegistry
//
//////////////////////////////////////////////////////////////////////////////

void CWallpaperChangerOptions::SaveToRegistry()
{
    CRegistryKey appKey = GetApp()->GetAppRegistryKey();
    ATLASSERT(appKey.IsOpen());

    appKey.Write(L"RecentPlaylists", m_RecentPlaylists);

    appKey.Write(L"ChangeInterval", m_ChangeInterval);
    appKey.Write(L"Paused", m_Paused);
    appKey.Write(L"SafePlaylist", m_SafePlaylist);

    appKey.Write(L"Hotkey.Next", m_NextImageHotkey.ToString());
    appKey.Write(L"Hotkey.Prev", m_PrevImageHotkey.ToString());
    appKey.Write(L"Hotkey.Random", m_RandomImageHotkey.ToString());
    appKey.Write(L"Hotkey.Safe", m_SafePlaylistHotkey.ToString());
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerOptions::GetRecentPlaylistName
//
//////////////////////////////////////////////////////////////////////////////

std::wstring
CWallpaperChangerOptions::GetRecentPlaylistName(
    int idx
)
{
    if (idx < m_RecentPlaylists.size())
        return m_RecentPlaylists[idx];
    else
        return std::wstring();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperChangerOptions::UsePlaylist
//
//////////////////////////////////////////////////////////////////////////////

void
CWallpaperChangerOptions::UsePlaylist(
    const std::wstring& playlistName
)
{
    // Check if file is already in MRU list.
    auto it = std::find(m_RecentPlaylists.begin(),
                        m_RecentPlaylists.end(),
                        playlistName);

    // Erase file from MRU list, if it already exists.
    if (it != m_RecentPlaylists.end())
        m_RecentPlaylists.erase(it);

    // If MRU list is full, drop the oldest entry.
    if (m_RecentPlaylists.size() == MaxRecentPlaylists)
        m_RecentPlaylists.pop_back();

    // Add file to start of the MRU list.
    m_RecentPlaylists.insert(m_RecentPlaylists.begin(), playlistName);

    // Save MRU list to the registry.
    SaveToRegistry();
}
