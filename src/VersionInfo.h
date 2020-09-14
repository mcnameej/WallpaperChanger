//////////////////////////////////////////////////////////////////////////////
//
//  VersionInfo.h
//
//  CVersionInfo class.
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

// Windows file version information.
class CVersionInfo
{
    public:

        CVersionInfo(
            LPCWSTR pwszFilename = nullptr
        )
            :
            m_pVersionDataBlock(nullptr),
            m_pFixedFileInfo(nullptr),
            m_pLanguages(nullptr)
        {
            if (pwszFilename)
                Load(pwszFilename);
        }

        ~CVersionInfo()
        {
            delete [] m_pVersionDataBlock;
        }

    public:

        bool Load(LPCWSTR pwszFilename)
        {
            DWORD Unused;
            UINT ValueSize;

            // Load() can only be called once!
            ATLASSERT(m_pVersionDataBlock == nullptr);

            // Get size of version information.
            DWORD VersionDataBlockSize = GetFileVersionInfoSizeW(pwszFilename, &Unused);
            if (VersionDataBlockSize == 0)
            {
                DebugPrint(L"CVersionInfo::Load: Invalid file: %s\n", pwszFilename);
                return false;
            }

            // Allocate memory for version information.
            m_pVersionDataBlock = new BYTE[VersionDataBlockSize];

            // Load version information.
            if (!GetFileVersionInfoW(pwszFilename, Unused, VersionDataBlockSize, m_pVersionDataBlock))
            {
                DebugPrint(L"CVersionInfo::Load: Invalid file: %s\n", pwszFilename);
                delete [] m_pVersionDataBlock;
                m_pVersionDataBlock = nullptr;
                return false;
            }

            // Get the fixed file information.
            VerQueryValueW(m_pVersionDataBlock,
                           L"\\",
                           (LPVOID*) &m_pFixedFileInfo,
                           &ValueSize);

            // Get the list of langauges and code pages.
            VerQueryValueW(m_pVersionDataBlock,
                           L"\\VarFileInfo\\Translation",
                           (LPVOID*) &m_pLanguages,
                           &ValueSize);

            return true;
        }

        bool IsOK()
        {
            return m_pVersionDataBlock != nullptr;
        }

        LPCWSTR GetVersionResorceString(LPCWSTR pwszName)
        {
            ATLASSERT(m_pLanguages != nullptr);

            return GetVersionResorceString(pwszName,
                                           m_pLanguages[0].wLanguage,
                                           m_pLanguages[0].wCodePage);
        }

        LPCWSTR GetVersionResorceString(LPCWSTR pwszName, WORD wLanguage, WORD wCodePage)
        {
            ATLASSERT(IsOK());

            // Build "path" to version resource string.
            WCHAR wszPath[MAX_PATH];
            StringCbPrintfW(wszPath, sizeof(wszPath),
                            L"\\StringFileInfo\\%04X%04X\\%s",
                            wLanguage,
                            wCodePage,
                            pwszName);

            // Get string value from version resource.
            LPCWSTR pwszValue = nullptr;
            UINT ValueSize = 0;
            if (!VerQueryValueW(m_pVersionDataBlock,
                                wszPath,
                                (LPVOID*) &pwszValue,
                                &ValueSize)
                || !pwszValue
                || !ValueSize)
            {
                // Return empty string on error, not nullptr.
                return L"";
            }

            return pwszValue;
        }

        LPCWSTR GetFileDescription()
        {
            return GetVersionResorceString(L"FileDescription");
        }

        LPCWSTR GetFileVersion()
        {
            return GetVersionResorceString(L"FileVersion");
        }

        void GetFileVersion(int* pMajor, int* pMinor, int* pBuild, int* pPatch)
        {
            ATLASSERT(m_pFixedFileInfo != nullptr);
            if (pMajor) { *pMajor = HIWORD(m_pFixedFileInfo->dwFileVersionMS); }
            if (pMinor) { *pMinor = LOWORD(m_pFixedFileInfo->dwFileVersionMS); }
            if (pBuild) { *pBuild = HIWORD(m_pFixedFileInfo->dwFileVersionLS); }
            if (pPatch) { *pPatch = LOWORD(m_pFixedFileInfo->dwFileVersionLS); }
        }

        LPCWSTR GetProductName()
        {
            return GetVersionResorceString(L"ProductName");
        }

        LPCWSTR GetProductVersion()
        {
            return GetVersionResorceString(L"ProductVersion");
        }

        void GetProductVersion(int* pMajor, int* pMinor, int* pBuild, int* pPatch)
        {
            ATLASSERT(m_pFixedFileInfo != nullptr);
            if (pMajor) { *pMajor = HIWORD(m_pFixedFileInfo->dwProductVersionMS); }
            if (pMinor) { *pMinor = LOWORD(m_pFixedFileInfo->dwProductVersionMS); }
            if (pBuild) { *pBuild = HIWORD(m_pFixedFileInfo->dwProductVersionLS); }
            if (pPatch) { *pPatch = LOWORD(m_pFixedFileInfo->dwProductVersionLS); }
        }

        LPCWSTR GetCompanyName()
        {
            return GetVersionResorceString(L"CompanyName");
        }

        LPCWSTR GetLegalCopyright()
        {
            return GetVersionResorceString(L"LegalCopyright");
        }

        LPCWSTR GetLegalTrademarks()
        {
            return GetVersionResorceString(L"LegalTrademarks");
        }

    private:

        struct LANGANDCODEPAGE
        {
            WORD wLanguage;
            WORD wCodePage;
        };

        LPVOID m_pVersionDataBlock;

        VS_FIXEDFILEINFO* m_pFixedFileInfo;

        LANGANDCODEPAGE* m_pLanguages;
};

// Application version information.
class CApplicationVersionInfo : public CVersionInfo
{
    public:

        CApplicationVersionInfo()
            :
            CVersionInfo()
        {
            WCHAR wszFilename[MAX_PATH];
            ::GetModuleFileNameW(::GetModuleHandleW(NULL), wszFilename, MAX_PATH);
            Load(wszFilename);
        }
};

#pragma comment(lib, "version")
