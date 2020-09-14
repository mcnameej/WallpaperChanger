//////////////////////////////////////////////////////////////////////////////
//
//  ImageListCache.h
//
//  CImageListCache class template.
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

template <size_t CacheSize>
class CImageListCache
{
    public:

        CImageListCache()
            :
            m_Cache()
        {
        }

        void
        Clear()
        {
            for (CacheEntry& entry: m_Cache)
            {
                entry.m_Key = nullptr;
            }
        }

        bool
        Lookup(
            void* key,
            int** ppImageListIndex
        )
        {
            using std::begin;
            using std::end;

            auto it = std::find_if(begin(m_Cache),
                                   end(m_Cache),
                                   [key] (CacheEntry& entry) { return entry.m_Key == key; });

            if (it == begin(m_Cache))
            {
                // Found existing entry in cache.
                // Already at top of list.
                *ppImageListIndex = &it->m_ImageListIndex;
                return true;
            }
            else if (it != end(m_Cache))
            {
                // Found existing entry in cache.
                // Move to top of list (most recently used).
                // "That's a rotate!"
                std::rotate(begin(m_Cache), it, it+1);
                *ppImageListIndex = &m_Cache[0].m_ImageListIndex;
                return true;
            }
            else
            {
                // Not in the cache.
                // Replace the oldest entry.
                std::rotate(begin(m_Cache), end(m_Cache)-1, end(m_Cache));
                m_Cache[0].m_Key = key;
                *ppImageListIndex = &m_Cache[0].m_ImageListIndex;
                return false;
            }
        }

    private:

        struct CacheEntry
        {
            CacheEntry()
                :
                m_Key(nullptr),
                m_ImageListIndex(-1)
            {
            }

            void* m_Key;
            int m_ImageListIndex;
        };

        CacheEntry m_Cache[CacheSize];
};
