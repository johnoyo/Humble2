#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    /// <summary>
    /// A Last-In, First-Out (LIFO) data structure that supports push, pop, and peek operations in O(1) time.
    /// Used for function call management, undo operations, and depth-first search.
    /// </summary>
    /// <typeparam name="T">The type of the element to store in the array.</typeparam>
    /// <typeparam name="TAllocator">The allocator type to use.</typeparam>
    template<typename T, typename TAllocator = StandardAllocator>
    class Stack
    {
    public:
        /// <summary>
        /// Constructs a Stack with an optional initial capacity.
        /// </summary>
        /// <param name="initialCapacity">The starting capacity of the stack.</param>
        Stack(uint32_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(nullptr)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
        }

        /// <summary>
        /// Constructs a Stack with an optional initial capacity and a custom allocator.
        /// </summary>
        /// <param name="allocator">The allocator to use for memory allocation.</param>
        /// <param name="initialCapacity">The starting capacity of the stack.</param>
        Stack(TAllocator* allocator, uint32_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(allocator)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
        }

        /// <summary>
        /// Destructor to release allocated memory.
        /// </summary>
        ~Stack()
        {
            Deallocate(m_Data);
        }

        /// <summary>
        /// Pushes an element to the top of the stack.
        /// </summary>
        /// <param name="value">The element to push.</param>
        void Push(const T& value)
        {
            if (m_CurrentSize >= m_Capacity)
            {
                m_Capacity *= 2;

                T* newData = Allocate(sizeof(T) * m_Capacity);
                HBL2_CORE_ASSERT(newData, "Memory allocation failed!");

                memcpy(newData, m_Data, m_CurrentSize * sizeof(T));
                Deallocate(m_Data);
                m_Data = newData;
            }

            m_Data[m_CurrentSize++] = value;
        }

        /// <summary>
        /// Removes the element in top of the stack.
        /// </summary>
        void Pop()
        {
            if (m_CurrentSize <= 0)
            {
                return;
            }

            --m_CurrentSize;
        }

        /// <summary>
        /// Returns the top element of the stack.
        /// </summary>
        /// <returns>The top element of the stack.</returns>
        T& Top()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Stack is empty!");
            return m_Data[m_CurrentSize - 1];
        }

        /// <summary>
        /// Returns the top element of the stack.
        /// </summary>
        /// <returns>The top element of the stack.</returns>
        const T& Top() const
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Stack is empty!");
            return m_Data[m_CurrentSize - 1];
        }

        /// <summary>
        /// Returns the number of elements in the stack.
        /// </summary>
        /// <returns>The number of elements in the stack.</returns>
        uint32_t Size() const { return m_CurrentSize; }

        /// <summary>
        /// Check if the stack is empty.
        /// </summary>
        /// <returns>True if the stack is empty, false otherwise.</returns>
        bool IsEmpty() const { return m_CurrentSize == 0; }

        /// <summary>
        /// Clears the entire stack.
        /// </summary>
        void Clear()
        {
            std::memset(m_Data, 0, m_CurrentSize * sizeof(T));
            m_CurrentSize = 0;
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
        T* m_Data;
        uint32_t m_Capacity; // Not in bytes
        uint32_t m_CurrentSize; // Not in bytes
        TAllocator* m_Allocator = nullptr; // Does not own the pointer
    };

    template<typename T, typename TAllocator>
    auto MakeStack(TAllocator* allocator, uint32_t initialCapacity = 8)
    {
        return Stack<T, TAllocator>(allocator, initialCapacity);
    }
}