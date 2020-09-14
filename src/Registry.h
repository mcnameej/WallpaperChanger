//////////////////////////////////////////////////////////////////////////////
//
//  Registry.h
//
//  CRegistryKey class.
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

#include "Util.h"

// Registry key.
class CRegistryKey
{
    //------------------------------------------------------------------------
    //
    //  Constructors, destructor, and operators.
    //
    //------------------------------------------------------------------------

    public:

        // Default ctor.
        CRegistryKey()
            :
            m_hKey(NULL)
        {
        }

        // Create CRegistryKey by attaching an existing HKEY.
        explicit
        CRegistryKey(
            HKEY hKey
        )
            :
            m_hKey(hKey)
        {
        }

        // Create CRegistryKey from HKEY and subkey name.
        CRegistryKey(
            HKEY hKey,                  // Must not be NULL.
            ConstWString pwszSubKey,    // Must not be NULL.
            REGSAM accessMask = MAXIMUM_ALLOWED,
            DWORD dwOptions = 0
        )
        {
            ATLASSERT(hKey && pwszSubKey);
            m_hKey = CreateHKEY(hKey, pwszSubKey, accessMask, dwOptions);
        }

        // Close HKEY when CRegistryKey object is destroyed.
        ~CRegistryKey()
        {
            Close();
        }

        // Move ctor.
        CRegistryKey(
            CRegistryKey&& that
        ) noexcept
            :
            m_hKey(NULL)
        {
            std::swap(m_hKey, that.m_hKey);
        }

        // Move assignment.
        CRegistryKey&
        operator=(
            CRegistryKey&& that
        )
        noexcept
        {
            if (this != &that)
            {
                Close();
                std::swap(m_hKey, that.m_hKey);
            }
            return *this;
        }

        // No copy ctor.
        CRegistryKey(const CRegistryKey&) = delete;

        // No copy assignment.
        CRegistryKey& operator=(const CRegistryKey&) = delete;

        // Test m_hKey for non-NULL if CRegistryKey used as boolean (e.g. if statement).
        operator bool() const
        {
            return IsOpen();
        }

        // Convert CRegistryKey to HKEY.
        operator HKEY() const
        {
            return m_hKey;
        }

    //------------------------------------------------------------------------
    //
    //  Create, Open, Close, Attach, and Detach.
    //
    //------------------------------------------------------------------------

    public:

        // Create or open subkey, and modify the CRegistryKey object to refer to subkey.
        // The current key is closed regardless of whether the subkey is created/opened.
        bool
        Create(
            ConstWString pwszSubKey,  // Must not be NULL.
            REGSAM accessMask = KEY_ALL_ACCESS,
            DWORD dwOptions = 0
        )
        {
            ATLASSERT(m_hKey && pwszSubKey);
            HKEY hKey = CreateHKEY(m_hKey, pwszSubKey, accessMask, dwOptions);
            Close();
            m_hKey = hKey;
            return IsOpen();
        }

        // Open subkey, and modify the CRegistryKey object to refer to subkey.
        // The current key is closed regardless of whether the subkey is opened.
        bool
        Open(
            ConstWString pwszSubKey,
            REGSAM accessMask = MAXIMUM_ALLOWED,
            DWORD dwOptions = 0
        )
        {
            ATLASSERT(m_hKey && pwszSubKey);
            HKEY hKey = OpenHKEY(m_hKey, pwszSubKey, accessMask, dwOptions);
            Close();
            m_hKey = hKey;
            return IsOpen();
        }

        // Close key.
        void
        Close()
        {
            if (m_hKey)
            {
                ::CloseHandle(m_hKey);
                m_hKey = NULL;
            }
        }

        // Attach HKEY from CRegistryKey object.
        void
        Attach(
            HKEY hKey
        )
        {
            Close();
            m_hKey = hKey;
        }        

        // Detach HKEY from CRegistryKey object.
        HKEY
        Detach()
        {
            HKEY hKey = m_hKey;
            m_hKey = NULL;
            return hKey;
        }        

    //------------------------------------------------------------------------
    //
    //  Misc.
    //
    //------------------------------------------------------------------------

    public:
        // Is key open?
        bool
        IsOpen()
        const
        {
            return !!m_hKey;
        }

        // Check if value exists.
        bool
        ValueExists(
            ConstWString pwszValueName
        )
        const
        {
            DWORD dwType, dwSize;
            return GetValueTypeAndSize(pwszValueName, &dwType, &dwSize);
        }

