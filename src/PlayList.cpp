//////////////////////////////////////////////////////////////////////////////
//
//  PlayList.cpp
//
//  CFileInfo, CFileList, and CPlayList classes.
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

#include "PlayList.h"
#include "WallpaperManager.h"

//============================================================================
//
//  CFileInfo
//
//============================================================================

//////////////////////////////////////////////////////////////////////////////
//
//  CFileInfo::GetLargeThumbnail
//
//////////////////////////////////////////////////////////////////////////////

HBITMAP
CFileInfo::GetLargeThumbnail()
const
{
    HBITMAP hBitmap = NULL;

    CComPtr<IShellItem> pItem;
    HRESULT hr = ::SHCreateItemFromParsingName(m_FullPath.c_str(), nullptr, IID_PPV_ARGS(&pItem));
    if (FAILED(hr))
    {
        // "This should never happen"
        DebugPrint(L"SHCreateItemFromParsingName failed for %s\n", m_FullPath.c_str());
        return NULL;
    }

    CComPtr<IShellItemImageFactory> pImageFactory;
    hr = pItem->QueryInterface(IID_PPV_ARGS(&pImageFactory));
    if (SUCCEEDED(hr))
    {
        hr = pImageFactory->GetImage({256, 256},
                                     SIIGBF_ICONBACKGROUND,
                                     &hBitmap);
        if (FAILED(hr) && hBitmap != NULL)
        {
            // This seems like a very unlikely case (FAILED yet returned a bitmap),
            // but we don't want to leak GDI handles, so we're going to deal with it.
            ::DeleteObject(hBitmap);
            hBitmap = NULL;
        }
    }

    return hBitmap;
}

//============================================================================
//
//  CFileList
//
//============================================================================

//////////////////////////////////////////////////////////////////////////////
//
//  CFileList::Find
//
//////////////////////////////////////////////////////////////////////////////

CFileList::iterator
CFileList::Find(
    const fs::path& path
)
{
    for (iterator it = begin(); it != end(); ++it)
    {
        if ((*it)->m_FullPath == path)
            return it;
    }
    return end();
}

