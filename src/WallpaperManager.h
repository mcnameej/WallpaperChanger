//////////////////////////////////////////////////////////////////////////////
//
//  WallpaperManager.h
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

#pragma once

#include "Resize.h"

// Wallpaper manager.
class CWallpaperManager
{
    public:

        CWallpaperManager();

        ~CWallpaperManager();

        CWallpaperManager(const CWallpaperManager&) = delete;
        CWallpaperManager(CWallpaperManager&&) = delete;
        CWallpaperManager& operator=(const CWallpaperManager&) = delete;
        CWallpaperManager& operator=(CWallpaperManager&&) = delete;

        // Change the desktop wallpaper image.
        bool
        SetWallpaper(
            const fs::path& fullPath
        );

        // Change the desktop to a solid color.
        void
        SetWallpaperToColor(
            COLORREF color
        );

        // Is wallpaper image enabled?
        // (Solid background color is displayed when image is disabled.)
        bool
        IsWallpaperEnabled()
        {
            return m_IsWallpaperEnabled;
        }

        // Get the background color.
        COLORREF
        GetBackgroundColor()
        {
            return m_BackgroundColor;
        }

        // Set the background color.
        void
        SetBackgroundColor(
            COLORREF backgroundColor
        )
        {
            m_BackgroundColor = backgroundColor;
        }

        // Get the resize mode.
        WallpaperResizeMode
        GetResizeMode()
        {
            return m_ResizeMode;
        }

        // Set the resize mode.
        void
        SetResizeMode(
            WallpaperResizeMode resizeMode
        )
        {
            m_ResizeMode = resizeMode;
        }

        // Get the current wallpaper file (that we set).
        const fs::path&
        GetCurrentWallpaperFile()
        {
            return m_CurrentWallpaperFile;
        }

        // Get the current wallpaper file that Windows is using
        static
        fs::path
        GetCurrentWindowsWallpaperFile();

        // Save wallpaper information to registry.
        void
        SaveToRegistry();

    private:

        // Load wallpaper information from registry.
        void
        LoadFromRegistry();

    private:

        fs::path m_CurrentWallpaperFile;

        COLORREF m_BackgroundColor;

        WallpaperResizeMode m_ResizeMode;

        bool m_IsWallpaperEnabled;
};
