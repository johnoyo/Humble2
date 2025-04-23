#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    /**
     * @brief An unordered collection of unique elements that provides O(1) average-time complexity for insertions, deletions, and lookups.
     *
     * Useful for fast membership testing and duplicate elimination.
     *
     * @tparam T The type of the element to store in the set.
     * @tparam TAllocator The allocator type to use.
     */
    template<typename T, typename TAllocator = StandardAllocator>
    class Set
    {
    public:
        /**
         * @brief Constructs a Set with an optional initial capacity.
         *
         * @param initialCapacity The starting capacity of the set (default is 16).
         */
        Set(uint32_t initialCapacity = 16)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(nullptr)
        {
            AllocateMemory(m_Capacity);
        }

        /**
         * @brief Constructs a Set with an optional initial capacity and a custom allocator.
         *
         * @param allocator The allocator to use for memory allocation.
         * @param initialCapacity The starting capacity of the set (default is 16).
         */
        Set(TAllocator* allocator, uint32_t initialCapacity = 16)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(allocator)
        {
            AllocateMemory(m_Capacity);
        }

        /**
         * @brief Copy constructor.
         *
         * @param other The Set to copy from.
         */
        Set(const Set& other)
            : m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Allocator(other.m_Allocator)
        {
            AllocateMemory(m_Capacity);
            std::memcpy(m_Data, other.m_Data, sizeof(T) * m_Capacity);
            std::memcpy(m_Used, other.m_Used, sizeof(bool) * m_Capacity);
        }

        /**
         * @brief Move constructor.
         *
         * @param other The Set to move from.
         */
        Set(Set&& other) noexcept
            : m_Data(other.m_Data), m_Used(other.m_Used), m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Allocator(other.m_Allocator)
        {
            other.m_Data = nullptr;
            other.m_Used = nullptr;
            other.m_Capacity = 0;
            other.m_CurrentSize = 0;
            other.m_Allocator = nullptr;
        }

        /**
         * @brief Destructor to release allocated memory.
         */
        ~Set()
        {
            Deallocate<T>(m_Data);
            Deallocate<bool>(m_Used);
        }

        /**
         * @brief Copy assignment operator.
         *
         * @param other The Set to copy from.
         * @return Reference to this Set.
         */
        Set& operator=(const Set& other)
        {
            if (this != &other)
            {
                Deallocate<T>(m_Data);
                Deallocate<bool>(m_Used);

                m_Capacity = other.m_Capacity;
                m_CurrentSize = other.m_CurrentSize;
                m_Allocator = other.m_Allocator;

                AllocateMemory(m_Capacity);
                std::memcpy(m_Data, other.m_Data, sizeof(T) * m_Capacity);
                std::memcpy(m_Used, other.m_Used, sizeof(bool) * m_Capacity);
            }
            return *this;
        }

        /**
         * @brief Move assignment operator.
         *
         * @param other The Set to move from.
         * @return Reference to this Set.
         */
        Set& operator=(Set&& other) noexcept
        {
            if (this != &other)
            {
                Deallocate<T>(m_Data);
                Deallocate<bool>(m_Used);

                m_Data = other.m_Data;
                m_Used = other.m_Used;
                m_Capacity = other.m_Capacity;
                m_CurrentSize = other.m_CurrentSize;
                m_Allocator = other.m_Allocator;

                other.m_Data = nullptr;
                other.m_Used = nullptr;
                other.m_Capacity = 0;
                other.m_CurrentSize = 0;
                other.m_Allocator = nullptr;
            }
            return *this;
        }

        /**
         * @brief Inserts an element in the set.
         *
         * @param value The element to insert.
         * @return True if inserted, false if the element already exists.
         */
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

        /**
         * @brief Removes an element from the set.
         *
         * @param value The element to remove.
         * @return True if removed, false if the element was not found.
         */
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

        /**
         * @brief Checks if an element exists in the set.
         *
         * @param value The element to search for.
         * @return True if the set contains the element, false otherwise.
         */
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

        /**
         * @brief Returns the number of elements in the set.
         *
         * @return The number of elements currently stored in the set.
         */
        uint32_t Size() const { return m_CurrentSize; }

        /**
         * @brief Clears the entire set, removing all elements.
         */
        void Clear()
        {
            std::memset(m_Used, 0, m_CurrentSize * sizeof(bool));
            std::memset(m_Data, 0, m_CurrentSize * sizeof(T));
            m_CurrentSize = 0;
        }

    private:
		template<typename U>
		U* Allocate(uint64_t size)
		{
			if (m_Allocator == nullptr)
			{
				U* data = (U*)operator new(size);
				memset(data, 0, size);				
				return data;
			}

			return m_Allocator->Allocate<U>(size);
		}

		template<typename U>
		void Deallocate(U* ptr)
		{
            if (m_Allocator == nullptr)
            {
                operator delete ptr;
                return;
            }

			m_Allocator->Deallocate<U>(ptr);
		}

    private:
        void AllocateMemory(uint32_t capacity)
        {
            m_Data = Allocate<T>(sizeof(T) * capacity);
            m_Used = Allocate<bool>(sizeof(bool) * capacity);
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

            Deallocate<T>(oldData);
            Deallocate<bool>(oldUsed);
        }

        uint32_t Hash(const T& value) const
        {
            return static_cast<uint32_t>(std::hash<T>{}(value) % m_Capacity);
        }

        T* m_Data = nullptr;
        bool* m_Used = nullptr;
        uint32_t m_Capacity;  // Not in bytes
        uint32_t m_CurrentSize; // Not in bytes
		TAllocator* m_Allocator = nullptr; // Does not own the pointer
    };

    template<typename T, typename TAllocator>
    auto MakeSet(TAllocator* allocator, uint32_t initialCapacity = 8)
    {
        return Set<T, TAllocator>(allocator, initialCapacity);
    }
}