//////////////////////////////////////////////////////////////////////////////
//
//  Util.cpp
//
//  Utility functions.
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

#include "Util.h"

//////////////////////////////////////////////////////////////////////////////
//
//  ReoprtFailureAndExit
//
//////////////////////////////////////////////////////////////////////////////

DECLSPEC_NORETURN
void
ReoprtFailureAndExit(
    HRESULT hr,
    LPCWSTR pwszFunction /*= nullptr*/
)
{
    WCHAR wszMessage[256];

    if (hr == 0 && pwszFunction)
        StringCbCopy(wszMessage, sizeof(wszMessage), pwszFunction);
    else if (hr != 0 && pwszFunction)
        StringCbPrintf(wszMessage, sizeof(wszMessage), L"%s returned HRESULT 0x%08X", pwszFunction, hr);
    else
        StringCbPrintf(wszMessage, sizeof(wszMessage), L"HRESULT 0x%08X", hr);

#ifdef _DEBUG
    OutputDebugStringW(TEXT("Fatal error: "));
    OutputDebugStringW(wszMessage);
    OutputDebugStringW(TEXT("\n"));
#endif

    ::MessageBoxW(NULL, wszMessage, L"Fatal error", MB_ICONEXCLAMATION | MB_OK);

#ifdef _DEBUG
    DebugBreak();
#endif

    ExitProcess(1);
}

//////////////////////////////////////////////////////////////////////////////
//
//  GetKnownFolderPath
//
//////////////////////////////////////////////////////////////////////////////

fs::path
GetKnownFolderPath(
    REFKNOWNFOLDERID rfid
)
{
    fs::path knownFolderPath;

    PWSTR pwszFolder = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(rfid, 0, NULL, &pwszFolder)))
    {
        knownFolderPath = pwszFolder;
        CoTaskMemFree(pwszFolder);
    }

    return knownFolderPath;
}

//////////////////////////////////////////////////////////////////////////////
//
//  GetImageCodecClass
//
//////////////////////////////////////////////////////////////////////////////

#if USING_GDIPLUS

// Get the list of GDI+ image codecs.
static
bool
GetImageCodecList(
    CHeapPtr<GP::ImageCodecInfo>& pImageCodecList,
    UINT& count
)
{
    count = 0;
    UINT size = 0;

    if (GP::GetImageEncodersSize(&count, &size) != GP::Ok || size == 0)
        return false;

    if (!pImageCodecList.AllocateBytes(size))
        return false;

    if (GP::GetImageEncoders(count, size, pImageCodecList) != GP::Ok)
        return false;

    return true;
}

CLSID
GetImageCodecClass(
    LPCWSTR pwszMimeType
)
{
    CLSID encoderClass = { 0 };

    CHeapPtr<GP::ImageCodecInfo> pImageCodecList;
    UINT count;

    if (GetImageCodecList(pImageCodecList, count))
    {
        for (UINT idx = 0; idx < count; idx++)
        {
            if (wcscmp(pImageCodecList[idx].MimeType, pwszMimeType) == 0)
            {
                encoderClass = pImageCodecList[idx].Clsid;
                break;
            }
        }
    }

    return encoderClass;
}

#endif

//////////////////////////////////////////////////////////////////////////////
//
//  IsValidImageFile
//
//////////////////////////////////////////////////////////////////////////////