        // Check if subkey exists.
        bool
        KeyExists(
            ConstWString pwszSubKey
        )
        const
        {
            ATLASSERT(m_hKey && pwszSubKey);
            HKEY hKey = OpenHKEY(m_hKey, pwszSubKey, 0, KEY_READ);
            if (hKey)
                ::CloseHandle(hKey);
            return !!hKey;
        }

    //------------------------------------------------------------------------
    //
    //  Delete.
    //
    //------------------------------------------------------------------------

    public:

        // Delete value.
        bool
        DeleteValue(
            ConstWString pwszValueName
        )
        const
        {
            return ::RegDeleteValueW(m_hKey, pwszValueName) == ERROR_SUCCESS;
        }

        // Delete subkey. Subkey must be empty. Use DeleteTree() to nuke it from space.
        bool
        DeleteKey(
            ConstWString pwszSubKey, // Must not be NULL.
            REGSAM regView = 0       // Can be KEY_WOW64_32KEY or KEY_WOW64_64KEY to force a specific view.
        )
        const
        {
            ATLASSERT(!!pwszSubKey);
            return ::RegDeleteKeyExW(m_hKey, pwszSubKey, regView, 0) == ERROR_SUCCESS;
        }

        // Delete subkey tree.
        bool
        DeleteTree(
            ConstWString pwszSubKey  // Pass NULL to delete *all* subkeys and values of the current key.
        )
        const
        {
            return ::RegDeleteTreeW(m_hKey, pwszSubKey) == ERROR_SUCCESS;
        }

    //------------------------------------------------------------------------
    //
    //  Read registry value.
    //
    //  The Read() function is overloaded by value type.
    //  Variable to receive the value is passed by reference.
    //
    //------------------------------------------------------------------------

    public:

        // Get value type and size.
        bool
        GetValueTypeAndSize(
            ConstWString pwszValueName,
            DWORD* pdwType,
            DWORD* pdwValueSize
        )
        const
        {
            return !!m_hKey &&
                   ::RegQueryValueExW(m_hKey,
                                      pwszValueName,
                                      NULL,
                                      pdwType,
                                      NULL,
                                      pdwValueSize) == ERROR_SUCCESS;
        }

        // Get value type.
        DWORD
        GetValueType(
            ConstWString pwszValueName
        )
        const
        {
            DWORD dwType;
            return GetValueTypeAndSize(pwszValueName, &dwType, nullptr) ? dwType : REG_NONE;
        }

        // Get value size.
        DWORD
        GetValueSize(
            ConstWString pwszValueName
        )
        const
        {
            DWORD dwSize;
            return GetValueTypeAndSize(pwszValueName, nullptr, &dwSize) ? dwSize : 0;
        }

        // Read arbitrary value.
        bool
        ReadValue(
            LPCWSTR pwszValueName,
            DWORD* pdwType,
            void* pValue,
            DWORD* pdwValueSize
        )
        const
        {
            return !!m_hKey &&
                   ::RegQueryValueExW(m_hKey,
                                      pwszValueName,
                                      NULL,
                                      pdwType,
                                      (LPBYTE) pValue,
                                      pdwValueSize) == ERROR_SUCCESS;
        }

        // Read REG_DWORD value.
        bool
        Read(
            ConstWString pwszValueName,
            DWORD& dwValue
        )
        const
        {
            DWORD dwType = REG_NONE;
            DWORD dwSize = sizeof(DWORD);
            return ReadValue(pwszValueName, &dwType, &dwValue, &dwSize) && dwType == REG_DWORD;
        }

        // Read REG_QWORD value.
        bool
        Read(
            ConstWString pwszValueName,
            UINT64& qwValue
        )
        const
        {
            DWORD dwType = REG_NONE;
            DWORD dwSize = sizeof(UINT64);
            return ReadValue(pwszValueName, &dwType, &qwValue, &dwSize) && dwType == REG_QWORD;
        }

        // Read REG_BINARY value.
        bool
        Read(
            ConstWString pwszValueName,
            void* pValue,
            DWORD* pdwValueSize
        )
        const
        {
            DWORD dwType = REG_NONE;
            return ReadValue(pwszValueName, &dwType, pValue, pdwValueSize) && dwType == REG_BINARY;
        }

