#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    template<typename T, typename TAllocator = StandardAllocator>
    class DynamicArray
    {
    public:
        DynamicArray(size_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0)
        {
            m_Allocator = new TAllocator;
            m_Data = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
        }

        DynamicArray(TAllocator* allocator, uint32_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(allocator)
        {
            m_Data = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
        }

        ~DynamicArray()
        {
            m_Allocator->Deallocate<T>(m_Data);
        }

        T& operator[](size_t i) { return m_Data[i]; }
        const T& operator[](size_t i) const { return m_Data[i]; }

        const uint32_t Size() const { return m_CurrentSize; }
        const T* Data() const { return m_Data; }

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

        void Pop()
        {
            if (m_CurrentSize > 0)
            {
                --m_CurrentSize;
            }
        }

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

        void Erase(uint32_t index)
        {
            if (index >= m_CurrentSize)
            {
                HBL2_CORE_ERROR("DynamicArray::Erase -> Index out of range!");
            }

            // Shift elements left
            for (uint32_t i = index; i < m_CurrentSize - 1; ++i)
            {
                m_Data[i] = m_Data[i + 1];
            }
            --m_CurrentSize;
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
}