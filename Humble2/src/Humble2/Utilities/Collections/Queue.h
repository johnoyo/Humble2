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
        /**
         * @brief Constructs a Queue with an optional initial capacity.
         *
         * @param initialCapacity The starting capacity of the queue (default 8).
         */
        Queue(uint32_t initialCapacity = 8)
            : m_CurrentSize(0), m_Capacity(initialCapacity), m_Front(0), m_Back(0), m_Allocator(nullptr)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
        }

        /**
         * @brief Constructs a Queue with an optional initial capacity and a custom allocator.
         *
         * @param allocator The allocator to use for memory allocation.
         * @param initialCapacity The starting capacity of the queue (default 8).
         */
        Queue(TAllocator* allocator, uint32_t initialCapacity = 8)
            : m_CurrentSize(0), m_Capacity(initialCapacity), m_Front(0), m_Back(0), m_Allocator(allocator)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
        }

        /**
         * @brief Copy constructor which performs deep copy of the queue.
         *
         * @param other The Queue to copy from.
         */
        Queue(const Queue& other)
            : m_CurrentSize(other.m_CurrentSize),
            m_Capacity(other.m_Capacity),
            m_Front(other.m_Front),
            m_Back(other.m_Back),
            m_Allocator(other.m_Allocator)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
            for (uint32_t i = 0; i < m_Capacity; ++i)
            {
                m_Data[i] = other.m_Data[i];
            }
        }

        /**
         * @brief Move constructor which transfers ownership of internal data.
         *
         * @param other The Queue to move from.
         */
        Queue(Queue&& other) noexcept
            : m_Data(other.m_Data),
            m_CurrentSize(other.m_CurrentSize),
            m_Capacity(other.m_Capacity),
            m_Front(other.m_Front),
            m_Back(other.m_Back),
            m_Allocator(other.m_Allocator)
        {
            other.m_Data = nullptr;
            other.m_CurrentSize = 0;
            other.m_Capacity = 0;
            other.m_Front = 0;
            other.m_Back = 0;
            other.m_Allocator = nullptr;
        }

        /**
         * @brief Destructor to release allocated memory.
         */
        ~Queue()
        {
            Deallocate(m_Data);
        }

        /**
         * @brief Copy assignment operator.
         *
         * @param other The Queue to copy from.
         * @return Reference to this Queue.
         */
        Queue& operator=(const Queue& other)
        {
            if (this == &other)
            {
                return *this;
            }

            Deallocate(m_Data);

            m_CurrentSize = other.m_CurrentSize;
            m_Capacity = other.m_Capacity;
            m_Front = other.m_Front;
            m_Back = other.m_Back;
            m_Allocator = other.m_Allocator;

            m_Data = Allocate(sizeof(T) * m_Capacity);
            for (uint32_t i = 0; i < m_Capacity; ++i)
            {
                m_Data[i] = other.m_Data[i];
            }

            return *this;
        }

        /**
         * @brief Move assignment operator.
         *
         * @param other The Queue to move from.
         * @return Reference to this Queue.
         */
        Queue& operator=(Queue&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }

            Deallocate(m_Data);

            m_Data = other.m_Data;
            m_CurrentSize = other.m_CurrentSize;
            m_Capacity = other.m_Capacity;
            m_Front = other.m_Front;
            m_Back = other.m_Back;
            m_Allocator = other.m_Allocator;

            other.m_Data = nullptr;
            other.m_CurrentSize = 0;
            other.m_Capacity = 0;
            other.m_Front = 0;
            other.m_Back = 0;
            other.m_Allocator = nullptr;

            return *this;
        }

        /**
         * @brief Adds a new element at the back of the queue.
         *
         * @param value The element to add.
         */
        void Enqueue(const T& value)
        {
            if (m_CurrentSize >= m_Capacity)
            {
                T* newData = Allocate(sizeof(T) * m_Capacity * 2);
                HBL2_CORE_ASSERT(newData, "Memory allocation failed!");

                for (uint32_t i = 0; i < m_CurrentSize; i++)
                {
                    newData[i] = m_Data[(m_Front + i) % m_Capacity];
                }

                Deallocate(m_Data);
                m_Data = newData;
                m_Capacity *= 2;
                m_Front = 0;
                m_Back = m_CurrentSize;
            }

            m_Data[m_Back] = value;
            m_Back = (m_Back + 1) % m_Capacity; // Wrap around
            ++m_CurrentSize;
        }

        /**
         * @brief Removes the first element in the queue.
         */
        void Dequeue()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Queue underflow!");

            m_Front = (m_Front + 1) % m_Capacity; // Move front forward
            --m_CurrentSize;
        }

        /**
         * @brief Returns the first element in the queue.
         *
         * @return Reference to the first element.
         */
        T& Front()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Queue is empty!");
            return m_Data[m_Front];
        }

        /**
         * @brief Returns the first element in the queue (const version).
         *
         * @return Const reference to the first element.
         */
        const T& Front() const
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Queue is empty!");
            return m_Data[m_Front];
        }

        /**
         * @brief Returns the number of elements in the queue.
         *
         * @return The number of elements currently stored.
         */
        uint32_t Size() const { return m_CurrentSize; }

        /**
         * @brief Checks if the queue is empty.
         *
         * @return True if the queue is empty, false otherwise.
         */
        bool IsEmpty() const { return m_CurrentSize == 0; }

        /**
         * @brief Clears the entire queue.
         */
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
		T* Allocate(uint64_t size)
		{
			if (m_Allocator == nullptr)
			{
				T* data = (T*)operator new(size);
				memset(data, 0, size);				
				return data;
			}

			return m_Allocator->Allocate<T>(size);
		}

		void Deallocate(T* ptr)
		{
            if (m_Allocator == nullptr)
            {
                operator delete(ptr);
                return;
            }

			m_Allocator->Deallocate<T>(ptr);
		}

    private:
        T* m_Data;
        uint32_t m_CurrentSize; // Not in bytes
        uint32_t m_Capacity; // Not in bytes
        uint32_t m_Front;
        uint32_t m_Back;
        TAllocator* m_Allocator = nullptr; // Does not own the pointer
    };

    template<typename T, typename TAllocator>
    auto MakeQueue(TAllocator* allocator, uint32_t initialCapacity = 8)
    {
        return Queue<T, TAllocator>(allocator, initialCapacity);
    }
}