        // Read string value.
        // IMPORTANT: REG_SZ and REG_EXPAND_SZ are treated the same.
        // Use ExpandEnvironmentVariables() [util.cpp] if desired.
        bool
        Read(
            ConstWString pwszValueName,
            std::wstring& strValue
        )
        const
        {
            DWORD dwType = REG_NONE;
            DWORD dwSize = 0;
            if (!GetValueTypeAndSize(pwszValueName, &dwType, &dwSize))
                return false;
            if (dwType != REG_SZ && dwType != REG_EXPAND_SZ)
                return false;

            // Make sure dwSize is an even number. Rounds down.
            dwSize &= ~0x0001;

            if (dwSize == 0)
            {
                strValue.clear();
            }
            else
            {
                strValue.resize(dwSize / sizeof(wchar_t));

                if (!ReadValue(pwszValueName, &dwType, &strValue[0], &dwSize))
                {
                    strValue.clear();
                    return false;
                }

                // Remove NUL terminator(s) from std::wstring.
                while (!strValue.empty() && strValue.back() == 0)
                    strValue.pop_back();
            }

            return true;
        }

        // Read REG_MULTI_SZ value.
        bool
        Read(
            ConstWString pwszValueName,
            std::vector<std::wstring>& multiStrings
        )
        const
        {
            multiStrings.clear();

            DWORD dwType = REG_NONE;
            DWORD dwSize = 0;
            if (!GetValueTypeAndSize(pwszValueName, &dwType, &dwSize))
                return false;
            if (dwType != REG_MULTI_SZ)
                return false;

            // dwSize is size in bytes, but vector<WCHAR> size is measured in WCHAR's.
            std::vector<WCHAR> buffer(dwSize / sizeof(WCHAR));

            if (!ReadValue(pwszValueName, &dwType, buffer.data(), &dwSize))
                return false;

            std::vector<WCHAR>::const_iterator it = buffer.cbegin();

            while (it != buffer.end() && *it != 0)
            {
                std::vector<WCHAR>::const_iterator start = it;

                while (it != buffer.end() && *it != 0)
                    ++it;

                multiStrings.emplace_back(start, it);

                if (it != buffer.end())
                    ++it;
            }

            return true;
        }

        //
        // Handle REG_DWORD values with all the different integer types.
        //

        // Read REG_DWORD value as a "bool".
        bool
        Read(
            ConstWString pwszValueName,
            bool& Value
        )
        const
        {
            DWORD dwValue;
            if (!Read(pwszValueName, dwValue))
                return false;
            Value = !!dwValue;
            return true;
        }

        // Read REG_DWORD value as a "char".
        bool
        Read(
            ConstWString pwszValueName,
            char& Value
        )
        const
        {
            DWORD dwValue;
            if (!Read(pwszValueName, dwValue))
                return false;
            Value = (char) dwValue;
            return true;
        }

        // Read REG_DWORD value as an "unsigned char".
        bool
        Read(
            ConstWString pwszValueName,
            unsigned char& Value
        )
        const
        {
            DWORD dwValue;
            if (!Read(pwszValueName, dwValue))
                return false;
            Value = (unsigned char) dwValue;
            return true;
        }

        // Read REG_DWORD value as a "short".
        bool
        Read(
            ConstWString pwszValueName,
            short& Value
        )
        const
        {
            DWORD dwValue;
            if (!Read(pwszValueName, dwValue))
                return false;
            Value = (short) dwValue;
            return true;
        }

        // Read REG_DWORD value as an "unsigned short".
        bool
        Read(
            ConstWString pwszValueName,
            unsigned short& Value
        )
        const
        {
            DWORD dwValue;
            if (!Read(pwszValueName, dwValue))
                return false;
            Value = (unsigned short) dwValue;
            return true;
        }

        // Read REG_DWORD value as an "int".
        FORCEINLINE
        bool
        Read(
            ConstWString pwszValueName,
            int& Value
        )
        const
        {
            static_assert(sizeof(DWORD) == sizeof(Value));
            return Read(pwszValueName, *(DWORD*)&Value);
        }

        // Read REG_DWORD value as an "unsigned int".
        FORCEINLINE
        bool
        Read(
            ConstWString pwszValueName,
            unsigned int& Value
        )
        const
        {
            static_assert(sizeof(DWORD) == sizeof(Value));
            return Read(pwszValueName, *(DWORD*)&Value);
        }

