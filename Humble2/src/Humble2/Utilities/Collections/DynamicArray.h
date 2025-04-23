#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    /**
     * @brief A resizable array that automatically grows when needed.
     *
     * It supports fast random access and amortized O(1) insertions at the end, making it suitable for dynamic collections.
     *
     * @tparam T The type of the element to store in the array.
     * @tparam TAllocator The allocator type to use.
     */
    template<typename T, typename TAllocator = StandardAllocator>
    class DynamicArray
    {
    public:
        /**
         * @brief Constructs a DynamicArray with an optional initial capacity.
         *
         * @param initialCapacity The starting capacity of the array (default 8).
         */
        DynamicArray(size_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(nullptr)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
        }

        /**
         * @brief Constructs a DynamicArray with an optional initial capacity and a custom allocator.
         *
         * @param allocator The allocator to use for memory allocation.
         * @param initialCapacity The starting capacity of the array (default 8).
         */
        DynamicArray(TAllocator* allocator, uint32_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(allocator)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
        }

        /**
         * @brief Copy constructor which performs deep copy of the array.
         *
         * @param other The DynamicArray to copy from.
         */
        DynamicArray(const DynamicArray& other)
            : m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Allocator(other.m_Allocator)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
            std::memcpy(m_Data, other.m_Data, sizeof(T) * m_CurrentSize);
        }

        /**
         * @brief Move constructor which transfers ownership of internal data.
         *
         * @param other The DynamicArray to move from.
         */
		DynamicArray(DynamicArray&& other) noexcept
			: m_Data(other.m_Data), m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Allocator(other.m_Allocator)
		{
			other.m_Data = nullptr;
			other.m_Capacity = 0;
			other.m_CurrentSize = 0;
			other.m_Allocator = nullptr;
		}

        /**
         * @brief Destructor to release allocated memory.
         */
        ~DynamicArray()
        {
            Deallocate(m_Data);
        }

        /**
         * @brief Copy assignment operator.
         *
         * @param other The DynamicArray to copy from.
         * @return Reference to this DynamicArray.
         */
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

        /**
         * @brief Move assignment operator.
         *
         * @param other The DynamicArray to move from.
         * @return Reference to this DynamicArray.
         */
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

        /**
         * @brief Access element at specified index.
         *
         * @param i Index of the element.
         * @return Reference to the element at index i.
         */
        T& operator[](size_t i) { return m_Data[i]; }

        /**
         * @brief Access element at specified index (const version).
         *
         * @param i Index of the element.
         * @return Const reference to the element at index i.
         */
        const T& operator[](size_t i) const { return m_Data[i]; }

        /**
         * @brief Returns the number of elements in the array.
         *
         * @return The number of elements currently stored.
         */
        const uint32_t Size() const { return m_CurrentSize; }

        /**
         * @brief Returns the raw pointer to the underlying data.
         *
         * @return Pointer to the array data.
         */
        const T* Data() const { return m_Data; }

        /**
         * @brief Pushes back a new element in the array.
         *
         * @param value The element to add.
         */
        void Add(const T& value)
        {
            if (m_CurrentSize == m_Capacity)
            {
				Reserve(m_Capacity * 2);
            }

            m_Data[m_CurrentSize++] = value;
        }

        /**
         * @brief Constructs a new element at the end using perfect forwarding.
         *
         * @tparam Args Types of constructor arguments.
         * @param args Arguments forwarded to the constructor of T.
         */
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

        /**
         * @brief Removes the last element from the array.
         */
        void Pop()
        {
            if (m_CurrentSize > 0)
            {
                --m_CurrentSize;
            }
        }

        /**
         * @brief Check if an element exists in the array.
         *
         * @param value The element to search for.
         * @return True if the element is found, false otherwise.
         */
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

        /**
         * @brief Returns index of first occurrence of value, or UINT32_MAX if not found.
         *
         * @param value The element to find.
         * @return Index of the element or UINT32_MAX if not found.
         */
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

        /**
         * @brief Erases the provided element from the array.
         *
         * @param value The element to erase.
         */
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

        /**
         * @brief Erases an element at the provided index from the array.
         *
         * @param index The index of the element to erase.
         */
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

        /**
         * @brief Clears the entire array.
         */
        void Clear()
        {
            std::memset(m_Data, 0, m_CurrentSize * sizeof(T));
            m_CurrentSize = 0;
        }

        /**
         * @brief Ensures the array can hold at least the given capacity without reallocating.
         *
         * @param newCapacity The minimum capacity to ensure.
         */
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

        /**
         * @brief Resizes the array to the new size.
         *
         * If the new size is larger, new elements are default-initialized or set to the provided default value.
         *
         * @param newSize The new size of the array.
         * @param defaultValue The default value to assign to new elements (if any).
         */
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
                operator delete ptr;
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
