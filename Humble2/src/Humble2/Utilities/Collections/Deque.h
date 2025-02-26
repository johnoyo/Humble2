#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    /// <summary>
    /// A dynamically allocated double-ended queue (deque) that allows fast insertions and deletions from both the front and back.
    /// Useful in scenarios requiring fast insertions and deletions at both ends, such as implementing sliding window algorithms, task scheduling, and navigating undo/redo operations.
    /// </summary>
    /// <typeparam name="T">The type of elements in the deque.</typeparam>
    /// <typeparam name="TAllocator">The allocator type to use.</typeparam>
    template<typename T, typename TAllocator = StandardAllocator>
    class Deque
    {
    public:
        /// <summary>
        /// Constructs a Deque with an optional initial capacity.
        /// </summary>
        /// <param name="initialCapacity">The starting capacity of the deque.</param>
        Deque(uint32_t initialCapacity = 16)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Front(0), m_Back(0)
        {
            m_Allocator = new TAllocator;
            m_Data = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
        }

        /// <summary>
        /// Constructs a Deque with an optional initial capacity and a custom allocator.
        /// </summary>
        /// <param name="allocator">The allocator to use for memory allocation.</param>
        /// <param name="initialCapacity">The starting capacity of the deque.</param>
        Deque(TAllocator* allocator, uint32_t initialCapacity = 16)
            : m_CurrentSize(0), m_Capacity(initialCapacity), m_Front(0), m_Back(0), m_Allocator(allocator)
        {
            m_Data = m_Allocator->Allocate<T>(sizeof(T) * m_Capacity);
        }

        /// <summary>
        /// Destructor to release allocated memory.
        /// </summary>
        ~Deque()
        {
            m_Allocator->Deallocate<T>(m_Data);
        }

        /// <summary>
        /// Inserts an element at the front of the deque.
        /// </summary>
        /// <param name="value">The value to insert.</param>
        void PushFront(const T& value)
        {
            if (m_CurrentSize == m_Capacity)
            {
                ReAllocate(m_Capacity * 2);
            }

            m_Front = (m_Front - 1 + m_Capacity) % m_Capacity;
            m_Data[m_Front] = value;
            m_CurrentSize++;
        }

        /// <summary>
        /// Inserts an element at the back of the deque.
        /// </summary>
        /// <param name="value">The value to insert.</param>
        void PushBack(const T& value)
        {
            if (m_CurrentSize == m_Capacity)
            {
                ReAllocate(m_Capacity * 2);
            }

            m_Data[m_Back] = value;
            m_Back = (m_Back + 1) % m_Capacity;
            m_CurrentSize++;
        }

        /// <summary>
        /// Removes an element from the front of the deque.
        /// </summary>
        void PopFront()
        {
            if (m_CurrentSize <= 0)
            {
                return;
            }

            m_Front = (m_Front + 1) % m_Capacity;
            m_CurrentSize--;
        }

        /// <summary>
        /// Removes an element from the back of the deque.
        /// </summary>
        void PopBack()
        {
            if (m_CurrentSize <= 0)
            {
                return;
            }

            m_Back = (m_Back - 1 + m_Capacity) % m_Capacity;
            m_CurrentSize--;
        }

        /// <summary>
        /// Gets the front element of the deque.
        /// </summary>
        /// <returns>Reference to the front element.</returns>
        T& Front()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Deque is empty!");
            return m_Data[m_Front];
        }

        /// <summary>
        /// Gets the back element of the deque.
        /// </summary>
        /// <returns>Reference to the back element.</returns>
        T& Back()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Deque is empty!");
            return m_Data[(m_Back - 1 + m_Capacity) % m_Capacity];
        }

        /// <summary>
        /// Gets the number of elements in the deque.
        /// </summary>
        /// <returns>The current size of the deque.</returns>
        uint32_t Size() const { return m_CurrentSize; }

        /// <summary>
        /// Checks if the deque is empty.
        /// </summary>
        /// <returns>True if the deque is empty, false otherwise.</returns>
        bool IsEmpty() const { return m_CurrentSize == 0; }

        /// <summary>
        /// Clears the entire deqeue.
        /// </summary>
        void Clear()
        {
            std::memset(m_Data, 0, m_CurrentSize * sizeof(T));
            m_Front = 0;
            m_Back = 0;
            m_CurrentSize = 0;
        }

        class Iterator
        {
        public:
            Iterator(T* data, uint32_t capacity, uint32_t index)
                : m_Data(data), m_Capacity(capacity), m_Index(index) { }

            T& operator*() { return m_Data[m_Index]; }
            Iterator& operator++() { m_Index = (m_Index + 1) % m_Capacity; return *this; }
            bool operator!=(const Iterator& other) const { return m_Index != other.m_Index; }

        private:
            T* m_Data;
            uint32_t m_Capacity;
            uint32_t m_Index;
        };

        class ReverseIterator
        {
        public:
            ReverseIterator(T* data, uint32_t capacity, uint32_t index)
                : m_Data(data), m_Capacity(capacity), m_Index(index) { }

            T& operator*() { return m_Data[m_Index]; }
            ReverseIterator& operator++() { m_Index = (m_Index - 1 + m_Capacity) % m_Capacity; return *this; }
            bool operator!=(const ReverseIterator& other) const { return m_Index != other.m_Index; }

        private:
            T* m_Data;
            uint32_t m_Capacity;
            uint32_t m_Index;
        };

        /// <summary>
        /// Returns an iterator to the beginning of the deque.
        /// </summary>
        /// <returns>An iterator to the first element.</returns>
        Iterator begin() { return Iterator(m_Data, m_Capacity, m_Front); }

        /// <summary>
        /// Returns an iterator to the end of the deque.
        /// </summary>
        /// <returns>An iterator to one past the last element.</returns>
        Iterator end() { return Iterator(m_Data, m_Capacity, m_Back); }

        /// <summary>
        /// Returns a reverse iterator to the end of the deque.
        /// </summary>
        /// <returns>A reverse iterator to the last element.</returns>
        ReverseIterator rbegin() { return ReverseIterator(m_Data, m_Capacity, (m_Back - 1 + m_Capacity) % m_Capacity); }

        /// <summary>
        /// Returns a reverse iterator to the beginning of the deque.
        /// </summary>
        /// <returns>A reverse iterator to one before the first element.</returns>
        ReverseIterator rend() { return ReverseIterator(m_Data, m_Capacity, (m_Front - 1 + m_Capacity) % m_Capacity); }

    private:
        void ReAllocate(uint32_t newCapacity)
        {
            T* newData = m_Allocator->Allocate<T>(sizeof(T) * newCapacity);
            HBL2_CORE_ASSERT(newData, "Memory allocation failed!");

            for (uint32_t i = 0; i < m_CurrentSize; i++)
            {
                newData[i] = m_Data[(m_Front + i) % m_Capacity];
            }

            m_Allocator->Deallocate<T>(m_Data);
            m_Data = newData;

            m_Front = 0;
            m_Back = m_CurrentSize;
            m_Capacity = newCapacity;
        }

        T* m_Data = nullptr;
        uint32_t m_Capacity;
        uint32_t m_CurrentSize;
        uint32_t m_Front;
        uint32_t m_Back;
        TAllocator* m_Allocator;
    };
}