        // Read REG_DWORD value as a "long".
        FORCEINLINE
        bool
        Read(
            ConstWString pwszValueName,
            long& Value
        )
        const
        {
            static_assert(sizeof(DWORD) == sizeof(Value));
            return Read(pwszValueName, *(DWORD*)&Value);
        }

        // Read REG_QWORD value as an "INT64".
        FORCEINLINE
        bool
        Read(
            ConstWString pwszValueName,
            INT64& Value
        )
        const
        {
            static_assert(sizeof(UINT64) == sizeof(Value));
            return Read(pwszValueName, *(UINT64*)&Value);
        }

    //------------------------------------------------------------------------
    //
    //  Read registry value.
    //
    //  Separate ReadXXX() functions for each value type.
    //  The function returns the value.
    //
    //  Provided to ease porting of code that uses this style.
    //  Not recommended for new code.
    //
    //------------------------------------------------------------------------

    public:

        // Read REG_DWORD value.
        DWORD
        ReadDWord(
            ConstWString pwszValueName,
            DWORD dwDefaultValue = 0
        )
        const
        {
            DWORD dwValue;
            return Read(pwszValueName, dwValue) ? dwValue : dwDefaultValue;
        }

        // Read REG_QWORD value.
        UINT64
        ReadQWord(
            ConstWString pwszValueName,
            UINT64 qwDefaultValue = 0
        )
        const
        {
            UINT64 qwValue;
            return Read(pwszValueName, qwValue) ? qwValue : qwDefaultValue;
        }

        // Read REG_SZ or REG_EXPAND_SZ value.
        std::wstring
        ReadString(
            ConstWString pwszValueName,
            const std::wstring& strDefaultValue = std::wstring()
        )
        const
        {
            std::wstring strValue;
            return Read(pwszValueName, strValue) ? strValue : strDefaultValue;
        }

        // Read REG_MULTI_SZ value.
        std::vector<std::wstring>
        ReadMultiString(
            ConstWString pwszValueName
        )
        const
        {
            std::vector<std::wstring> multiStrings;
            if (!Read(pwszValueName, multiStrings))
                multiStrings.clear();
            return multiStrings;
        }

    //------------------------------------------------------------------------
    //
    //  Write registry value.
    //
    //  Write() is overloaded by value type, with one exception:
    //  WriteExpandString() is used to write REG_EXPAND_SZ values.
    //  I'm not happy about this, but don't see an elegant solution.
    //  The only difference between Write() and WriteExpandString()
    //  is the REG_XXX type used.  The string written is the same.
    //
    //  You can use std::wstring in place of LPCWSTR, and get a small
    //  optimization by avoiding a wcslen() call.  You can write a
    //  std::wstring containing embedded nulls, but this will confuse
    //  99% of Win32 applications (including RegEdit).  They'll stop
    //  at the first null and effectively truncate the string when
    //  they read it.
    //
    //------------------------------------------------------------------------

    public:

        // Write arbitrary value.
        bool
        WriteValue(
            LPCWSTR pwszValueName,
            DWORD dwType,
            const void* pValue,
            size_t cbValueSize
        )
        const
        {
            return !!m_hKey &&
                   ::RegSetValueExW(m_hKey,
                                    pwszValueName,
                                    0,
                                    dwType,
                                    (LPCBYTE) pValue,
                                    (DWORD) cbValueSize) == ERROR_SUCCESS;
        }

        // I came up with two approaches for implementing Write() for integer
        // types.  The generated code is more-or-less the same, but using
        // inline functions for each type provides better Intellisense.
#if 1

        // Write "DWORD/ULONG" as REG_DWORD value.
        bool
        Write(
            ConstWString pwszValueName,
            DWORD dwValue
        )
        const
        {
            return WriteValue(pwszValueName,
                              REG_DWORD,
                              &dwValue,
                              sizeof(DWORD));
        }

        // Write "UINT64" as REG_QWORD value.
        bool
        Write(
            ConstWString pwszValueName,
            UINT64 qwValue
        )
        const
        {
            return WriteValue(pwszValueName,
                              REG_QWORD,
                              &qwValue,
                              sizeof(UINT64));
        }

        // Handle all the other integer types...

        // Write "bool" as REG_DWORD value.
        bool Write(ConstWString pwszValueName, bool Value) const
            { return Write(pwszValueName, (DWORD)(long) Value); }

        // Write "char" as REG_DWORD value.
        bool Write(ConstWString pwszValueName, char Value) const
            { return Write(pwszValueName, (DWORD)(long) Value); }

