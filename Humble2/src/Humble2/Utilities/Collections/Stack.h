#pragma once

#include "Base.h"
#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
    /**
     * @brief A Last-In, First-Out (LIFO) stack container.
     *
     * Supports constant-time push, pop, and peek operations.
     * Typically used for function call management, undo systems, and depth-first traversal.
     *
     * @tparam T Type of elements stored in the stack.
     * @tparam TAllocator Allocator type used for dynamic memory.
     */
    template<typename T, typename TAllocator = StandardAllocator>
    class Stack
    {
    public:
        /**
         * @brief Constructs a stack with an optional initial capacity.
         *
         * @param initialCapacity The starting capacity of the stack (default is 8).
         */
        Stack(uint32_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(nullptr)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
        }

        /**
         * @brief Constructs a stack with a custom allocator and optional initial capacity.
         *
         * @param allocator Pointer to the allocator used for memory allocation.
         * @param initialCapacity The starting capacity of the stack (default is 8).
         */
        Stack(TAllocator* allocator, uint32_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(allocator)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
        }

        /**
         * @brief Copy constructor.
         *
         * Creates a new Stack by copying the contents of another.
         * Allocates new memory and copies the elements.
         *
         * @param other The Stack to copy from.
         */
        Stack(const Stack& other)
            : m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Allocator(other.m_Allocator)
        {
            m_Data = Allocate(sizeof(T) * m_Capacity);
            std::memcpy(m_Data, other.m_Data, m_CurrentSize * sizeof(T));
        }

        /**
         * @brief Move constructor.
         *
         * Transfers ownership of resources from another Stack.
         *
         * @param other The Stack to move from.
         */
        Stack(Stack&& other) noexcept
            : m_Data(other.m_Data), m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Allocator(other.m_Allocator)
        {
            other.m_Data = nullptr;
            other.m_Capacity = 0;
            other.m_CurrentSize = 0;
            other.m_Allocator = nullptr;
        }

        /**
         * @brief Destructor that releases allocated memory.
         */
        ~Stack()
        {
            Deallocate(m_Data);
        }

        /**
         * @brief Pushes an element onto the top of the stack.
         *
         * Automatically resizes the internal buffer if needed.
         *
         * @param value The element to push onto the stack.
         */
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

        /**
         * @brief Removes the top element from the stack.
         *
         * Does nothing if the stack is empty.
         */
        void Pop()
        {
            if (m_CurrentSize <= 0)
            {
                return;
            }

            --m_CurrentSize;
        }

        /**
         * @brief Accesses the top element of the stack.
         *
         * @return Reference to the top element.
         * @throws Assertion failure if the stack is empty.
         */
        T& Top()
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Stack is empty!");
            return m_Data[m_CurrentSize - 1];
        }

        /**
         * @brief Accesses the top element of the stack (const version).
         *
         * @return Const reference to the top element.
         * @throws Assertion failure if the stack is empty.
         */
        const T& Top() const
        {
            HBL2_CORE_ASSERT(m_CurrentSize > 0, "Stack is empty!");
            return m_Data[m_CurrentSize - 1];
        }

        /**
         * @brief Returns the number of elements in the stack.
         *
         * @return The current size of the stack.
         */
        uint32_t Size() const { return m_CurrentSize; }

        /**
         * @brief Checks whether the stack is empty.
         *
         * @return True if the stack is empty, false otherwise.
         */
        bool IsEmpty() const { return m_CurrentSize == 0; }

        /**
         * @brief Clears all elements from the stack.
         *
         * Resets the size to zero and zeroes out memory.
         */
        void Clear()
        {
            std::memset(m_Data, 0, m_CurrentSize * sizeof(T));
            m_CurrentSize = 0;
        }

        /**
         * @brief Copy assignment operator.
         *
         * Copies the contents of another Stack, replacing this one.
         * Existing memory is deallocated before assignment.
         *
         * @param other The Stack to copy from.
         * @return Reference to this Stack.
         */
        Stack& operator=(const Stack& other)
        {
            if (this == &other)
            {
                return *this;
            }

            Deallocate(m_Data);

            m_Capacity = other.m_Capacity;
            m_CurrentSize = other.m_CurrentSize;
            m_Allocator = other.m_Allocator;

            m_Data = Allocate(sizeof(T) * m_Capacity);
            std::memcpy(m_Data, other.m_Data, m_CurrentSize * sizeof(T));

            return *this;
        }

        /**
         * @brief Move assignment operator.
         *
         * Transfers ownership of resources from another Stack.
         * Deallocates any existing resources.
         *
         * @param other The Stack to move from.
         * @return Reference to this Stack.
         */
        Stack& operator=(Stack&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }

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