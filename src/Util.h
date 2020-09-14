//////////////////////////////////////////////////////////////////////////////
//
//  Util.h
//
//  Utility classes and functions.
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

//////////////////////////////////////////////////////////////////////////////
//
//  Utility class that allows passing C++ string or C string as a parameter.
//  Works around std::basic_string<T> not having an implicit conversion to
//  const T*.  Avoids having to sprinkle c_str() calls all over the place.
//
//////////////////////////////////////////////////////////////////////////////

// Allow passing C++ string or C string as a function call parameter.
template <typename CHAR_TYPE>
class ConstStringTemplate
{
    public:

        using const_pointer = const CHAR_TYPE *;

        ConstStringTemplate()
            : m_pString(nullptr)
        {}

        ConstStringTemplate(const_pointer pString)
            : m_pString(pString)
        {}

        ConstStringTemplate(const std::basic_string<CHAR_TYPE>& string)
            : m_pString(string.c_str())
        {}

        ConstStringTemplate(const std::filesystem::path& path)
            : m_pString(path.c_str())
        {}

        operator const_pointer() const
        { return m_pString; }

        operator bool() const
        { return m_pString != nullptr; }

        const_pointer c_str() const
        { return m_pString; }

    private:

        const_pointer m_pString;
};

using ConstAString = ConstStringTemplate<char>;
using ConstWString = ConstStringTemplate<WCHAR>;
using ConstTString = ConstStringTemplate<TCHAR>;

//////////////////////////////////////////////////////////////////////////////
//
//  CBoolResult
//
//  This class has a limited use case, but it comes up reasonably often...
//
//  (1) You're calling a series of functions that return a boolean to
//      indicate whether they succeeded or failed.
//
//  (2) You need to call all the functions, regardless of whether any
//      of them fail.
//
//  (3) You need to keep track of whether any function failed.
//
//  Doing this manually requires code like this...
//
//      bool ok = true;
//      ok = !func1("foo", 0) ? false : ok;
//      ok = !func2("bar", 3.14) ? false : ok;
//      ok = !func3("and grill", 42) ? false : ok;
//
//  Needing a ternary operator at the end of each statement makes the
//  code hard to read. Hence this class...
//
//      CBoolResult ok;
//      ok |= func1("foo", 0);
//      ok |= func2("bar", 3.14);
//      ok |= func3("and grill", 42);
//
//  Another reasonably common use case is calling a series of functions,
//  but wanting to stop as soon as one fails.  Plain C++ code works well
//  for that...
//
//      bool ok = true;
//      ok = ok && func1("foo", 0);
//      ok = ok && func2("bar", 3.14);
//      ok = ok && func3("and grill", 42);
//
//////////////////////////////////////////////////////////////////////////////

// Boolean result accumulator.
class CBoolResult
{
    public:

        // Construct result accumulator object.
        CBoolResult(
            bool result = true
        )
            :
            m_result(result)
        {
        }

        // Reset the result accumulator.
        void
        reset(
            bool result = true
        )
        {
            m_result = result;
        }

        // Accumulate result (C++ bool).
        CBoolResult& operator |=(bool result)
        {
            if (!result)
                m_result = false;
            return *this;
        }

        operator bool() const
        {
            return m_result;
        }

    private:

        bool m_result;
};

//////////////////////////////////////////////////////////////////////////////
//
//  Misc. functions
//
//////////////////////////////////////////////////////////////////////////////

// Report fatal error and terminate process.
DECLSPEC_NORETURN
void
ReoprtFailureAndExit(
    HRESULT hr,
    LPCWSTR pwszFunction = nullptr
);

// Report fatal error and terminate process.
DECLSPEC_NORETURN
FORCEINLINE
void
ReoprtFailureAndExit(
    LPCWSTR pwszMessage
)
{
    ReoprtFailureAndExit(0, pwszMessage);
}

// Get path to a standard Windows directory.
fs::path
GetKnownFolderPath(
    REFKNOWNFOLDERID rfid
);

#if USING_GDIPLUS

// Get the GDI+ codec for a specific MIME type.
CLSID
GetImageCodecClass(
    LPCWSTR pwszMimeType
);

#endif

// Check if file is a valid wallpaper image.
bool
IsValidImageFile(
    ConstWString imageFile
);

// Expand environment variables in a string.
std::wstring
ExpandEnvironmentVariables(
    ConstWString src
);

// Get the full path of our executable (.EXE file).
std::wstring
GetExecutablePath();

// Add quotes around path if it contains spaces.
std::wstring
QuoteIfNeeded(
   ConstWString path
);

// Load string resource into std::wstring.
std::wstring
LoadStringResourceW(
    UINT ID
);

// Trim whitespace from start and end of string.
std::wstring
TrimWhitespace(
    const std::wstring& str
);

// Get window text into a std::wstring.
std::wstring
GetWindowTextStringW(
    CWindow window
);

// Get menu item text into a std::wstring.
std::wstring
GetMenuItemStringW(
    CMenuHandle menu,
    UINT ID,
    UINT flags = MF_BYCOMMAND
);

// Get size of an open file.
inline
size_t
GetOpenFileSize(
    HANDLE hFile
)
{
    ULARGE_INTEGER fileSize;
    fileSize.LowPart = ::GetFileSize(hFile, &fileSize.HighPart);
    return fileSize.QuadPart;
}

// Read entire file into memory.
std::string
SlurpFile(
    ConstWString path,
    size_t maximumSize = 16 * 1024 * 1024
);

// Write memory buffer to file.
bool
WriteDataToFile(
    ConstWString path,
    const void* pData,
    size_t dataSize
);

// Write memory buffer to file.
inline bool WriteDataToFile(ConstWString path, const std::string& buffer)
{
    return WriteDataToFile(path, &buffer[0], buffer.size());
}

// Write memory buffer to file.
template <typename T>
bool WriteDataToFile(ConstWString path, const std::vector<T>& buffer)
{
    return WriteDataToFile(path, &buffer[0], buffer.size() * sizeof(T));
}

// Call a function for every line in a text file.
// File contents are expected to be in UTF-8 format,
// which is converted to UTF-16 (std::wstring) before
// calling the processing function.
bool
ForEachLineOfFile(
    ConstWString path,
    std::function<bool(const std::wstring&)> processLine
);

// Convert UTF-16 text to UTF-8 std::string.
std::string
UTF16_to_UTF8(
    std::wstring::const_iterator startIter,
    std::wstring::const_iterator endIter
);

// Convert UTF-16 std::wstring to UTF-8 std::string.
FORCEINLINE
std::string
UTF16_to_UTF8(
    const std::wstring& strW
)
{
    return UTF16_to_UTF8(strW.cbegin(), strW.cend());
}

// Convert UTF-8 text to UTF-16 std::wstring.
std::wstring
UTF8_to_UTF16(
    std::string::const_iterator startIter,
    std::string::const_iterator endIter
);

// Convert UTF-8 std::string to UTF-16 std::wstring.
FORCEINLINE
std::wstring
UTF8_to_UTF16(
    const std::string& strU8
)
{
    return UTF8_to_UTF16(strU8.begin(), strU8.end());
}

// "sprintf" into std::string.
std::string
__cdecl
Format(
    ConstAString format,
    ...
);

// "sprintf" into std::wstring.
std::wstring
__cdecl
Format(
    ConstWString format,
    ...
);
