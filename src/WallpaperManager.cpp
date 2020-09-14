//////////////////////////////////////////////////////////////////////////////
//
//  WallpaperManager.cpp
//
//  CWallpaperManager class.
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

#include "WallpaperChangerApp.h"

#include "DesktopWallpaper.h"
#include "WallpaperManager.h"

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperManager ctor/dtor
//
//////////////////////////////////////////////////////////////////////////////

CWallpaperManager::CWallpaperManager()
    :
    m_CurrentWallpaperFile(),
    m_BackgroundColor(0x404040),
    m_ResizeMode(RESIZE_Fill),
    m_IsWallpaperEnabled(false)
{
    LoadFromRegistry();
}

CWallpaperManager::~CWallpaperManager()
{
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperManager::SetWallpaper
//
//////////////////////////////////////////////////////////////////////////////

// Change the desktop wallpaper image.
bool
CWallpaperManager::SetWallpaper(
    const fs::path& fullPath
)
{
    DebugPrint(L"Set wallpaper: %s\n", fullPath.c_str());

    if (!fullPath.empty() && fs::is_regular_file(fullPath))
    {
        CDesktopWallpaper* pDesktopWallpaper = new CDesktopWallpaper;

        // Get the background color that Windows is currently using.
        COLORREF currentBackgroundColor = 0;
        bool ok = SUCCEEDED(pDesktopWallpaper->GetBackgroundColor(&currentBackgroundColor));

        // Get the resize mode that Windows is currently using.
        DESKTOP_WALLPAPER_POSITION currentWallpaperPosition = DWPOS_CENTER;
        ok = ok && SUCCEEDED(pDesktopWallpaper->GetPosition(&currentWallpaperPosition));

        // Set the background color (if needed)
        if (ok && currentBackgroundColor != m_BackgroundColor)
            ok = ok && SUCCEEDED(pDesktopWallpaper->SetBackgroundColor(m_BackgroundColor));

        // Set the wallpaper image.
        ok = ok && SUCCEEDED(pDesktopWallpaper->SetWallpaper(fullPath));

        // Set the image resize mode (if needed).
        if (ok && currentWallpaperPosition != (DESKTOP_WALLPAPER_POSITION)GetResizeMode())
            ok = ok && SUCCEEDED(pDesktopWallpaper->SetPosition((DESKTOP_WALLPAPER_POSITION)GetResizeMode()));

        delete pDesktopWallpaper;

        if (ok)
        {
            m_IsWallpaperEnabled = true;
            m_CurrentWallpaperFile = fullPath;
            SaveToRegistry();
        }
        else
        {
            DebugPrint("Failed to set wallpaper!\n");
        }

        return ok;
    }
    else
    {
        m_CurrentWallpaperFile.clear();

        SetWallpaperToColor(m_BackgroundColor);

        return true;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperManager::SetWallpaperToColor
//
//////////////////////////////////////////////////////////////////////////////

// Change the desktop to a solid color.
void
CWallpaperManager::SetWallpaperToColor(
    COLORREF color
)
{
    DebugPrint(L"Set solid color: 0x%08X\n", color);

    m_BackgroundColor = color;

    CDesktopWallpaper* pDesktopWallpaper = new CDesktopWallpaper;
    pDesktopWallpaper->SetBackgroundColor(color);
    pDesktopWallpaper->Disable();
    delete pDesktopWallpaper;

    m_IsWallpaperEnabled = false;

    SaveToRegistry();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperManager::GetCurrentWindowsWallpaperFile
//
//  Called by CPlayList::CreateDefaultPlaylists(), which needs this
//  static function because it runs before the global CWallpaperManager
//  object is created.
//
//////////////////////////////////////////////////////////////////////////////

fs::path
CWallpaperManager::GetCurrentWindowsWallpaperFile()
{
    std::wstring wallpaperFile;

    CDesktopWallpaper* pDesktopWallpaper = new CDesktopWallpaper;
    pDesktopWallpaper->GetWallpaper(&wallpaperFile);
    delete pDesktopWallpaper;

    return wallpaperFile;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperManager::LoadFromRegistry
//
//////////////////////////////////////////////////////////////////////////////

void
CWallpaperManager::LoadFromRegistry()
{
    // Set defaults based on what Windows is doing.

    CDesktopWallpaper* pDesktopWallpaper = new CDesktopWallpaper;

    std::wstring wallpaperFile;
    if (SUCCEEDED(pDesktopWallpaper->GetWallpaper(&wallpaperFile)) &&
        !wallpaperFile.empty())
    {
        m_CurrentWallpaperFile = wallpaperFile;
    }

    COLORREF backgroundColor;
    if (SUCCEEDED(pDesktopWallpaper->GetBackgroundColor(&backgroundColor)) &&
        backgroundColor != 0)
    {
        m_BackgroundColor = backgroundColor;
    }

    DESKTOP_WALLPAPER_POSITION position;
    if (SUCCEEDED(pDesktopWallpaper->GetPosition(&position)) &&
        (WallpaperResizeMode) position >= RESIZE_Center &&
        (WallpaperResizeMode) position <= RESIZE_Fill)
    {
        m_ResizeMode = (WallpaperResizeMode) position;
    }

    m_IsWallpaperEnabled = !m_CurrentWallpaperFile.empty();

    delete pDesktopWallpaper;

    // Load our settings from the registry.

    CRegistryKey appRegKey = GetApp()->GetAppRegistryKey();

    appRegKey.Read(L"CurrentWallpaper", m_CurrentWallpaperFile);
    appRegKey.Read(L"BackgroundColor", m_BackgroundColor);
    appRegKey.ReadEnum(L"ResizeMode", m_ResizeMode);
    appRegKey.Read(L"WallpaperEnabled", m_IsWallpaperEnabled);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWallpaperManager::SaveToRegistry
//
//////////////////////////////////////////////////////////////////////////////

void
CWallpaperManager::SaveToRegistry()
{
    CRegistryKey appRegKey = GetApp()->GetAppRegistryKey();

    appRegKey.Write(L"CurrentWallpaper", m_CurrentWallpaperFile);
    appRegKey.Write(L"BackgroundColor", m_BackgroundColor);
    appRegKey.Write(L"ResizeMode", m_ResizeMode);
    appRegKey.Write(L"WallpaperEnabled", m_IsWallpaperEnabled);
}
