#pragma once

#include "Base.h"
#include "Utilities\Allocators\Arena.h"

#include <cstdint>
#include <initializer_list>
#include <cstring>

namespace HBL2
{
    /// Simple variable length array backed by a heap-allocated fixed size buffer
    template <class T>
    class FixedArray
    {
    public:
        /// Default constructor
        FixedArray() = default;

        /// Constructor with capacity
        explicit FixedArray(Arena* arena, uint32_t inCapacity)
            : m_Arena(arena), m_Capacity(inCapacity)
        {
            m_Elements = (T*)m_Arena->Alloc(sizeof(T) * m_Capacity);
        }

        /// Constructor from initializer list
        FixedArray(Arena* arena, uint32_t inCapacity, std::initializer_list<T> inList)
            : m_Arena(arena), m_Capacity(inCapacity)
        {
            HBL2_CORE_ASSERT(inList.size() <= inCapacity, "");

            m_Elements = (T*)m_Arena->Alloc(sizeof(T) * m_Capacity);
            for (const T& v : inList)
            {
                new (&m_Elements[m_Size++]) T(v);
            }
        }

        /// Copy constructor
        FixedArray(const FixedArray<T>& inRHS)
            : m_Arena(inRHS.m_Arena), m_Capacity(inRHS.m_Capacity)
        {
            m_Elements = (T*)m_Arena->Alloc(sizeof(T) * m_Capacity);
            while (m_Size < inRHS.m_Size)
            {
                new (&m_Elements[m_Size]) T(inRHS[m_Size]);
                ++m_Size;
            }
        }

        /// Move constructor
        FixedArray(FixedArray<T>&& inRHS) noexcept
            : m_Size(inRHS.m_Size), m_Capacity(inRHS.m_Capacity), m_Elements(inRHS.m_Elements), m_Arena(inRHS.m_Arena)
        {
            inRHS.m_Size = 0;
            inRHS.m_Capacity = 0;
            inRHS.m_Elements = nullptr;
            inRHS.m_Arena = nullptr;
        }

        /// Destructor
        ~FixedArray()
        {
            clear();
            m_Elements = nullptr;
        }

        /// Destruct all elements and set length to zero
        void clear()
        {
            if constexpr (!std::is_trivially_destructible<T>())
            {
                for (T* e = reinterpret_cast<T*>(m_Elements), *end = e + m_Size; e < end; ++e)
                {
                    e->~T();
                }
            }
            m_Size = 0;
        }

        /// Add element to the back of the array
        void push_back(const T& inElement)
        {
            HBL2_CORE_ASSERT(m_Size < m_Capacity, "");
            new (&m_Elements[m_Size++]) T(inElement);
        }

        /// Construct element at the back of the array
        template <class... A>
        void emplace_back(A&&... inElement)
        {
            HBL2_CORE_ASSERT(m_Size < m_Capacity, "");
            new (&m_Elements[m_Size++]) T(std::forward<A>(inElement)...);
        }

        /// Remove element from the back of the array
        void pop_back()
        {
            HBL2_CORE_ASSERT(m_Size > 0, "");
            reinterpret_cast<T&>(m_Elements[--m_Size]).~T();
        }

        /// Returns true if there are no elements in the array
        bool empty() const { return m_Size == 0; }

        /// Returns amount of elements in the array
        uint32_t size() const { return m_Size; }

        /// Returns maximum amount of elements the array can hold
        uint32_t capacity() const { return m_Capacity; }

        /// Resize array to new length
        void resize(uint32_t inNewSize)
        {
            HBL2_CORE_ASSERT(inNewSize <= m_Capacity, "");

            if constexpr (!std::is_trivially_constructible<T>())
            {
                for (T* e = reinterpret_cast<T*>(m_Elements) + m_Size, *end = reinterpret_cast<T*>(m_Elements) + inNewSize; e < end; ++e)
                {
                    new (e) T;
                }
            }

            if constexpr (!std::is_trivially_destructible<T>())
            {
                for (T* e = reinterpret_cast<T*>(m_Elements) + inNewSize, *end = reinterpret_cast<T*>(m_Elements) + m_Size; e < end; ++e)
                {
                    e->~T();
                }
            }

            m_Size = inNewSize;
        }

        using const_iterator = const T*;
        using iterator = T*;

        const_iterator begin() const { return reinterpret_cast<const T*>(m_Elements); }
        const_iterator end()   const { return reinterpret_cast<const T*>(m_Elements + m_Size); }
        iterator       begin() { return reinterpret_cast<T*>(m_Elements); }
        iterator       end() { return reinterpret_cast<T*>(m_Elements + m_Size); }

