#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    /// <summary>
    /// A resizable array that automatically grows when needed.
    /// It supports fast random access and amortized O(1) insertions at the end, making it suitable for dynamic collections.
    /// </summary>
    /// <typeparam name="T">The type of the element to store in the array.</typeparam>
    /// <typeparam name="TAllocator">The allocator type to use.</typeparam>
    template<typename T, typename TAllocator = StandardAllocator>
    class DynamicArray
    {
    public:
        /// <summary>
        /// Constructs a DynamicArray with an optional initial capacity.
        /// </summary>
        /// <param name="initialCapacity">The starting capacity of the array.</param>
        DynamicArray(size_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0)
        {
            m_Allocator = new TAllocator;
            m_Data = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
        }

        /// <summary>
        /// Constructs a DynamicArray with an optional initial capacity and a custom allocator.
        /// </summary>
        /// <param name="allocator">The allocator to use for memory allocation.</param>
        /// <param name="initialCapacity">The starting capacity of the array.</param>
        DynamicArray(TAllocator* allocator, uint32_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(allocator)
        {
            m_Data = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
        }

        /// <summary>
        /// Destructor to release allocated memory.
        /// </summary>
        ~DynamicArray()
        {
            m_Allocator->Deallocate<T>(m_Data);
        }

        T& operator[](size_t i) { return m_Data[i]; }
        const T& operator[](size_t i) const { return m_Data[i]; }

        /// <summary>
        /// Returns the number of elements in the array.
        /// </summary>
        /// <returns>The number of elements in the array.</returns>
        const uint32_t Size() const { return m_CurrentSize; }

        /// <summary>
        /// Returns the raw pointer to the underlying data.
        /// </summary>
        /// <returns>The raw pointer to the underlying data.</returns>
        const T* Data() const { return m_Data; }

        /// <summary>
        /// Pushes back a new element in the array.
        /// </summary>
        /// <param name="value">The element to add.</param>
        void Add(const T& value)
        {
            if (m_CurrentSize == m_Capacity)
            {
                m_Capacity *= 2;

                T* newData = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
                HBL2_CORE_ASSERT(newData, "Memory allocation failed!");

                std::memcpy(newData, m_Data, m_CurrentSize * sizeof(T));
                m_Allocator->Deallocate<T>(m_Data);
                m_Data = newData;
            }

            m_Data[m_CurrentSize++] = value;
        }

        /// <summary>
        /// Removes the last element from the array.
        /// </summary>
        void Pop()
        {
            if (m_CurrentSize > 0)
            {
                --m_CurrentSize;
            }
        }

        /// <summary>
        /// Check if an element exists in the array.
        /// </summary>
        /// <param name="value">The element to search.</param>
        /// <returns>True if the array contains the element, false if not found.</returns>
        bool Contains(const T& value) const
        {
            for (uint32_t i = 0; i < m_CurrentSize; ++i)
            {
                if (m_Data[i] == value)
                {
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// Erases the provided element from the array.
        /// </summary>
        /// <param name="value">The element to erase.</param>
        void Erase(const T& value)
        {
            uint32_t index = UINT32_MAX;

            for (uint32_t i = 0; i < m_CurrentSize; ++i)
            {
                if (m_Data[i] == value)
                {
                    index = i;
                    break;
                }
            }

            EraseAt(index);
        }

        /// <summary>
        /// Erases an element at the provided index from the array.
        /// </summary>
        /// <param name="index">The index of the element to erase.</param>
        void EraseAt(uint32_t index)
        {
            if (index >= m_CurrentSize)
            {
                HBL2_CORE_ERROR("DynamicArray::EraseAt(), Index out of range!");
                return;
            }

            // Shift elements left
            for (uint32_t i = index; i < m_CurrentSize - 1; ++i)
            {
                m_Data[i] = m_Data[i + 1];
            }
            --m_CurrentSize;
        }

        /// <summary>
        /// Clears the entire array.
        /// </summary>
        void Clear()
        {
            std::memset(m_Data, 0, m_CurrentSize * sizeof(T));
            m_CurrentSize = 0;
        }

        T* begin() { return m_Data; }
        T* end() { return m_Data + m_CurrentSize; }
        const T* begin() const { return m_Data; }
        const T* end() const { return m_Data + m_CurrentSize; }

    private:
        T* m_Data = nullptr;
        uint32_t m_Capacity; // Not in bytes
        uint32_t m_CurrentSize; // Not in bytes
        TAllocator* m_Allocator = nullptr;
    };

    template<typename T, typename TAllocator>
    auto MakeDynamicArray(TAllocator* allocator, uint32_t initialCapacity = 8)
    {
        return DynamicArray<T, TAllocator>(allocator, initialCapacity);
    }
}
