#pragma once

#include "Arena.h"

namespace HBL2
{
    /**
     * @brief STL-compatible allocator that allocates memory from a given Arena.
     *
     * This adapter allows STL containers (and custom containers) to use the Arena allocator seamlessly.
     *
     * @tparam T Type of elements to allocate.
     */
    template<typename T>
    class ArenaAllocator
    {
    public:
        using value_type = T;
        using is_always_equal = std::false_type;
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_copy_assignment = std::false_type;
        using propagate_on_container_swap = std::true_type;

    public:
        explicit ArenaAllocator(Arena* arena) noexcept
            : m_Arena(arena)
        {
        }

        explicit ArenaAllocator(ScratchArena* scratch) noexcept
            : m_Arena(scratch->GetArena())
        {
        }

        template<typename U>
        ArenaAllocator(const ArenaAllocator<U>& other) noexcept
            : m_Arena(other.GetArena())
        {
        }

        [[nodiscard]] T* allocate(std::size_t n)
        {
            HBL2_CORE_ASSERT(m_Arena, "ArenaAllocator: no arena set!");
            void* ptr = m_Arena->Alloc(n * sizeof(T), alignof(T));

            if (!ptr)
            {
                throw std::bad_alloc{};
            }

            return static_cast<T*>(ptr);
        }

        void deallocate(T* p, std::size_t) noexcept
        {
            // Arena allocators typically don't free individual blocks
            // Memory is released when the arena is reset or destroyed
        }

        template<typename U>
        struct rebind { using other = ArenaAllocator<U>; };

        Arena* GetArena() const noexcept { return m_Arena; }

        bool operator==(const ArenaAllocator& rhs) const noexcept { return m_Arena == rhs.m_Arena; }
        bool operator!=(const ArenaAllocator& rhs) const noexcept { return !(*this == rhs); }

    private:
        Arena* m_Arena = nullptr;
    };

} // namespace HBL2