        const T* data() const { return reinterpret_cast<const T*>(m_Elements); }
        T* data() { return reinterpret_cast<T*>(m_Elements); }

        /// Element access
        T& operator[](uint32_t inIdx)
        {
            HBL2_CORE_ASSERT(inIdx < m_Size, "");
            return reinterpret_cast<T&>(m_Elements[inIdx]);
        }

        const T& operator[](uint32_t inIdx) const
        {
            HBL2_CORE_ASSERT(inIdx < m_Size, "");
            return reinterpret_cast<const T&>(m_Elements[inIdx]);
        }

        T& at(uint32_t inIdx)
        {
            HBL2_CORE_ASSERT(inIdx < m_Size, "");
            return reinterpret_cast<T&>(m_Elements[inIdx]);
        }

        const T& at(uint32_t inIdx) const
        {
            HBL2_CORE_ASSERT(inIdx < m_Size, "");
            return reinterpret_cast<const T&>(m_Elements[inIdx]);
        }

        T& front()
        {
            HBL2_CORE_ASSERT(m_Size > 0, "");
            return reinterpret_cast<T&>(m_Elements[0]);
        }

        const T& front() const
        {
            HBL2_CORE_ASSERT(m_Size > 0, "");
            return reinterpret_cast<const T&>(m_Elements[0]);
        }

        T& back()
        {
            HBL2_CORE_ASSERT(m_Size > 0, "");
            return reinterpret_cast<T&>(m_Elements[m_Size - 1]);
        }

        const T& back() const
        {
            HBL2_CORE_ASSERT(m_Size > 0, "");
            return reinterpret_cast<const T&>(m_Elements[m_Size - 1]);
        }

        /// Remove one element from the array
        void erase(const_iterator inIter)
        {
            uint32_t p = uint32_t(inIter - begin());
            HBL2_CORE_ASSERT(p < m_Size, "");
            reinterpret_cast<T&>(m_Elements[p]).~T();
            if (p + 1 < m_Size)
            {
                std::memmove(m_Elements + p, m_Elements + p + 1, (m_Size - p - 1) * sizeof(T));
            }
            --m_Size;
        }

        /// Remove a range of elements from the array
        void erase(const_iterator inBegin, const_iterator inEnd)
        {
            uint32_t p = uint32_t(inBegin - begin());
            uint32_t n = uint32_t(inEnd - inBegin);

            HBL2_CORE_ASSERT(inEnd <= end(), "");

            for (uint32_t i = 0; i < n; ++i)
            {
                reinterpret_cast<T&>(m_Elements[p + i]).~T();
            }

            if (p + n < m_Size)
            {
                std::memmove(m_Elements + p, m_Elements + p + n, (m_Size - p - n) * sizeof(T));
            }

            m_Size -= n;
        }

        /// Copy assignment
        FixedArray<T>& operator=(const FixedArray<T>& inRHS)
        {
            if (this == &inRHS)
            {
                return *this;
            }

            clear();

            m_Capacity = inRHS.m_Capacity;
            m_Elements = (T*)m_Arena->Alloc(sizeof(T) * m_Capacity);

            while (m_Size < inRHS.m_Size)
            {
                new (&m_Elements[m_Size]) T(inRHS[m_Size]);
                ++m_Size;
            }

            return *this;
        }

        /// Move assignment
        FixedArray<T>& operator=(FixedArray<T>&& inRHS) noexcept
        {
            if (this == &inRHS)
            {
                return *this;
            }

            clear();

            m_Size = inRHS.m_Size;
            m_Capacity = inRHS.m_Capacity;
            m_Elements = inRHS.m_Elements;
            m_Arena = inRHS.m_Arena;

            inRHS.m_Size = 0;
            inRHS.m_Capacity = 0;
            inRHS.m_Elements = nullptr;
            inRHS.m_Arena = nullptr;

            return *this;
        }

        /// Equality operators
        bool operator==(const FixedArray<T>& inRHS) const
        {
            if (m_Size != inRHS.m_Size)
            {
                return false;
            }

            for (uint32_t i = 0; i < m_Size; ++i)
            {
                if (!(reinterpret_cast<const T&>(m_Elements[i]) == reinterpret_cast<const T&>(inRHS.m_Elements[i])))
                {
                    return false;
                }
            }

            return true;
        }

        bool operator!=(const FixedArray<T>& inRHS) const
        {
            return !(*this == inRHS);
        }

    protected:
        uint32_t m_Size = 0;
        uint32_t m_Capacity = 0;
        T* m_Elements = nullptr;
        Arena* m_Arena = nullptr;
    };
}