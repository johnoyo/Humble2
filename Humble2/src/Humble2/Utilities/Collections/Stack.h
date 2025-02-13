#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    template<typename T, typename TAllocator = StandardAllocator>
    class Stack
    {
    public:
        Stack(uint32_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0)
        {
            m_Allocator = new TAllocator;
            m_Data = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
        }

        Stack(TAllocator* allocator, uint32_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(allocator)
        {
            m_Data = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
        }

        ~Stack()
        {
            m_Allocator->Deallocate<T>(m_Data);
        }

        void Push(const T& value)
        {
            if (m_CurrentSize >= m_Capacity)
            {
                m_Capacity *= 2;

                T* newData = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
                HBL2_CORE_ASSERT(newData, "Memory allocation failed!");

                memcpy(newData, m_Data, m_CurrentSize * sizeof(T));
                m_Allocator->Deallocate<T>(m_Data);
                m_Data = newData;
            }

            m_Data[m_CurrentSize++] = value;
        }

        void Pop()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Stack underflow!");
            --m_CurrentSize;
        }

        T& Top()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Stack is empty!");
            return m_Data[m_CurrentSize - 1];
        }

        const T& Top() const
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Stack is empty!");
            return m_Data[m_CurrentSize - 1];
        }

        uint32_t Size() const { return m_CurrentSize; }
        bool IsEmpty() const { return m_CurrentSize == 0; }

        T* begin() { return m_Data; }
        T* end() { return m_Data + m_CurrentSize; }
        const T* begin() const { return m_Data; }
        const T* end() const { return m_Data + m_CurrentSize; }

    private:
        T* m_Data;
        uint32_t m_Capacity;
        uint32_t m_CurrentSize;
        TAllocator* m_Allocator = nullptr;
    };
}