CFileList::iterator
CFileList::Find(
    const CFileInfo* pFile
)
{
    for (iterator it = begin(); it != end(); ++it)
    {
        if (*it == pFile)
            return it;
    }
    return end();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CFileList::Add
//
//////////////////////////////////////////////////////////////////////////////

CFileList::iterator
CFileList::Add(
    const fs::path& path,
    bool recurseIntoSubdirs /*= false*/
)
{
    if (fs::is_regular_file(path))
    {
        return AddIfNew(path);
    }

    if (fs::is_directory(path))
    {
        // Keep track of the index of the first file added, so we can create
        // an iterator when we're done. Can't create the iterator while adding
        // files because the next push_back() would invalidate it.
        size_t firstFileAdded = (size_t) -1;

        if (recurseIntoSubdirs)
        {
            for (const auto& entry: fs::recursive_directory_iterator(path))
            {
                if (entry.is_regular_file())
                {
                    if (AddIfNew(entry.path()) != end())
                    {
                        if (firstFileAdded == -1)
                            firstFileAdded = m_Files.size() - 1;
                    }
                }
            }
        }
        else
        {
            for (const auto& entry: fs::directory_iterator(path))
            {
                if (entry.is_regular_file())
                {
                    if (AddIfNew(entry.path()) != end())
                    {
                        if (firstFileAdded == -1)
                            firstFileAdded = m_Files.size() - 1;
                    }
                }
            }
        }

        return (firstFileAdded != -1) ? iat(firstFileAdded) : end();
    }

    return end();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CFileList::AddIfNew
//
//////////////////////////////////////////////////////////////////////////////

CFileList::iterator
CFileList::AddIfNew(
    const fs::path& path
)
{
    if (Find(path) == end())
    {
        // New file.
        m_Files.push_back(new CFileInfo(path));
        return m_Files.end() - 1;
    }
    else
    {
        // File already exists.
        return end();
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  CFileList::Sort
//
//////////////////////////////////////////////////////////////////////////////

void
CFileList::Sort()
{
    using FileInfoPtr = CFileInfo*;

    std::sort(m_Files.begin(),
              m_Files.end(),
              [] (const FileInfoPtr p1, const FileInfoPtr p2)
              {
#if 1
                  // Sort using case insensitive comparison of display name.
                  // The net effect is to ignore directories.
                  return _wcsicmp(p1->GetDisplayName(), p2->GetDisplayName()) < 0;
#else
                  // Sort using case insensitive comparison of full path.
                  // The net effect is to sort files by directory, and then
                  // by file name and extension.
                  // return _wcsicmp(p1->m_FullPath.c_str(), p2->m_FullPath.c_str()) < 0;
#endif
              });
}

//////////////////////////////////////////////////////////////////////////////
//
//  CFileList::Shuffle
//
//////////////////////////////////////////////////////////////////////////////

void
CFileList::Shuffle()
{
    std::random_device seed;
    std::mt19937 rng(seed());
    std::shuffle(m_Files.begin(), m_Files.end(), rng);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CFileList::GetRandom
//
//////////////////////////////////////////////////////////////////////////////

CFileInfo*
CFileList::GetRandom()
{
    ATLASSERT(!m_Files.empty());
    std::random_device seed;
    std::mt19937 rng(seed());
    return m_Files[rng() % m_Files.size()];
}

//////////////////////////////////////////////////////////////////////////////
//
//  CFileList::clear
//
//////////////////////////////////////////////////////////////////////////////

void
CFileList::clear()
noexcept
{
    // Remove all CFileInfo objects from the file list.
    // Called by dtor, and can also be called by application.

    for (CFileInfo* pFile: m_Files)
        delete pFile;

    m_Files.clear();

    m_Files.shrink_to_fit();
}

//////////////////////////////////////////////////////////////////////////////
//
//  CFileList::Remove
//
//////////////////////////////////////////////////////////////////////////////

bool
CFileList::Remove(
    iterator it
)
{
    CFileInfo* pFile = *it;

    delete pFile;

    m_Files.erase(it);

    return true;
}

//============================================================================
//
//  CPlayList
//
//============================================================================

//////////////////////////////////////////////////////////////////////////////
//
//  CPlayList::Load
//
//////////////////////////////////////////////////////////////////////////////

bool
CPlayList::Load(
    const fs::path& path
)
{
    clear();

    m_PlaylistPath = path;

    bool ok = true;

    ForEachLineOfFile(
        path,
        /*LAMBDA*/ [this, &ok] (const std::wstring& lineW)
        {
            // Ignore comment lines.
            if (lineW.front() == L';' || lineW.front() == L'#')
                return true;

            // Add file to the list.
            if (this->Add(lineW) == this->end())
            {
                DebugPrint(L"CFileList::Load: Failed to add: %s\n", lineW.c_str());
                ok = false;
            }

            return true;
        }
    );

    return ok;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CPlayList::Save
//
//////////////////////////////////////////////////////////////////////////////

bool
CPlayList::Save(
    const fs::path& path
)
{
    m_PlaylistPath = path;

    // Rename current playlist file (if any) to .bak extension.
    // The intent is to provide *some* recovery mechanism in case
    // of failure during playlist write.  Recovery isn't exposed
    // in UI -- a sophisticated user must go into the application
    // data directory and manually fix it.
    if (fs::is_regular_file(path))
    {
        std::error_code ec;
        fs::path bakFilePath = fs::path(path).replace_extension(L"bak");
        fs::remove(bakFilePath, ec);
        fs::rename(path, bakFilePath, ec);
    }

    FILE* fp = _wfopen(path.c_str(), L"wb");

    if (!fp)
        return false;

    fputs("# Wallpaper Changer playlist\r\n\r\n", fp);

    for (const CFileInfo* pFile: m_Files)
    {
        std::string strU8 = UTF16_to_UTF8(pFile->m_FullPath);
        fwrite(strU8.c_str(), strU8.size(), 1, fp);
        fputs("\r\n", fp);
    }

    fclose(fp);

    return true;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CPlayList::NameToPath
//
//////////////////////////////////////////////////////////////////////////////

/*static*/
fs::path
CPlayList::NameToPath(
    ConstWString playlistName
)
{
    if (!playlistName || !playlistName[0])
    {
        ATLASSERT(false);
        playlistName = L"Default";
    }

    fs::path playlistPath = (LPCWSTR) playlistName;

    // Get absolute path.
    if (playlistPath.is_relative())
        playlistPath = GetApp()->GetAppDataDirectory() / playlistPath;

    // Use .WPPL extension for playlist files.
    playlistPath.replace_extension(L"wppl");

    return playlistPath;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CPlayList::GetAllPlaylists
//
//////////////////////////////////////////////////////////////////////////////

/*static*/
std::vector<std::wstring>
CPlayList::GetAllPlaylists()
{
    std::vector<std::wstring> playlistNames;

    for (const auto& entry: fs::directory_iterator(GetApp()->GetAppDataDirectory()))
    {
        if (_wcsicmp(entry.path().extension().c_str(), L".WPPL") == 0)
        {
            playlistNames.push_back(entry.path().stem());
        }
    }

    std::sort(playlistNames.begin(),
              playlistNames.end(),
              [] (const std::wstring& n1, const std::wstring& n2)
              { return _wcsicmp(n1.c_str(), n2.c_str()) < 0; });

    return playlistNames;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CPlayList::CreateDefaultPlaylists
//
//  Called from CWallpaperChangerOptions::LoadFromRegistry().
//  Not intended to be called from anywhere else.
//
//////////////////////////////////////////////////////////////////////////////

// Add wallpaper images from standard "Windows 10" theme to playlist.
static void AddStandardWallpaper(CPlayList& playlist)
{
    fs::path standardWallpaperPath = GetKnownFolderPath(FOLDERID_Windows) / L"Web\\Wallpaper\\Theme1";

    for (int imageNum = 1; imageNum <= 4; imageNum++)
    {
        fs::path imagePath = standardWallpaperPath / Format(L"img%d.jpg", imageNum);

        if (fs::is_regular_file(imagePath))
            playlist.Add(imagePath);
    }
}

/*static*/
void
CPlayList::CreateDefaultPlaylists()
{
    // Create the "Default" playlist.
    {
        fs::path playlistPath = NameToPath(L"Default");

        if (!fs::is_regular_file(playlistPath))
        {
            CPlayList defaultPlaylist;

            // Add the currently displayed wallpaper to the Default playlist.
            fs::path currentWindowsWallpaper = CWallpaperManager::GetCurrentWindowsWallpaperFile();
            if (!currentWindowsWallpaper.empty() &&
                fs::is_regular_file(currentWindowsWallpaper))
            {
                defaultPlaylist.Add(currentWindowsWallpaper);
            }

            // TODO(maybe): Add Windows slideshow images to the Default playlist.

            // Add images from the "Windows 10" theme to the Default playlist.
            AddStandardWallpaper(defaultPlaylist);

            defaultPlaylist.Save(playlistPath);

            GetAppOptions()->UsePlaylist(defaultPlaylist.GetPlaylistName());
        }
    }

    // Create the "Safe For Work" playlist.
    {
        fs::path playlistPath = NameToPath(L"SFW");

        if (!fs::is_regular_file(playlistPath))
        {
            CPlayList safePlaylist;

            AddStandardWallpaper(safePlaylist);

            safePlaylist.Save(playlistPath);

            if (GetAppOptions()->m_SafePlaylist.empty())
            {
                GetAppOptions()->m_SafePlaylist = safePlaylist.GetPlaylistName();
                GetAppOptions()->SaveToRegistry();
            }
        }
    }
}