        // Write "unsigned char" as REG_DWORD value.
        bool Write(ConstWString pwszValueName, unsigned char Value) const
            { return Write(pwszValueName, (DWORD) Value); }

        // Write "short" as REG_DWORD value.
        bool Write(ConstWString pwszValueName, short Value) const
            { return Write(pwszValueName, (DWORD)(long) Value); }

        // Write "unsigned short" as REG_DWORD value.
        bool Write(ConstWString pwszValueName, unsigned short Value) const
            { return Write(pwszValueName, (DWORD) Value); }

        // Write "int" as REG_DWORD value.
        bool Write(ConstWString pwszValueName, int Value) const
            { return Write(pwszValueName, (DWORD)(long) Value); }

        // Write "unsigned int" as REG_DWORD value.
        bool Write(ConstWString pwszValueName, unsigned int Value) const
            { return Write(pwszValueName, (DWORD) Value); }

        // Write "long" as REG_DWORD value.
        bool Write(ConstWString pwszValueName, long Value) const
            { return Write(pwszValueName, (DWORD) Value); }

        // Write "INT64" as REG_QWORD value.
        bool Write(ConstWString pwszValueName, INT64 Value) const
            { return Write(pwszValueName, (UINT64) Value); }

#else

        struct DWordValue
        {
            DWORD dwValue;

            FORCEINLINE DWordValue(char value)  : dwValue(value) {}
            FORCEINLINE DWordValue(BYTE value)  : dwValue(value) {}
            FORCEINLINE DWordValue(short value) : dwValue(value) {}
            FORCEINLINE DWordValue(WORD value)  : dwValue(value) {}
            FORCEINLINE DWordValue(int value)   : dwValue(value) {}
            FORCEINLINE DWordValue(UINT value)  : dwValue(value) {}
            FORCEINLINE DWordValue(long value)  : dwValue(value) {}
            FORCEINLINE DWordValue(DWORD value) : dwValue(value) {}
        };

        struct QWordValue
        {
            UINT64 qwValue;

            FORCEINLINE QWordValue(INT64 value)  : qwValue(value) {}
            FORCEINLINE QWordValue(UINT64 value) : qwValue(value) {}
        };

        bool Write(ConstWString pwszValueName, DWordValue Value)
        {
            return WriteValue(pwszValueName, REG_DWORD, &Value, sizeof(DWORD));
        }

        bool Write(ConstWString pwszValueName, QWordValue Value)
        {
            return WriteValue(pwszValueName, REG_QWORD, &Value, sizeof(UINT64));
        }

#endif

        // Write REG_BINARY value.
        bool
        Write(
            ConstWString pwszValueName,
            const void* pValue,
            size_t cbValueSize
        )
        const
        {
            return WriteValue(pwszValueName, REG_BINARY, pValue, cbValueSize);
        }

        // Write REG_BINARY value (size specified as int, rather than size_t).
        bool Write(ConstWString pwszValueName, const void* pValue, int cbValueSize) const
            { return Write(pwszValueName, pValue, (size_t) cbValueSize); }

        // Write REG_SZ value (from C string).
        bool
        Write(
            ConstWString pwszValueName,
            LPCWSTR pwszValue
        )
        const
        {
            return WriteValue(pwszValueName,
                              REG_SZ,
                              pwszValue,
                              (wcslen(pwszValue) + 1) * sizeof(WCHAR));
        }

        // Write REG_SZ value (from C++ string).
        bool
        Write(
            ConstWString pwszValueName,
            const std::wstring& strValue
        )
        const
        {
            return WriteValue(pwszValueName,
                              REG_SZ,
                              strValue.c_str(),
                              (strValue.size() + 1) * sizeof(WCHAR));
        }

        // Write REG_EXPAND_SZ value.
        bool
        WriteExpandString(
            ConstWString pwszValueName,
            LPCWSTR pwszValue
        )
        const
        {
            return WriteValue(pwszValueName,
                              REG_EXPAND_SZ,
                              pwszValue,
                              (wcslen(pwszValue) + 1) * sizeof(WCHAR));
        }

        // Write REG_EXPAND_SZ value.
        bool
        WriteExpandString(
            ConstWString pwszValueName,
            std::wstring& strValue
        )
        const
        {
            return WriteValue(pwszValueName,
                              REG_EXPAND_SZ,
                              strValue.c_str(),
                              (strValue.size() + 1) * sizeof(WCHAR));
        }

