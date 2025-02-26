#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    /// <summary>
    /// A key-value data structure that offers O(1) average-time complexity for insertions, deletions, and lookups.
    /// Uses hashing for fast access and collision resolution techniques like linear probing.
    /// </summary>
    /// <typeparam name="TKey">The type of the key.</typeparam>
    /// <typeparam name="TAllocator">The type of the allocator to use.</typeparam>
    /// <typeparam name="TValue">The type of the key.</typeparam>
    template<typename TKey, typename TValue, typename TAllocator = StandardAllocator>
    class HashMap
    {
    private:
        struct Entry
        {
            TKey Key;
            TValue Value;
            bool IsOccupied = false;
            bool IsDeleted = false;
        };

    public:
        /// <summary>
        /// Constructs a HashMap with an optional initial capacity.
        /// </summary>
        /// <param name="initialCapacity">The starting capacity of the map.</param>
        HashMap(uint32_t initialCapacity = 16)
            : m_Capacity(initialCapacity), m_CurrentSize(0)
        {
            m_Allocator = new TAllocator;
            m_Data = m_Allocator->Allocate<Entry>(sizeof(Entry) * m_Capacity);
        }

        /// <summary>
        /// Constructs a HashMap with an optional initial capacity and a custom allocator.
        /// </summary>
        /// <param name="allocator">The allocator to use for memory allocation.</param>
        /// <param name="initialCapacity">The starting capacity of the map.</param>
        HashMap(TAllocator* allocator, uint32_t initialCapacity = 16)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(allocator)
        {
            m_Data = m_Allocator->Allocate<Entry>(sizeof(Entry) * m_Capacity);
        }

        /// <summary>
        /// Destructor to release allocated memory.
        /// </summary>
        ~HashMap()
        {
            m_Allocator->Deallocate<Entry>(m_Data);
        }
                
        /// <summary>
        /// Insert a key-value pair into the hash map.
        /// </summary>
        /// <param name="key">The key to insert the value to.</param>
        /// <param name="value">The value to insert to the key.</param>
        void Insert(const TKey& key, const TValue& value)
        {
            size_t index = Hash(key) % m_Capacity;

            while (m_Data[index].IsOccupied && !m_Data[index].IsDeleted)
            {
                if (m_Data[index].Key == key)
                {
                    m_Data[index].Value = value;  // Update existing key-value pair
                    return;
                }

                index = (index + 1) % m_Capacity;
            }

            if (IsString<TKey>())
            {
                new (&m_Data[index].Key) TKey(key);
            }
            else
            {
                m_Data[index].Key = key;
            }

            if (IsString<TValue>())
            {
                new (&m_Data[index].Value) TValue(value);
            }
            else
            {
                m_Data[index].Value = value;
            }

            m_Data[index].IsOccupied = true;
            m_Data[index].IsDeleted = false;
            ++m_CurrentSize;

            // Reallocate if the load factor exceeds 0.7
            if (m_CurrentSize > m_Capacity * 0.7f)
            {
                ReAllocate();
            }
        }
                
        /// <summary>
        /// Find a value by key.
        /// </summary>
        /// <param name="key">The key to find.</param>
        /// <param name="outValue">The value to emplace the found value.</param>
        /// <returns>True if the key was found, false if not found.</returns>
        bool Find(const TKey& key, TValue& outValue) const
        {
            size_t index = Hash(key) % m_Capacity;

            while (m_Data[index].IsOccupied)
            {
                if (m_Data[index].Key == key && !m_Data[index].IsDeleted)
                {
                    outValue = m_Data[index].Value;
                    return true;
                }

                index = (index + 1) % m_Capacity;
            }

            return false;
        }

        /// <summary>
        /// Find if a key exists in the map.
        /// </summary>
        /// <param name="key">The key to find.</param>
        /// <returns>True if the key was found, false if not found.</returns>
        bool ContainsKey(const TKey& key) const
        {
            size_t index = Hash(key) % m_Capacity;

            while (m_Data[index].IsOccupied)
            {
                if (m_Data[index].Key == key && !m_Data[index].IsDeleted)
                {
                    return true;
                }

                index = (index + 1) % m_Capacity;
            }

            return false;
        }

        /// <summary>
        /// Remove a key-value pair by key.
        /// </summary>
        /// <param name="key">The key to remove.</param>
        void Remove(const TKey& key)
        {
            size_t index = Hash(key) % m_Capacity;

            while (m_Data[index].IsOccupied)
            {
                if (m_Data[index].Key == key && !m_Data[index].IsDeleted)
                {
                    m_Data[index].IsDeleted = true;
                    --m_CurrentSize;
                    return;
                }

                index = (index + 1) % m_Capacity;
            }
        }

        /// <summary>
        /// Get the current size of the hash map.
        /// </summary>
        /// <returns>The current size of the hash map.</returns>
        size_t Size() const { return m_CurrentSize; }

        /// <summary>
        /// Get the capacity of the hash map.
        /// </summary>
        /// <returns>The capacity of the hash map.</returns>
        size_t Capacity() const { return m_Capacity; }

        /// <summary>
        /// Clears the entire queue.
        /// </summary>
        void Clear()
        {
            std::memset(m_Data, 0, m_CurrentSize * sizeof(Entry));
            m_CurrentSize = 0;
        }

        /// <summary>
        /// Returns an iterator to the first valid element in the hash map.
        /// </summary>
        class Iterator
        {
        public:
            Iterator(Entry* data, size_t capacity, size_t index)
                : m_Data(data), m_Capacity(capacity), m_Index(index)
            {
                SkipToValid();
            }

            std::pair<TKey, TValue>& operator*()
            {
                return *reinterpret_cast<std::pair<TKey, TValue>*>(&m_Data[m_Index]);
            }

            Iterator& operator++()
            {
                ++m_Index;
                SkipToValid();
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator temp = *this;
                ++(*this);
                return temp;
            }

            bool operator!=(const Iterator& other) const
            {
                return m_Index != other.m_Index;
            }

        private:
            Entry* m_Data;
            size_t m_Capacity;
            size_t m_Index;

            void SkipToValid()
            {
                while (m_Index < m_Capacity && (!m_Data[m_Index].IsOccupied || m_Data[m_Index].IsDeleted))
                {
                    ++m_Index;
                }
            }
        };

        /// <summary>
        /// Returns an const iterator to the first valid element in the hash map.
        /// </summary>
        class ConstIterator
        {
        public:
            ConstIterator(const Entry* data, size_t capacity, size_t index)
                : m_Data(data), m_Capacity(capacity), m_Index(index)
            {
                SkipToValid();
            }

            const std::pair<TKey, TValue>& operator*() const
            {
                return *reinterpret_cast<const std::pair<TKey, TValue>*>(&m_Data[m_Index]);
            }

            ConstIterator& operator++()
            {
                ++m_Index;
                SkipToValid();
                return *this;
            }

            ConstIterator operator++(int)
            {
                ConstIterator temp = *this;
                ++(*this);
                return temp;
            }

            bool operator!=(const ConstIterator& other) const
            {
                return m_Index != other.m_Index;
            }

        private:
            const Entry* m_Data;
            size_t m_Capacity;
            size_t m_Index;

            void SkipToValid()
            {
                while (m_Index < m_Capacity && (!m_Data[m_Index].IsOccupied || m_Data[m_Index].IsDeleted))
                {
                    ++m_Index;
                }
            }
        };

        /// <summary>
        /// Returns an iterator to the first valid element in the hash map.
        /// </summary>
        Iterator begin() { return Iterator(m_Data, m_Capacity, 0); }

        /// <summary>
        /// Returns an iterator representing the end of the hash map.
        /// </summary>
        Iterator end() { return Iterator(m_Data, m_Capacity, m_Capacity); }

        /// <summary>
        /// Returns a const iterator to the first valid element in the hash map.
        /// </summary>
        ConstIterator begin() const { return ConstIterator(m_Data, m_Capacity, 0); }

        /// <summary>
        /// Returns a const iterator representing the end of the hash map.
        /// </summary>
        ConstIterator end() const { return ConstIterator(m_Data, m_Capacity, m_Capacity); }

        /// <summary>
        /// Gets or inserts a value in the given key.
        /// </summary>
        /// <param name="key">The key to get or set.</param>
        /// <returns>The value of the key or default.</returns>
        TValue& operator[](const TKey& key)
        {
            size_t index = Hash(key) % m_Capacity;

            while (m_Data[index].IsOccupied)
            {
                if (m_Data[index].Key == key)
                {
                    return m_Data[index].Value;
                }

                index = (index + 1) % m_Capacity;
            }

            if (IsString<TKey>())
            {
                new (&m_Data[index].Key) TKey(key);
            }
            else
            {
                m_Data[index].Key = key;
            }

            if (IsString<TValue>())
            {
                new (&m_Data[index].Value) TValue(TValue());
            }
            else
            {
                m_Data[index].Value = TValue();
            }

            m_Data[index].IsOccupied = true;
            m_Data[index].IsDeleted = false;
            ++m_CurrentSize;

            if (m_CurrentSize > m_Capacity * 0.7f)
            {
                ReAllocate();
            }

            index = Hash(key) % m_Capacity;

            return m_Data[index].Value;
        }
        
    private:
        size_t Hash(const TKey& key) const
        {
            return std::hash<TKey>()(key);
        }

        void ReAllocate()
        {
            size_t newCapacity = m_Capacity * 2;
            Entry* newData = m_Allocator->Allocate<Entry>(sizeof(Entry) * newCapacity);
            HBL2_CORE_ASSERT(newData, "Memory allocation failed!");

            // Rehash all entries
            for (size_t i = 0; i < m_Capacity; ++i)
            {
                if (m_Data[i].IsOccupied && !m_Data[i].IsDeleted)
                {
                    size_t index = Hash(m_Data[i].Key) % newCapacity;
                    while (newData[index].IsOccupied)
                    {
                        index = (index + 1) % newCapacity;
                    }

                    if (IsString<TKey>())
                    {
                        new (&newData[index].Key) TKey(m_Data[i].Key);
                    }
                    else
                    {
                        newData[index].Key = m_Data[i].Key;
                    }

                    if (IsString<TValue>())
                    {
                        new (&newData[index].Value) TValue(m_Data[i].Value);
                    }
                    else
                    {
                        newData[index].Value = m_Data[i].Value;
                    }

                    newData[index].IsOccupied = m_Data[i].IsOccupied;
                    newData[index].IsDeleted = m_Data[i].IsDeleted;
                }
            }

            m_Allocator->Deallocate<Entry>(m_Data);
            m_Data = newData;
            m_Capacity = newCapacity;
        }

        template<typename T>
        bool IsString()
        {
            return std::is_same<T, std::string>::value;
        }

    private:
        Entry* m_Data;
        uint32_t m_Capacity; // Not in bytes
        uint32_t m_CurrentSize; // Not in bytes
        TAllocator* m_Allocator;
    };
}