#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    /**
     * @brief A dynamically allocated double-ended queue (deque) that allows fast insertions and deletions from both the front and back.
     *
     * Useful in scenarios requiring fast insertions and deletions at both ends, such as implementing sliding window algorithms, task scheduling, and navigating undo/redo operations.
     *
     * @tparam T The type of the element to store in the deque.
     * @tparam TAllocator The allocator type to use.
     */
    template<typename T, typename TAllocator = StandardAllocator>
    class Deque
    {
    public:
        /**
         * @brief Constructs a Deque with an optional initial capacity.
         *
         * @param initialCapacity The starting capacity of the deque (default 16).
         */
        Deque(uint32_t initialCapacity = 16)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Front(0), m_Back(0), m_Allocator(nullptr)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
        }

        /**
         * @brief Constructs a Deque with an optional initial capacity and a custom allocator.
         *
         * @param allocator The allocator to use for memory allocation.
         * @param initialCapacity The starting capacity of the deque (default 16).
         */
        Deque(TAllocator* allocator, uint32_t initialCapacity = 16)
            : m_CurrentSize(0), m_Capacity(initialCapacity), m_Front(0), m_Back(0), m_Allocator(allocator)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
        }

        /**
         * @brief Copy constructor which performs deep copy of the deque.
         *
         * @param other The Deque to copy from.
         */
        Deque(const Deque& other)
            : m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Front(other.m_Front), m_Back(other.m_Back), m_Allocator(other.m_Allocator)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
            std::memcpy(m_Data, other.m_Data, sizeof(T) * m_Capacity);
        }

        /**
         * @brief Move constructor which transfers ownership of internal data.
         *
         * @param other The Deque to move from.
         */
        Deque(Deque&& other) noexcept
            : m_Data(other.m_Data), m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Front(other.m_Front), m_Back(other.m_Back), m_Allocator(other.m_Allocator)
        {
            other.m_Data = nullptr;
            other.m_Capacity = 0;
            other.m_CurrentSize = 0;
            other.m_Front = 0;
            other.m_Back = 0;
            other.m_Allocator = nullptr;
        }

        /**
         * @brief Destructor to release allocated memory.
         */
        ~Deque()
        {
            Deallocate(m_Data);
        }

        /**
         * @brief Copy assignment operator.
         *
         * @param other The Deque to copy from.
         * @return Reference to this Deque.
         */
        Deque& operator=(const Deque& other)
        {
            if (this == &other)
            {
                return *this;
            }

            Deallocate(m_Data);

            m_Capacity = other.m_Capacity;
            m_CurrentSize = other.m_CurrentSize;
            m_Front = other.m_Front;
            m_Back = other.m_Back;
            m_Allocator = other.m_Allocator;

            m_Data = Allocate(sizeof(T) * m_Capacity);
            std::memcpy(m_Data, other.m_Data, sizeof(T) * m_Capacity);

            return *this;
        }

        /**
         * @brief Move assignment operator.
         *
         * @param other The Deque to move from.
         * @return Reference to this Deque.
         */
        Deque& operator=(Deque&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }

            Deallocate(m_Data);

            m_Data = other.m_Data;
            m_Capacity = other.m_Capacity;
            m_CurrentSize = other.m_CurrentSize;
            m_Front = other.m_Front;
            m_Back = other.m_Back;
            m_Allocator = other.m_Allocator;

            other.m_Data = nullptr;
            other.m_Capacity = 0;
            other.m_CurrentSize = 0;
            other.m_Front = 0;
            other.m_Back = 0;
            other.m_Allocator = nullptr;

            return *this;
        }

        /**
         * @brief Inserts an element at the front of the deque.
         *
         * @param value The value to insert.
         */
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

        /**
         * @brief Inserts an element at the back of the deque.
         *
         * @param value The value to insert.
         */
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

        /**
         * @brief Removes an element from the front of the deque.
         */
        void PopFront()
        {
            if (m_CurrentSize <= 0)
            {
                return;
            }

            m_Front = (m_Front + 1) % m_Capacity;
            m_CurrentSize--;
        }

        /**
         * @brief Removes an element from the back of the deque.
         */
        void PopBack()
        {
            if (m_CurrentSize <= 0)
            {
                return;
            }

            m_Back = (m_Back - 1 + m_Capacity) % m_Capacity;
            m_CurrentSize--;
        }

        /**
         * @brief Gets the front element of the deque.
         *
         * @return Reference to the front element.
         */
        T& Front()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Deque is empty!");
            return m_Data[m_Front];
        }

        /**
         * @brief Gets the back element of the deque.
         *
         * @return Reference to the back element.
         */
        T& Back()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Deque is empty!");
            return m_Data[(m_Back - 1 + m_Capacity) % m_Capacity];
        }

        /**
         * @brief Gets the number of elements in the deque.
         *
         * @return The current size of the deque.
         */
        uint32_t Size() const { return m_CurrentSize; }

        /**
         * @brief Checks if the deque is empty.
         *
         * @return True if the deque is empty, false otherwise.
         */
        bool IsEmpty() const { return m_CurrentSize == 0; }

        /**
         * @brief Clears the entire deque.
         */
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
            ReverseIterator operator++(int) { ReverseIterator temp = *this; ++(*this); return temp; }
            ReverseIterator& operator--() { m_Index = (m_Index + 1) % m_Capacity; return *this; }
            ReverseIterator operator--(int) { ReverseIterator temp = *this;--(*this); return temp; }
            bool operator!=(const ReverseIterator& other) const { return m_Index != other.m_Index; }

        private:
            T* m_Data;
            uint32_t m_Capacity;
            uint32_t m_Index;
        };

        Iterator begin() { return Iterator(m_Data, m_Capacity, m_Front); }
        Iterator end() { return Iterator(m_Data, m_Capacity, m_Back); }

        ReverseIterator rbegin() { return ReverseIterator(m_Data, m_Capacity, (m_Back - 1 + m_Capacity) % m_Capacity); }
        ReverseIterator rend() { return ReverseIterator(m_Data, m_Capacity, (m_Front - 1 + m_Capacity) % m_Capacity); }

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
        void ReAllocate(uint32_t newCapacity)
        {
            T* newData = Allocate(sizeof(T) * newCapacity);
            HBL2_CORE_ASSERT(newData, "Memory allocation failed!");

            for (uint32_t i = 0; i < m_CurrentSize; i++)
            {
                newData[i] = m_Data[(m_Front + i) % m_Capacity];
            }

            Deallocate(m_Data);
            m_Data = newData;

            m_Front = 0;
            m_Back = m_CurrentSize;
            m_Capacity = newCapacity;
        }

        T* m_Data = nullptr;
        uint32_t m_Capacity; // Not in bytes
        uint32_t m_CurrentSize; // Not in bytes
        uint32_t m_Front;
        uint32_t m_Back;
        TAllocator* m_Allocator = nullptr; // Does not own the pointer
    };

    template<typename T, typename TAllocator>
    auto MakeDeque(TAllocator* allocator, uint32_t initialCapacity = 8)
    {
        return Deque<T, TAllocator>(allocator, initialCapacity);
    }
}
