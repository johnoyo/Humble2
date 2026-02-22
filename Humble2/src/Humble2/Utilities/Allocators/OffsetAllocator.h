#pragma once

#include "OffsetArena.h"

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>

namespace HBL2
{
    inline thread_local OffsetArena* g_DefaultOffsetArena = nullptr;

    inline void SetDefaultOffsetArena(OffsetArena* a) { g_DefaultOffsetArena = a; }
    inline OffsetArena* GetDefaultOffsetArena() { return g_DefaultOffsetArena; }

    template <typename T>
    class OffsetAllocator
    {
    public:
        using value_type = T;
        using is_always_equal = std::false_type;
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_copy_assignment = std::false_type;
        using propagate_on_container_swap = std::true_type;

        OffsetAllocator() noexcept : m_Arena(GetDefaultOffsetArena()) {}
        explicit OffsetAllocator(OffsetArena* a) noexcept : m_Arena(a) {}

        template <typename U>
        OffsetAllocator(const OffsetAllocator<U>& o) noexcept : m_Arena(o.GetAllocator()) {}

        [[nodiscard]] T* allocate(std::size_t n)
        {
            OffsetArena* a = m_Arena ? m_Arena : GetDefaultOffsetArena();
            HBL2_CORE_ASSERT(a, "OffsetAllocator: allocator null (set default or pass allocator)");

            if (n > (std::size_t(-1) / sizeof(T)))
            {
                return nullptr;
            }

            if (n == 0)
            {
#if defined(_MSC_VER) && defined(_ITERATOR_DEBUG_LEVEL) && (_ITERATOR_DEBUG_LEVEL != 0)
                void* p = a->Alloc(1);
                return static_cast<T*>(p ? p : reinterpret_cast<void*>(static_cast<std::uintptr_t>(alignof(T))));
#else
                return nullptr;
#endif
            }

            void* p = a->Alloc(n * sizeof(T), alignof(T));

            if (!p)
            {
                return nullptr;
            }

            return static_cast<T*>(p);
        }

        void deallocate(T* p, std::size_t n) noexcept
        {
            if (!p)
            {
                return;
            }

            OffsetArena* a = m_Arena ? m_Arena : GetDefaultOffsetArena();

            if (!a)
            {
                return;
            }

            a->Free(p);
        }

        template<typename U> struct rebind { using other = OffsetAllocator<U>; };

        OffsetArena* GetAllocator() const noexcept { return m_Arena ? m_Arena : GetDefaultOffsetArena(); }
        void SetAllocator(OffsetArena* a) noexcept { m_Arena = a; }

        bool operator==(const OffsetAllocator& rhs) const noexcept { return GetAllocator() == rhs.GetAllocator(); }
        bool operator!=(const OffsetAllocator& rhs) const noexcept { return !(*this == rhs); }

    private:
        template <typename> friend class OffsetAllocator;
        OffsetArena* m_Arena = nullptr;
    };
}