        // Write REG_MULTI_SZ value.
        bool
        Write(
            ConstWString pwszValueName,
            const std::vector<std::wstring>& multiStrings
        )
        const
        {
            if (!m_hKey)
                return false;

            // Add up the sizes of all the strings in the vector.
            size_t bufferSize = 1;
            for (const auto& str: multiStrings)
                bufferSize += str.size() + 1;

            std::vector<WCHAR> buffer(bufferSize);

            LPWSTR pDst = buffer.data();
            for (const auto& str: multiStrings)
            {
                // REG_MULTI_SZ doesn't allow empty strings!
                ATLASSERT(str.size() != 0);
                wcscpy(pDst, str.c_str());
                pDst += str.size() + 1;
            }
            *pDst = 0;

            return WriteValue(pwszValueName, REG_MULTI_SZ, buffer.data(), buffer.size() * sizeof(WCHAR));
        }

    //------------------------------------------------------------------------
    //
    //  Read and write specialty value types.
    //
    //------------------------------------------------------------------------

    public:

        // Read REG_DWORD value as an enumeration type.
        template <typename ENUMTYPE>
        bool
        ReadEnum(
            ConstWString pwszValueName,
            ENUMTYPE& Value
        )
        const
        {
            DWORD dwValue;
            if (!Read(pwszValueName, dwValue))
                return false;
            Value = (ENUMTYPE) dwValue;
            return true;
        }

        // NOTE: Enumeration types are int's for purposes of Write().

        // Read RECT value.
        bool
        Read(
            ConstWString pwszValueName,
            RECT& rcValue
        )
        const
        {
            std::wstring strValue;
            if (!Read(pwszValueName, strValue))
                return false;
            return swscanf(strValue.c_str(),
                           L"%ld,%ld,%ld,%ld",
                           &rcValue.left,
                           &rcValue.top,
                           &rcValue.right,
                           &rcValue.bottom) == 4;
        }

        // Write RECT value.
        bool
        Write(
            ConstWString pwszValueName,
            RECT rcValue
        )
        const
        {
            WCHAR wszBuffer[128];
            StringCbPrintfW(wszBuffer,
                            sizeof(wszBuffer),
                            L"%ld,%ld,%ld,%ld",
                            rcValue.left,
                            rcValue.top,
                            rcValue.right,
                            rcValue.bottom);
            return Write(pwszValueName, wszBuffer);
        }

        // Read fs::path value.
        bool
        Read(
            ConstWString pwszValueName,
            std::filesystem::path& Value
        )
        const
        {
            std::wstring strValue;
            if (!Read(pwszValueName, strValue))
                return false;
            Value = std::move(strValue);
            return true;
        }

        // Write fs::path value.
        bool
        Write(
            ConstWString pwszValueName,
            const std::filesystem::path& Value
        )
        const
        {
            return Write(pwszValueName, Value.native());
        }

    //------------------------------------------------------------------------
    //
    //  Private functions.
    //
    //------------------------------------------------------------------------

    private:

        // Note that the order of the accessMask and dwOptions parameters
        // is reversed in these functions compared to the underlying API.
        // The reason is that accessMask is much more likely to be used,
        // so it makes sense with C++ default parameter values to have
        // it first.  The underlying C API didn't have this consideration.

        // Create key using RegCreateKeyExW().
        static
        HKEY
        CreateHKEY(
            HKEY hParent,
            LPCWSTR pwszSubKey,
            REGSAM accessMask = KEY_ALL_ACCESS,
            DWORD dwOptions = 0
        )
        {
            HKEY hKey = NULL;
            ::RegCreateKeyExW(hParent, pwszSubKey, 0, nullptr, dwOptions, accessMask, nullptr, &hKey, nullptr);
            return hKey;
        }

        // Open key using RegOpenKeyExW().
        static
        HKEY
        OpenHKEY(
            HKEY hParent,
            LPCWSTR pwszSubKey,
            REGSAM accessMask = MAXIMUM_ALLOWED,
            DWORD dwOptions = 0
        )
        {
            HKEY hKey = NULL;
            ::RegOpenKeyExW(hParent, pwszSubKey, dwOptions, accessMask, &hKey);
            return hKey;
        }

    //------------------------------------------------------------------------
    //
    //  Private data.
    //
    //------------------------------------------------------------------------

    private:

        HKEY m_hKey;
};
