#pragma once

#include "Arena.h"

namespace HBL2
{
    /**
     * @brief Thin RAII wrapper that marks an Arena on construction and restores on destruction.
     */
    class ScratchArena
    {
    public:
        /**
         * @brief Construct and capture arena mark.
         *
         * @param arena Reference to an existing Arena.
         */
        explicit ScratchArena(Arena& arena)
            : m_Arena(arena), m_Mark(arena.Mark())
        {
        }

        /**
         * @brief Restore arena to the captured mark on destruction.
         */
        ~ScratchArena()
        {
            m_Arena.Restore(m_Mark);
        }

        /**
         * @brief Allocate temporary bytes from the underlying arena.
         *
         * @param bytes Number of bytes to allocate.
         * @param align Alignment requirement.
         * @return Pointer to allocated memory.
         */
        void* Alloc(size_t bytes, size_t align = alignof(std::max_align_t))
        {
            return m_Arena.Alloc(bytes, align);
        }

        /**
         * @brief Contruct the provided allocated memory (by calling the ctor using placement new).
         *
         * @param allocatedMemory Pointer to allocated memory.
         * @param args Arguments to forward to ctor of type.
         * @return Pointer to constructed memory.
         */
        template<typename T, typename... Args>
        T* Construct(void* allocatedMemory, Args&&... args)
        {
            return m_Arena.Construct<T>(allocatedMemory, std::forward<Args>(args)...);
        }

        /**
         * @brief Allocate and construct object of type T in scratch area.
         *
         * @param args Arguments to forward to ctor of type.
         * @return Pointer to allocated and constructed memory.
         */
        template<typename T, typename... Args>
        T* AllocConstruct(Args&&... args)
        {
            return m_Arena.AllocConstruct<T>(std::forward<Args>(args)...);
        }

        /**
         * @brief Destruct an object of type T inside the arena (call the dtor of the object).
         *
         * @param object Pointer to the object.
         */
        template<typename T>
        void Destruct(T* object)
        {
            m_Arena.Destruct(object);
        }

        inline Arena* GetArena() { return &m_Arena; }

    private:
        Arena& m_Arena;
        Arena::Marker m_Mark;
    };
}