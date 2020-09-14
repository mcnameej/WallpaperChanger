//////////////////////////////////////////////////////////////////////////////
//
//  HotKey.h
//
//  CHotKeyDefinition class.
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

// Hotkey definition (VK and modifier flags).
class CHotKeyDefinition
{
    public:

        UINT m_VirtKey;     // Virtual Key code.
        UINT m_Modifiers;   // Modifier flags (MOD_CONTROL, MOD_ALT, etc.).
        bool m_Enabled;     // Enabled flag.

    public:

        // Default ctor.
        CHotKeyDefinition()
            :
            m_VirtKey(0),
            m_Modifiers(0),
            m_Enabled(false)
        {
        }

        // Construct from VK and modifier flags.
        CHotKeyDefinition(
            UINT virtKey,
            UINT modifiers,
            bool enabled = true
        )
            :
            m_VirtKey(virtKey),
            m_Modifiers(modifiers),
            m_Enabled(enabled)
        {
        }

        // Construct from definition string.
        CHotKeyDefinition(
            const std::wstring& strDefinition
        )
            :
            m_VirtKey(0),
            m_Modifiers(0),
            m_Enabled(false)
        {
            Parse(strDefinition);
        }

        // Clear hotkey definiton.
        void
        clear()
        {
            m_VirtKey = 0;
            m_Modifiers = 0;
            m_Enabled = false;
        }

        // Is hotkey definition valid?
        bool
        IsValid()
        {
            // Must be enabled, have a VK, and have at least one modifier.
            return m_Enabled && m_VirtKey != 0 && m_Modifiers != 0;
        }

        // Register hotkey with Windows.
        bool
        Register(
            HWND hWnd,
            int id
        )
        {
            ::UnregisterHotKey(hWnd, id);

            if (!IsValid())
                return false;

            return ::RegisterHotKey(hWnd, id, m_Modifiers, m_VirtKey);
        }

        // Parse hotkey definition string.
        // No change to current definition if parse fails.
        bool
        Parse(
            const std::wstring& strDefinition
        )
        {
            UINT virtKey, modifiers;
            bool enabled;

            if (!ParseHotKeyString(strDefinition, virtKey, modifiers, enabled))
                return false;

            m_VirtKey = virtKey;
            m_Modifiers = modifiers;
            m_Enabled = enabled;

            return true;
        }

        // Create hotkey definition string.
        std::wstring
        ToString()
        {
            std::wstring strHotKey;

            if (!m_Enabled)
                strHotKey += L"Disabled+";

            UINT modifiers = m_Modifiers;

            if (modifiers & MOD_CONTROL)
            {
                strHotKey += L"Ctrl+";
                modifiers &= ~MOD_CONTROL;
            }
            if (modifiers & MOD_SHIFT)
            {
                strHotKey += L"Shift+";
                modifiers &= ~MOD_SHIFT;
            }
            if (modifiers & MOD_ALT)
            {
                strHotKey += L"Alt+";
                modifiers &= ~MOD_ALT;
            }
            if (modifiers & MOD_WIN)
            {
                strHotKey += L"Win+";
                modifiers &= ~MOD_WIN;
            }

            if (modifiers != 0)
                DebugPrint("CHotKeyDefinition::ToString: Invalid modifier bits: 0x%04X\n", modifiers);

            if (m_VirtKey != 0xFFFF)
            {
                if ((m_VirtKey >= '0' && m_VirtKey <= '9') || (m_VirtKey >= 'A' && m_VirtKey <= 'Z'))
                {
                    strHotKey += (WCHAR) m_VirtKey;
                }
                else if (m_VirtKey >= VK_F1 && m_VirtKey <= VK_F24)
                {
                    strHotKey += L'F';
                    strHotKey += std::to_wstring((m_VirtKey - VK_F1) + 1);
                }
                else
                {
                    DebugPrint("CHotKeyDefinition::ToString: Invalid VK: 0x%02X\n", m_VirtKey);
                    strHotKey += L"INVALID!";
                }
            }
            else
            {
                // Remove trailing plus.
                if (strHotKey.back() == L'+')
                    strHotKey.pop_back();
            }

            return strHotKey;
        }

    private:

        static
        bool
        IsHotKeyModifier(
            const std::wstring& str,
            size_t& pos,
            LPCWSTR pwszModifierName,
            size_t nameLen
        )
        {
            if (pos + nameLen < str.size() &&
                _wcsnicmp(&str[pos], pwszModifierName, nameLen) == 0 &&
                (str[pos+nameLen] == L'-' || str[pos+nameLen] == L'+'))
            {
                pos += nameLen + 1;
                return true;
            }

            return false;
        }

        static
        bool
        ParseHotKeyString(
            const std::wstring& strHotKey,
            UINT& vk,
            UINT& modifiers,
            bool& enabled
        )
        {
            vk = 0;
            modifiers = 0;
            enabled = true;

            size_t pos = 0;

            while (pos < strHotKey.size())
            {
                if (IsHotKeyModifier(strHotKey, pos, L"DISABLED", 8))
                {
                    enabled = false;
                }
                if (IsHotKeyModifier(strHotKey, pos, L"SHIFT", 5))
                {
                    modifiers |= MOD_SHIFT;
                }
                else if (IsHotKeyModifier(strHotKey, pos, L"CTRL", 4))
                {
                    modifiers |= MOD_CONTROL;
                }
                else if (IsHotKeyModifier(strHotKey, pos, L"ALT", 3))
                {
                    modifiers |= MOD_ALT;
                }
                else if (IsHotKeyModifier(strHotKey, pos, L"WIN", 3))
                {
                    modifiers |= MOD_WIN;
                }
                else
                {
                    WCHAR ch = strHotKey[pos];
                    if (pos + 1 == strHotKey.size())
                    {
                        if (iswalpha(ch))
                        {
                            // Letter.
                            vk = towupper(ch);
                            pos++;
                        }
                        else if (iswdigit(ch))
                        {
                            // Digit.
                            vk = ch;
                            pos++;
                        }
                    }
                    else if (ch == L'F' || ch == L'f')
                    {
                        // Function key.
                        LPWSTR pwszEndOfNumber = nullptr;
                        UINT funcKey = (UINT) ::wcstoul(&strHotKey[pos+1], &pwszEndOfNumber, 10);
                        if (pwszEndOfNumber != &strHotKey[pos+1] &&
                            funcKey >= 1 &&
                            funcKey <= 24)
                        {
                            vk = VK_F1 + (funcKey - 1);
                            pos = pwszEndOfNumber - strHotKey.data();
                        }
                    }
                    break;
                }
            }

            return (pos == strHotKey.size()) &&  // We made it to end of string.
                   (vk != 0) &&                  // We got a virtual key code.
                   (modifiers != 0);             // We got modifier flag(s).
        }
};
