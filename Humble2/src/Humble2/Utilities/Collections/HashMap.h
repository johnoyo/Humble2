#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    template<typename TKey, typename TValue, typename TAllocator = StandardAllocator>
    class HashMap
    {
    public:
        HashMap(uint32_t initialCapacity = 16)
            : m_Capacity(initialCapacity), m_CurrentSize(0)
        {
            m_Allocator = new TAllocator;
            m_Entries = m_Allocator->Allocate<Entry>(sizeof(Entry) * m_Capacity);
        }

        HashMap(TAllocator* allocator, uint32_t initialCapacity = 16)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(allocator)
        {
            m_Entries = m_Allocator->Allocate<Entry>(sizeof(Entry) * m_Capacity);
        }

        ~HashMap()
        {
            m_Allocator->Deallocate<Entry>(m_Entries);
        }

        void Insert(const TKey& key, const TValue& value)
        {
            if (m_CurrentSize >= m_Capacity / 2)
            {
                Entry* oldEntries = m_Entries;
                uint32_t oldCapacity = m_Capacity;

                m_Capacity = m_Capacity * 2;
                m_Entries = m_Allocator->Allocate<Entry>(sizeof(Entry) * m_Capacity);
                HBL2_CORE_ASSERT(m_Entries, "Memory allocation failed!");

                m_CurrentSize = 0;
                for (uint32_t i = 0; i < oldCapacity; i++)
                {
                    if (oldEntries[i].occupied && !oldEntries[i].deleted)
                    {
                        Insert(oldEntries[i].key, oldEntries[i].value);
                    }
                }

                m_Allocator->Deallocate<Entry>(oldEntries);
            }

            uint32_t index = Hash(key) % m_Capacity;

            while (m_Entries[index].occupied) // Linear probing if collision
            {
                if (m_Entries[index].key == key) // Update existing key
                {
                    m_Entries[index].value = value;
                    return;
                }

                index = (index + 1) % m_Capacity;
            }

            m_Entries[index] = { key, value, true };
            ++m_CurrentSize;
        }

        bool Remove(const TKey& key)
        {
            uint32_t index = Hash(key) % m_Capacity;

            while (m_Entries[index].occupied)
            {
                if (m_Entries[index].key == key)
                {
                    m_Entries[index].deleted = true;
                    --m_CurrentSize;
                    return true;
                }

                index = (index + 1) % m_Capacity;
            }

            return false; // Key not found
        }

        TValue* Find(const TKey& key)
        {
            uint32_t index = Hash(key) % m_Capacity;

            while (m_Entries[index].occupied)
            {
                if (m_Entries[index].key == key)
                {
                    return &m_Entries[index].value;
                }

                index = (index + 1) % m_Capacity;
            }

            return nullptr; // Key not found
        }

        uint32_t Size() const { return m_CurrentSize; }
        bool IsEmpty() const { return m_CurrentSize == 0; }

        // TODO: Add iterator support
        
    private:
        struct Entry
        {
            TKey key;
            TValue value;
            bool occupied;
            bool deleted;
        };

    private:
        uint32_t Hash(const TKey& key) const
        {
            return static_cast<uint32_t>(std::hash<TKey>{}(key));
        }

    private:
        Entry* m_Entries;
        uint32_t m_Capacity;
        uint32_t m_CurrentSize;
        TAllocator* m_Allocator;
    };
}