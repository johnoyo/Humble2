#pragma once

#include "Base.h"
#include "Utilities\Allocators\ArenaAllocator.h"

#include <cstring>
#include <stdint.h>
#include <type_traits>
#include <new>
#include <optional>

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
    template<typename T, typename TAllocator = ArenaAllocator<T>>
    class Stack
    {
    public:
        Stack(uint32_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0)
        {
            m_Data = Allocate(m_Capacity);
        }

        Stack(const TAllocator& alloc, uint32_t initialCapacity = 8)
            : m_Capacity(initialCapacity), m_CurrentSize(0), m_Allocator(alloc)
        {
            m_Data = Allocate(m_Capacity);
        }

        /**
         * @brief Copy constructor.
         *
         * Allocates new memory in the same allocator and copies elements.
         */
        Stack(const Stack& other)
            : m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Allocator(other.m_Allocator)
        {
            m_Data = Allocate(m_Capacity);

            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memcpy(m_Data, other.m_Data, m_CurrentSize * sizeof(T));
            }
            else
            {
                for (uint32_t i = 0; i < m_CurrentSize; ++i)
                {
                    new (m_Data + i) T(other.m_Data[i]);
                }
            }
        }

        /**
        * @brief Move constructor.
        *
        * Transfers ownership of the memory and allocator.
        */
        Stack(Stack&& other) noexcept
            : m_Data(other.m_Data), m_Capacity(other.m_Capacity), m_CurrentSize(other.m_CurrentSize), m_Allocator(std::move(other.m_Allocator))
        {
            other.m_Data = nullptr;
            other.m_Capacity = 0;
            other.m_CurrentSize = 0;
        }

        ~Stack()
        {
            Clear();
            Deallocate(m_Data, m_Capacity);
        }

        /**
         * @brief Copy assignment operator.
         */
        Stack& operator=(const Stack& other)
        {
            if (this == &other)
            {
                return *this;
            }

            Clear();
            Deallocate(m_Data, m_Capacity);

            m_Capacity = other.m_Capacity;
            m_CurrentSize = other.m_CurrentSize;
            m_Allocator = other.m_Allocator;

            m_Data = Allocate(m_Capacity);

            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memcpy(m_Data, other.m_Data, m_CurrentSize * sizeof(T));
            }
            else
            {
                for (uint32_t i = 0; i < m_CurrentSize; ++i)
                {
                    new (m_Data + i) T(other.m_Data[i]);
                }
            }

            return *this;
        }

        /**
         * @brief Move assignment operator.
         */
        Stack& operator=(Stack&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }

            Clear();
            Deallocate(m_Data, m_Capacity);

            m_Data = other.m_Data;
            m_Capacity = other.m_Capacity;
            m_CurrentSize = other.m_CurrentSize;
            m_Allocator = std::move(other.m_Allocator);

            other.m_Data = nullptr;
            other.m_Capacity = 0;
            other.m_CurrentSize = 0;

            return *this;
        }

        void Push(const T& value)
        {
            if (m_CurrentSize >= m_Capacity)
            {
                Resize(m_Capacity * 2);
            }

            new (m_Data + m_CurrentSize++) T(value);
        }

        void Push(T&& value)
        {
            if (m_CurrentSize >= m_Capacity)
            {
                Resize(m_Capacity * 2);
            }

            new (m_Data + m_CurrentSize++) T(std::move(value));
        }

        template<typename... Args>
        void Emplace(Args&&... args)
        {
            if (m_CurrentSize >= m_Capacity)
            {
                Resize(m_Capacity * 2);
            }

            new (m_Data + m_CurrentSize++) T(std::forward<Args>(args)...);
        }

        void Pop()
        {
            if (m_CurrentSize == 0)
            {
                return;
            }

            --m_CurrentSize;

            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                m_Data[m_CurrentSize].~T();
            }
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

        uint32_t Size() const noexcept { return m_CurrentSize; }
        bool Empty() const noexcept { return m_CurrentSize == 0; }

        void Clear()
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (uint32_t i = 0; i < m_CurrentSize; ++i)
                {
                    m_Data[i].~T();
                }
            }

            m_CurrentSize = 0;
        }

        T* begin() noexcept { return m_Data; }
        T* end() noexcept { return m_Data + m_CurrentSize; }
        const T* begin() const noexcept { return m_Data; }
        const T* end() const noexcept { return m_Data + m_CurrentSize; }

    private:
        void Resize(uint32_t newCapacity)
        {
            T* newData = Allocate(newCapacity);
            HBL2_CORE_ASSERT(newData, "ArenaAllocator Stack: allocation failed!");

            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memcpy(newData, m_Data, m_CurrentSize * sizeof(T));
            }
            else
            {
                for (uint32_t i = 0; i < m_CurrentSize; ++i)
                {
                    new (newData + i) T(std::move(m_Data[i]));
                }
            }

            Deallocate(m_Data, m_Capacity);
            m_Data = newData;
            m_Capacity = newCapacity;
        }

        T* Allocate(uint32_t count)
        {
            if (!m_Allocator.has_value())
            {
                return new T[count];
            }

            return m_Allocator->allocate(count);
        }

        void Deallocate(T* ptr, uint32_t count)
        {
            if (!m_Allocator.has_value())
            {
                delete[] ptr;
                return;
            }

            m_Allocator->deallocate(ptr, count);
        }

    private:
        T* m_Data = nullptr;
        uint32_t m_Capacity = 0; // Not in bytes
        uint32_t m_CurrentSize = 0; // Not in bytes
        std::optional<TAllocator> m_Allocator;
    };

    template<typename T>
    auto MakeStack(Arena* arena, uint32_t initialCapacity = 8)
    {
        return Stack<T>(ArenaAllocator<T>(&arena), initialCapacity);
    }

    template<typename T>
    auto MakeStack(ScratchArena* scratch, uint32_t initialCapacity = 8)
    {
        return Stack<T>(ArenaAllocator<T>(scratch), initialCapacity);
    }
}