#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    /// <summary>
    /// An unordered collection of unique elements that provides O(1) average-time complexity for insertions, deletions, and lookups.
    /// Useful for fast membership testing and duplicate elimination.
    /// </summary>
    /// <typeparam name="T">The type of the element to store in the array.</typeparam>
    /// <typeparam name="TAllocator">The allocator type to use.</typeparam>
    template<typename T, typename TAllocator = StandardAllocator>
    class Set
    {
    public:
        /// <summary>
        /// Constructs a Set with an optional initial capacity.
        /// </summary>
        /// <param name="initialCapacity">The starting capacity of the set.</param>
        Set(uint32_t initialCapacity = 16)
            : m_Capacity(initialCapacity), m_CurrentSize(0)
        {
            m_Allocator = new StandardAllocator;
            AllocateMemory(m_Capacity);
        }

        /// <summary>
        /// Constructs a Set with an optional initial capacity and a custom allocator.
        /// </summary>
        /// <param name="allocator">The allocator to use for memory allocation.</param>
        /// <param name="initialCapacity">The starting capacity of the set.</param>
        Set(TAllocator* allocator, uint32_t initialCapacity = 16)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(allocator)
        {
            AllocateMemory(m_Capacity);
        }

        /// <summary>
        /// Destructor to release allocated memory.
        /// </summary>
        ~Set()
        {
            m_Allocator->Deallocate<T>(m_Data);
            m_Allocator->Deallocate<bool>(m_Used);
        }

        /// <summary>
        /// Inserts an element in the set.
        /// </summary>
        /// <param name="value">The element to insert.</param>
        /// <returns>True if inserted, false if already exists.</returns>
        bool Insert(const T& value)
        {
            if (m_CurrentSize >= m_Capacity * 0.75f)
            {
                ReAllocate(m_Capacity * 2);
            }

            uint32_t index = Hash(value);

            while (m_Used[index])
            {
                if (m_Data[index] == value)
                {
                    return false;
                }

                index = (index + 1) % m_Capacity;
            }

            m_Data[index] = value;
            m_Used[index] = true;
            m_CurrentSize++;
            return true;
        }

        /// <summary>
        /// Removes an element from the set.
        /// </summary>
        /// <param name="value">The element to remove</param>
        /// <returns>True if removed, false if not found.</returns>
        bool Erase(const T& value)
        {
            uint32_t index = Hash(value);

            while (m_Used[index])
            {
                if (m_Data[index] == value)
                {
                    m_Used[index] = false;
                    m_CurrentSize--;
                    return true;
                }

                index = (index + 1) % m_Capacity;
            }

            return false; // Not found
        }

        /// <summary>
        /// Check if an element exists in the set.
        /// </summary>
        /// <param name="value">The element to search.</param>
        /// <returns>True if the set contains the element, false if not found.</returns>
        bool Contains(const T& value) const
        {
            uint32_t index = Hash(value);

            while (m_Used[index])
            {
                if (m_Data[index] == value)
                {
                    return true;
                }

                index = (index + 1) % m_Capacity;
            }

            return false;
        }

        /// <summary>
        /// Returns the number of elements in the set.
        /// </summary>
        /// <returns>The number of elements in the set.</returns>
        uint32_t Size() const { return m_CurrentSize; }

        /// <summary>
        /// Clears the entire set.
        /// </summary>
        void Clear()
        {
            std::memset(m_Used, 0, m_CurrentSize * sizeof(bool));
            std::memset(m_Data, 0, m_CurrentSize * sizeof(T));
            m_CurrentSize = 0;
        }

    private:
        void AllocateMemory(uint32_t capacity)
        {
            m_Data = m_Allocator->Allocate<T>(sizeof(T) * capacity);
            m_Used = m_Allocator->Allocate<bool>(sizeof(bool) * capacity);
            memset(m_Used, 0, sizeof(bool) * capacity);
        }

        void ReAllocate(uint32_t newCapacity)
        {
            T* oldData = m_Data;
            bool* oldUsed = m_Used;
            uint32_t oldCapacity = m_Capacity;

            AllocateMemory(newCapacity);
            m_Capacity = newCapacity;
            m_CurrentSize = 0; // Reset size and re-insert elements

            for (uint32_t i = 0; i < oldCapacity; i++)
            {
                if (oldUsed[i])
                {
                    Insert(oldData[i]);
                }
            }

            m_Allocator->Deallocate<T>(oldData);
            m_Allocator->Deallocate<bool>(oldUsed);
        }

        uint32_t Hash(const T& value) const
        {
            return static_cast<uint32_t>(std::hash<T>{}(value) % m_Capacity);
        }

        T* m_Data = nullptr;
        bool* m_Used = nullptr;
        uint32_t m_Capacity;  // Not in bytes
        uint32_t m_CurrentSize; // Not in bytes
        TAllocator* m_Allocator;
    };

    template<typename T, typename TAllocator>
    auto MakeSet(TAllocator* allocator, uint32_t initialCapacity = 8)
    {
        return Set<T, TAllocator>(allocator, initialCapacity);
    }
}