//////////////////////////////////////////////////////////////////////////////
//
//  DesktopWallpaper.h
//
//  CDesktopWallpaper class.
//
//  Expose the IDesktopWallpaper shell interface, with some enhancements.
//  Intended to be used by the CWallpaperManager class, and not directly
//  by the application.
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

// Expose the IDesktopWallpaper shell interface, with some enhancements.
class CDesktopWallpaper
{
    public:

        // Create DesktopWallpaper COM object.
        CDesktopWallpaper()
        {
            HRESULT hr = ::CoCreateInstance(CLSID_DesktopWallpaper,
                                            nullptr,
                                            CLSCTX_LOCAL_SERVER,
                                            IID_PPV_ARGS(&m_pDesktopWallpaper));
            if (FAILED(hr))
                ReoprtFailureAndExit(hr, TEXT("CoCreateInstance(CLSID_DesktopWallpaper)"));
        }

        //
        //  Background color
        //

        HRESULT
        GetBackgroundColor(
            COLORREF* pColor
        )
        {
            return m_pDesktopWallpaper->GetBackgroundColor(pColor);
        }

        HRESULT
        SetBackgroundColor(
            COLORREF color
        )
        {
            return m_pDesktopWallpaper->SetBackgroundColor(color);
        }

        //
        //  Wallpaper file
        //

        HRESULT
        GetWallpaper(
            LPCWSTR pwszMonitorID,
            LPWSTR* ppwszWallpaperFile  // Use CComHeapPtr<> or free with CoTaskMemFree()
        )
        {
            return m_pDesktopWallpaper->GetWallpaper(pwszMonitorID, ppwszWallpaperFile);
        }

        HRESULT
        GetWallpaper(
            LPCWSTR pwszMonitorID,
            std::wstring* psWallpaperFile
        )
        {
            psWallpaperFile->clear();

            LPWSTR pwszWallpaperFile = nullptr;

            HRESULT hr = m_pDesktopWallpaper->GetWallpaper(pwszMonitorID, &pwszWallpaperFile);

            if (SUCCEEDED(hr) && pwszWallpaperFile != nullptr)
            {
                psWallpaperFile->assign(pwszWallpaperFile);
                CoTaskMemFree(pwszWallpaperFile);
            }

            return hr;
        }

        HRESULT
        GetWallpaper(
            LPWSTR* ppwszWallpaperFile  // Use CComHeapPtr<> or free with CoTaskMemFree()
        )
        {
            // Returns S_FALSE if all monitors don't have the same wallpaper.
            return m_pDesktopWallpaper->GetWallpaper(nullptr, ppwszWallpaperFile);
        }

        HRESULT
        GetWallpaper(
            std::wstring* psWallpaperFile
        )
        {
            // Returns S_FALSE if all monitors don't have the same wallpaper.
            return GetWallpaper(nullptr, psWallpaperFile);
        }

        HRESULT
        SetWallpaper(
            LPCWSTR pwszMonitorID,
            ConstWString pwszWallpaperFile
        )
        {
            return m_pDesktopWallpaper->SetWallpaper(pwszMonitorID, pwszWallpaperFile);
        }

        HRESULT
        SetWallpaper(
            ConstWString pwszWallpaperFile
        )
        {
            return m_pDesktopWallpaper->SetWallpaper(nullptr, pwszWallpaperFile);
        }

        //
        //  Wallpaper position
        //

        HRESULT
        GetPosition(
            DESKTOP_WALLPAPER_POSITION* pPosition
        )
        {
            return m_pDesktopWallpaper->GetPosition(pPosition);
        }

        HRESULT
        SetPosition(
            DESKTOP_WALLPAPER_POSITION position
        )
        {
            return m_pDesktopWallpaper->SetPosition(position);
        }

        //
        //  Slideshow
        //

        HRESULT
        GetSlideshowFiles(
            IShellItemArray** ppItemArray
        )
        {
            return m_pDesktopWallpaper->GetSlideshow(ppItemArray);
        }

        HRESULT
        SetSlideshowFiles(
            IShellItemArray* pItemArray
        )
        {
            return m_pDesktopWallpaper->SetSlideshow(pItemArray);
        }

        HRESULT
        GetSlideshowOptions(
            DESKTOP_SLIDESHOW_OPTIONS* pOptions,
            UINT* pSlideshowTick
        )
        {
            return m_pDesktopWallpaper->GetSlideshowOptions(pOptions, pSlideshowTick);
        }

        HRESULT
        SetSlideshowOptions(
            DESKTOP_SLIDESHOW_OPTIONS options,
            UINT slideshowTick
        )
        {
            return m_pDesktopWallpaper->SetSlideshowOptions(options, slideshowTick);
        }

        HRESULT
        AdvanceSlideshow(
            LPCWSTR pwszMonitorID = nullptr,
            DESKTOP_SLIDESHOW_DIRECTION direction = DSD_FORWARD
        )
        {
            return m_pDesktopWallpaper->AdvanceSlideshow(pwszMonitorID, direction);
        }

        HRESULT
        Next(
            LPCWSTR monitorID = nullptr
        )
        {
            return AdvanceSlideshow(monitorID, DSD_FORWARD);
        }

        HRESULT
        Previous(
            LPCWSTR monitorID = nullptr
        )
        {
            return AdvanceSlideshow(monitorID, DSD_BACKWARD);
        }

        //
        //  Multi-monitor support
        //

        HRESULT
        GetMonitorDevicePathCount(
            UINT* pCount
        )
        {
            return m_pDesktopWallpaper->GetMonitorDevicePathCount(pCount);
        }

        HRESULT
        GetMonitorDevicePathAt(
            UINT monitorIndex,
            LPWSTR* ppwszMonitorID  // Use CComHeapPtr<> or free with CoTaskMemFree()
        )
        {
            return m_pDesktopWallpaper->GetMonitorDevicePathAt(monitorIndex, ppwszMonitorID);
        }

        HRESULT
        GetMonitorDevicePathAt(
            UINT monitorIndex,
            std::wstring* psMonitorID
        )
        {
            psMonitorID->clear();

            LPWSTR pwszMonitorID = nullptr;

            HRESULT hr = m_pDesktopWallpaper->GetMonitorDevicePathAt(monitorIndex, &pwszMonitorID);

            if (SUCCEEDED(hr) && pwszMonitorID != nullptr)
            {
                psMonitorID->assign(pwszMonitorID);
                CoTaskMemFree(pwszMonitorID);
            }

            return hr;
        }

        HRESULT
        GetMonitorRECT(
          LPCWSTR pwszMmonitorID,
          RECT* pDisplayRect
        )
        {
            return m_pDesktopWallpaper->GetMonitorRECT(pwszMmonitorID, pDisplayRect);
        }

        //
        //  Misc
        //

        HRESULT
        Enable(
            BOOL enable = TRUE
        )
        {
            return m_pDesktopWallpaper->Enable(enable);
        }

        HRESULT
        Disable()
        {
            return m_pDesktopWallpaper->Enable(FALSE);
        }

        HRESULT
        GetStatus(
            DESKTOP_SLIDESHOW_STATE* pState
        )
        {
            return m_pDesktopWallpaper->GetStatus(pState);
        }

    private:

        CComPtr<IDesktopWallpaper> m_pDesktopWallpaper;
};
