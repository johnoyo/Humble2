#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    /// <summary>
    /// A First-In, First-Out (FIFO) data structure that supports enqueue and dequeue operations in O(1) time.
    /// Used in task scheduling, breadth-first search, and buffering.
    /// </summary>
    /// <typeparam name="T">The type of the element to store in the array.</typeparam>
    /// <typeparam name="TAllocator">The allocator type to use.</typeparam>
    template<typename T, typename TAllocator = StandardAllocator>
    class Queue
    {
    public:
        /// <summary>
        /// Constructs a Queue with an optional initial capacity.
        /// </summary>
        /// <param name="initialCapacity">The starting capacity of the queue.</param>
        Queue(uint32_t initialCapacity = 8)
            : m_CurrentSize(0), m_Capacity(initialCapacity), m_Front(0), m_Back(0)
        {
            m_Allocator = new TAllocator;
            m_Data = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
        }

        /// <summary>
        /// Constructs a Queue with an optional initial capacity and a custom allocator.
        /// </summary>
        /// <param name="allocator">The allocator to use for memory allocation.</param>
        /// <param name="initialCapacity">The starting capacity of the queue.</param>
        Queue(TAllocator* allocator, uint32_t initialCapacity = 8)
            : m_CurrentSize(0), m_Capacity(initialCapacity), m_Front(0), m_Back(0), m_Allocator(allocator)
        {
            m_Data = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
        }

        /// <summary>
        /// Destructor to release allocated memory.
        /// </summary>
        ~Queue()
        {
            m_Allocator->Deallocate<T>(m_Data);
        }

        /// <summary>
        /// Adds a new element at the back of the queue.
        /// </summary>
        /// <param name="value">The element to add.</param>
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

        /// <summary>
        /// Removes the first element in the queue.
        /// </summary>
        void Dequeue()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Queue underflow!");

            m_Front = (m_Front + 1) % m_Capacity; // Move front forward
            --m_CurrentSize;
        }

        /// <summary>
        /// Returns the first element in the queue.
        /// </summary>
        /// <returns>The first element in the queue.</returns>
        T& Front()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Queue is empty!");
            return m_Data[m_Front];
        }

        /// <summary>
        /// Returns the first element in the queue.
        /// </summary>
        /// <returns>The first element in the queue.</returns>
        const T& Front() const
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Queue is empty!");
            return m_Data[m_Front];
        }

        /// <summary>
        /// Returns the number of elements in the queue.
        /// </summary>
        /// <returns>The number of elements in the queue.</returns>
        uint32_t Size() const { return m_CurrentSize; }

        /// <summary>
        /// Check if the queue is empty.
        /// </summary>
        /// <returns>True if the queue is empty, false otherwise.</returns>
        bool IsEmpty() const { return m_CurrentSize == 0; }

        /// <summary>
        /// Clears the entire queue.
        /// </summary>
        void Clear()
        {
            std::memset(m_Data, 0, m_CurrentSize * sizeof(T));
            m_Front = 0;
            m_Back = 0;
            m_CurrentSize = 0;
        }

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

    template<typename T, typename TAllocator>
    auto MakeQueue(TAllocator* allocator, uint32_t initialCapacity = 8)
    {
        return Queue<T, TAllocator>(allocator, initialCapacity);
    }
}