bool
IsValidImageFile(
    ConstWString imageFile
)
{
    // Ideally we'd validate the image by attempting to load it,
    // but that takes a lot of time.  For now, I'm just going to
    // check the file extension against a whitelist.

    static LPCWSTR validExtensions[] =
    {
        L".bmp",
        L".jpg",
        L".jpeg",
        L".png"
    };

    std::wstring ext = fs::path((LPCWSTR)imageFile).extension();

    for (int idx = 0; idx < ARRAYSIZE(validExtensions); idx++)
    {
        if (_wcsicmp(ext.c_str(), validExtensions[idx]) == 0)
            return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////////
//
//  ExpandEnvironmentVariables
//
//////////////////////////////////////////////////////////////////////////////

std::wstring
ExpandEnvironmentVariables(
    ConstWString src
)
{
    ATLASSERT(src != nullptr);

    std::wstring dst;

    DWORD dwSize = ::ExpandEnvironmentStringsW(src, &dst[0], 0);

    if (dwSize > 0)
    {
        dst.resize(dwSize-1);
        ::ExpandEnvironmentStringsW(src, &dst[0], dwSize);
    }

    return dst;
}

//////////////////////////////////////////////////////////////////////////////
//
//  GetExecutablePath
//
//////////////////////////////////////////////////////////////////////////////

std::wstring
GetExecutablePath()
{
    WCHAR szExePath[MAX_PATH];

    ::GetModuleFileNameW(::GetModuleHandleW(NULL), szExePath, MAX_PATH);

    return szExePath;
}

//////////////////////////////////////////////////////////////////////////////
//
//  QuoteIfNeeded
//
//////////////////////////////////////////////////////////////////////////////

std::wstring
QuoteIfNeeded(
   ConstWString path
)
{
    if (wcschr(path, L' ') == NULL)
        return std::wstring(path);
    else
        return L"\"" + std::wstring(path) + L"\"";
}

//////////////////////////////////////////////////////////////////////////////
//
//  LoadStringResourceW
//
//////////////////////////////////////////////////////////////////////////////

std::wstring
LoadStringResourceW(
    UINT ID
)
{
    LPCWSTR pwszString;
    size_t length = ::LoadStringW(ModuleHelper::GetResourceInstance(), ID, (LPWSTR) &pwszString, 0);
    return std::wstring(pwszString, length);
}

//////////////////////////////////////////////////////////////////////////////
//
//  TrimWhitespace
//
//////////////////////////////////////////////////////////////////////////////

template <class STRING_ITER>
STRING_ITER
TrimLeadingWhitespace(
    STRING_ITER startIter,
    STRING_ITER endIter
)
{
    while (startIter != endIter)
    {
        UINT ch = *startIter;
        if (ch != ' ' && ch != '\t')
            break;
        ++startIter;
    }
    return startIter;
}

template <class STRING_ITER>
STRING_ITER
TrimTrailingWhitespace(
    STRING_ITER startIter,
    STRING_ITER endIter
)
{
    while (startIter != endIter)
    {
        UINT ch = endIter[-1];
        if (ch != ' ' && ch != '\t')
            break;
        --endIter;
    }
    return endIter;
}

std::wstring
TrimWhitespace(
    const std::wstring& str
)
{
    auto startIter = TrimLeadingWhitespace(str.begin(), str.end());
    auto endIter   = TrimTrailingWhitespace(startIter, str.end());
    return std::wstring(startIter, endIter);
}

//////////////////////////////////////////////////////////////////////////////
//
//  GetWindowTextStringW
//
//////////////////////////////////////////////////////////////////////////////

std::wstring
GetWindowTextStringW(
    CWindow window
)
{
    std::wstring str;

    int textLength = window.GetWindowTextLengthW();

    if (textLength != 0)
    {
        str.resize(textLength);
        window.GetWindowTextW(str.data(), textLength + 1);
    }

    return str;
}

//////////////////////////////////////////////////////////////////////////////
//
//  GetMenuItemStringW
//
//////////////////////////////////////////////////////////////////////////////

std::wstring
GetMenuItemStringW(
    CMenuHandle menu,
    UINT id,
    UINT flags /*= MF_BYCOMMAND*/
)
{
    std::wstring str;

    int textLength = menu.GetMenuStringLen(id, flags);

    if (textLength != 0)
    {
        str.resize(textLength);
        menu.GetMenuStringW(id, str.data(), textLength + 1, flags);
    }

    return str;
}

//////////////////////////////////////////////////////////////////////////////
//
//  SlurpFile
//
//////////////////////////////////////////////////////////////////////////////

std::string
SlurpFile(
    ConstWString path,
    size_t maximumSize /*= 16 * 1024 * 1024*/
)
{
    std::string fileData;

    HANDLE hFile = ::CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        // Get file size.
        size_t fileSize = GetOpenFileSize(hFile);

        // Make sure file is within size limit.
        if (fileSize != 0 && fileSize <= maximumSize)
        {
            // Allocate memory for file data.
            fileData.resize(fileSize);

            // Read the whole file into memory.
            DWORD dwBytesRead = 0;
            if (!::ReadFile(hFile, fileData.data(), (DWORD) fileData.size(), &dwBytesRead, NULL))
                dwBytesRead = 0;

            // Clear the buffer if there was a read error.
            if (dwBytesRead != fileData.size())
                fileData.clear();
        }

        // Close the file.
        ::CloseHandle(hFile);
    }

    return fileData;
}

//////////////////////////////////////////////////////////////////////////////
//
//  WriteDataToFile
//
//////////////////////////////////////////////////////////////////////////////

bool
WriteDataToFile(
    ConstWString path,
    const void* pData,
    size_t dataSize
)
{
    ATLASSERT(pData != nullptr);

    HANDLE hFile = ::CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD dwWritten = 0;
    if (!::WriteFile(hFile, pData, (DWORD) dataSize, &dwWritten, NULL))
        dwWritten = 0;

    ::CloseHandle(hFile);

    return (dwWritten == dataSize);
}

//////////////////////////////////////////////////////////////////////////////
//
//  ForEachLineOfFile
//
//////////////////////////////////////////////////////////////////////////////

bool
ForEachLineOfFile(
    ConstWString path,
    std::function<bool(const std::wstring &)> processLine
)
{
    //
    // Read file into memory.
    //

    auto fileData = SlurpFile(path);

    //
    // Parse file
    //

    // TODO(maybe): Handle either UTF8 or UTF16 files.

    char lineEndings[2] = { '\r', '\n' };

    for (auto startOfLine = fileData.cbegin(); startOfLine != fileData.cend(); /**/)
    {
        // Find end of line (CR, LF, or end of buffer).

        auto endOfLine = std::find_first_of(startOfLine, fileData.cend(),
                                            std::cbegin(lineEndings), std::cend(lineEndings));

        if (endOfLine != startOfLine)
        {
            // Convert line from UTF-8 to UTF-16 (WCHAR).

            std::wstring lineW = UTF8_to_UTF16(TrimLeadingWhitespace(startOfLine, endOfLine),
                                               TrimTrailingWhitespace(startOfLine, endOfLine));

            // Process the line.

            if (!lineW.empty())
            {
                if (!processLine(lineW))
                    return false;
            }
        }

        // Skip CR/LF or LF at end of line.

        if (endOfLine != fileData.cend() && *endOfLine == '\r')
            ++endOfLine;
        if (endOfLine != fileData.cend() && *endOfLine == '\n')
            ++endOfLine;

        startOfLine = endOfLine;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////
//
//  UTF16_to_UTF8
//
//////////////////////////////////////////////////////////////////////////////

std::string
UTF16_to_UTF8(
    std::wstring::const_iterator startIter,
    std::wstring::const_iterator endIter
)
{
    std::string strU8;

    const int strLengthU16 = (int)(endIter - startIter);

    if (strLengthU16 != 0)
    {
        const LPCWSTR strDataU16 = &*startIter;

        int strLengthU8 = ::WideCharToMultiByte(CP_UTF8, 0,
                                                strDataU16, strLengthU16,
                                                nullptr, 0,
                                                nullptr, nullptr);

        strU8.resize(strLengthU8);

        ::WideCharToMultiByte(CP_UTF8, 0,
                              strDataU16, strLengthU16,
                              strU8.data(), strLengthU8,
                              nullptr, nullptr);
    }

    return strU8;
}

//////////////////////////////////////////////////////////////////////////////
//
//  UTF8_to_UTF16
//
//////////////////////////////////////////////////////////////////////////////

std::wstring
UTF8_to_UTF16(
    std::string::const_iterator startIter,
    std::string::const_iterator endIter
)
{
    std::wstring strU16;

    const int strLengthU8 = (int)(endIter - startIter);

    if (strLengthU8 != 0)
    {
        const LPCSTR strDataU8 = &*startIter;

        int strLengthU16 = ::MultiByteToWideChar(CP_UTF8, 0,
                                                strDataU8, strLengthU8,
                                                nullptr, 0);

        strU16.resize(strLengthU16);

        ::MultiByteToWideChar(CP_UTF8, 0,
                              strDataU8, strLengthU8,
                              strU16.data(), strLengthU16);
    }

    return strU16;
}

//////////////////////////////////////////////////////////////////////////////
//
//  Format
//
//////////////////////////////////////////////////////////////////////////////

std::string
__cdecl
Format(
    ConstAString format,
    ...
)
{
    std::string strResult;
    va_list args;

    ATLASSERT(format);

    va_start(args, format);
    int length = _vsnprintf(nullptr, 0, format, args);
    va_end(args);

    if (length > 0)
    {
        strResult.resize(length);

        va_start(args, format);
        _vsnprintf(strResult.data(), length+1, format, args);
        va_end(args);
    }

    return strResult;
}

std::wstring
__cdecl
Format(
    ConstWString format,
    ...
)
{
    std::wstring strResult;
    va_list args;

    ATLASSERT(format);

    va_start(args, format);
    int length = _vsnwprintf(nullptr, 0, format, args);
    va_end(args);

    if (length > 0)
    {
        strResult.resize(length);

        va_start(args, format);
        _vsnwprintf(strResult.data(), length+1, format, args);
        va_end(args);
    }

    return strResult;
}
