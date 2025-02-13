#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    template<typename T, typename TAllocator = StandardAllocator>
    class Queue
    {
    public:
        Queue(uint32_t initialCapacity = 8)
            : m_CurrentSize(0), m_Capacity(initialCapacity), m_Front(0), m_Back(0)
        {
            m_Allocator = new TAllocator;
            m_Data = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
        }

        Queue(TAllocator* allocator, uint32_t initialCapacity = 8)
            : m_CurrentSize(0), m_Capacity(initialCapacity), m_Front(0), m_Back(0), m_Allocator(allocator)
        {
            m_Data = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
        }

        ~Queue()
        {
            m_Allocator->Deallocate<T>(m_Data);
        }

        void Enqueue(const T& value)
        {
            if (m_CurrentSize >= m_Capacity)
            {
                T* newData = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity * 2);
                HBL2_CORE_ASSERT(newData, "Memory allocation failed!");

                for (uint32_t i = 0; i < m_CurrentSize; i++)
                {
                    newData[i] = m_Data[(m_Front + i) % m_Capacity];
                }

                m_Allocator->Deallocate<T>(m_Data);
                m_Data = newData;
                m_Capacity *= 2;
                m_Front = 0;
                m_Back = m_CurrentSize;
            }

            m_Data[m_Back] = value;
            m_Back = (m_Back + 1) % m_Capacity; // Wrap around
            ++m_CurrentSize;
        }

        void Dequeue()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Queue underflow!");

            m_Front = (m_Front + 1) % m_Capacity; // Move front forward
            --m_CurrentSize;
        }

        T& Front()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Queue is empty!");
            return m_Data[m_Front];
        }

        const T& Front() const
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Queue is empty!");
            return m_Data[m_Front];
        }

        uint32_t Size() const { return m_CurrentSize; }
        bool IsEmpty() const { return m_CurrentSize == 0; }

        T* begin() { return &m_Data[m_Front]; }
        T* end() { return &m_Data[m_Back]; }
        const T* begin() const { return &m_Data[m_Front]; }
        const T* end() const { return &m_Data[m_Back]; }

    private:
        T* m_Data;
        uint32_t m_CurrentSize;
        uint32_t m_Capacity;
        uint32_t m_Front;
        uint32_t m_Back;
        TAllocator* m_Allocator = nullptr;
    };
}