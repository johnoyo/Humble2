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
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(nullptr)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
        }

        /// <summary>
        /// Constructs a DynamicArray with an optional initial capacity and a custom allocator.
        /// </summary>
        /// <param name="allocator">The allocator to use for memory allocation.</param>
        /// <param name="initialCapacity">The starting capacity of the array.</param>
        DynamicArray(TAllocator* allocator, uint32_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(allocator)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
        }

        /// <summary>
        /// Copy constructor which performs deep copy of the array.
        /// </summary>
        DynamicArray(const DynamicArray& other)
            : m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Allocator(other.m_Allocator)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
            std::memcpy(m_Data, other.m_Data, sizeof(T) * m_CurrentSize);
        }

		/// <summary>
		/// Move constructor which transfers ownership of internal data.
		/// </summary>
		DynamicArray(DynamicArray&& other) noexcept
			: m_Data(other.m_Data), m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Allocator(other.m_Allocator)
		{
			other.m_Data = nullptr;
			other.m_Capacity = 0;
			other.m_CurrentSize = 0;
			other.m_Allocator = nullptr;
		}

        /// <summary>
        /// Destructor to release allocated memory.
        /// </summary>
        ~DynamicArray()
        {
            Deallocate(m_Data);
        }

		/// <summary>
		/// Copy assignment operator.
		/// </summary>
		DynamicArray& operator=(const DynamicArray& other)
		{
			if (this == &other)
			{
				return *this;
			}

			// Clean up current data
			Deallocate(m_Data);

			m_Capacity = other.m_Capacity;
			m_CurrentSize = other.m_CurrentSize;
			m_Allocator = other.m_Allocator;

			m_Data = Allocate(sizeof(T) * m_Capacity);
			std::memcpy(m_Data, other.m_Data, sizeof(T) * m_CurrentSize);

			return *this;
		}

		/// <summary>
		/// Move assignment operator.
		/// </summary>
		DynamicArray& operator=(DynamicArray&& other) noexcept
		{
			if (this == &other)
			{
				return *this;
			}

			// Clean up current data
			Deallocate(m_Data);

			m_Data = other.m_Data;
			m_Capacity = other.m_Capacity;
			m_CurrentSize = other.m_CurrentSize;
			m_Allocator = other.m_Allocator;

			other.m_Data = nullptr;
			other.m_Capacity = 0;
			other.m_CurrentSize = 0;
			other.m_Allocator = nullptr;

			return *this;
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
				Reserve(m_Capacity * 2);
            }

            m_Data[m_CurrentSize++] = value;
        }

        /// <summary>
        /// Constructs a new element at the end using perfect forwarding (avoids copy).
        /// </summary>
        /// <typeparam name="...Args">Arguments for T constructor.</typeparam>
        template<typename... Args>
        void Emplace(Args&&... args)
        {
            if (m_CurrentSize == m_Capacity)
            {
                Reserve(m_Capacity * 2);
            }

            // Placement new operator to prevent extra copy.
            new (&m_Data[m_CurrentSize++]) T(std::forward<Args>(args)...);
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
        /// Returns index of first occurrence of value, or UINT32_MAX if not found.
        /// </summary>
        uint32_t FindIndex(const T& value) const
        {
            for (uint32_t i = 0; i < m_CurrentSize; ++i)
            {
                if (m_Data[i] == value)
                {
                    return i;
                }
            }
            return UINT32_MAX;
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

		/// <summary>
        /// Ensures the array can hold at least the given capacity without reallocating.
        /// </summary>
        /// <param name="newCapacity">The minimum capacity to ensure.</param>
        void Reserve(uint32_t newCapacity)
        {
			if (newCapacity <= m_Capacity)
			{
				return;
			}

            T* newData = Allocate(sizeof(T) * newCapacity);
            HBL2_CORE_ASSERT(newData, "DynamicArray::Reserve(), memory allocation failed!");

            std::memcpy(newData, m_Data, m_CurrentSize * sizeof(T));
            Deallocate(m_Data);
            m_Data = newData;

            m_Capacity = newCapacity;
        }

        /// <summary>
        /// Resizes the array to the new size.
        /// If the new size is larger, new elements are default-initialized.
        /// </summary>
        /// <param name="newSize">The new size of the array.</param>
        /// <param name="defaultValue">The default value to assign to new elements (if any).</param>
        void Resize(uint32_t newSize, const T& defaultValue = T{})
        {
            if (newSize > m_Capacity)
            {
                Reserve(newSize);
            }

            if (newSize > m_CurrentSize)
            {
                for (uint32_t i = m_CurrentSize; i < newSize; ++i)
                {
                    m_Data[i] = defaultValue;
                }
            }

            m_CurrentSize = newSize;
        }

        T* begin() { return m_Data; }
        T* end() { return m_Data + m_CurrentSize; }
        const T* begin() const { return m_Data; }
        const T* end() const { return m_Data + m_CurrentSize; }

        T* rbegin() { return m_Data + m_CurrentSize - 1; }
        T* rend() { return m_Data - 1; }
        const T* rbegin() const { return m_Data + m_CurrentSize - 1; }
        const T* rend() const { return m_Data - 1; }

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
				if constexpr (std::is_array_v<T>)
				{
					delete[] ptr;
				}
				else
				{
					delete ptr;
				}

				return;
			}

			m_Allocator->Deallocate<T>(ptr);
		}

    private:
        T* m_Data = nullptr;
        uint32_t m_Capacity = 0; // Not in bytes
        uint32_t m_CurrentSize = 0; // Not in bytes
        TAllocator* m_Allocator = nullptr; // Does not own the pointer
    };

    template<typename T, typename TAllocator>
    auto MakeDynamicArray(TAllocator* allocator, uint32_t initialCapacity = 8)
    {
        return DynamicArray<T, TAllocator>(allocator, initialCapacity);
    }
}
