#pragma once

#include "Handle.h"
#include "PoolSlotMeta.h"
#include "LockFreeIndexStack.h"
#include "Utilities/Collections/Span.h"

#include "Core/Allocators.h"
#include "Utilities/Allocators/Arena.h"

#include <new>
#include <atomic>
#include <utility>
#include <type_traits>
#include <cstdint>
#include <cstring>

namespace HBL2
{
    template <typename T, typename H>
    class RefCountedPool
    {
    public:
        static constexpr uint16_t InvalidIndex = LockFreeIndexStack::InvalidIndex;

        RefCountedPool() = default;

        explicit RefCountedPool(uint32_t size)
        {
            Initialize(size);
        }

        ~RefCountedPool()
        {
        }

        void Initialize(uint32_t size)
        {
            m_Size = size;

            if (m_Size == 0 || m_Size > 0xFFFEu)
            {
                m_Size = 32;
            }

            size_t bytes = ArenaLayout::Create()
                .Add<T>(m_Size)
                .template Add<PoolSlotMeta>(m_Size)
                .template Add<std::atomic<uint16_t>>(m_Size)
                .Total();

            m_Reservation = Allocator::Arena.Reserve("PoolReservation", bytes);
            m_PoolArena.Initialize(&Allocator::Arena, bytes, m_Reservation);

            m_Data = (T*)m_PoolArena.Alloc(sizeof(T) * m_Size, alignof(T));
            std::memset(m_Data, 0, sizeof(T) * m_Size);

            void* poolSlotMetaMem = m_PoolArena.Alloc(sizeof(PoolSlotMeta) * m_Size, alignof(PoolSlotMeta));
            m_Meta = m_PoolArena.ConstructArray<PoolSlotMeta>(poolSlotMetaMem, m_Size, 0);

            void* nextFreeMem = m_PoolArena.Alloc(sizeof(std::atomic<uint16_t>) * m_Size, alignof(std::atomic<uint16_t>));
            m_NextFree = m_PoolArena.ConstructArray<std::atomic<uint16_t>>(nextFreeMem, m_Size, 0);

            for (uint32_t i = 0; i < m_Size; ++i)
            {
                m_Meta[i].InitFree(1);
            }

            m_FreeList.Initialize(m_NextFree, m_Size);
        }

        template <typename Arg>
        Handle<H> Insert(const Arg&& arg)
        {
            const uint16_t index = m_FreeList.Pop();
            if (index == InvalidIndex)
            {
                HBL2_CORE_ASSERT(false, "Exhausted available Pool indeces!");
                return {};
            }

            new (&m_Data[index]) T(std::forward<const Arg>(arg));

            const uint16_t gen = m_Meta[index].Open();
            return { index, gen };
        }

        void Remove(Handle<H> handle)
        {
            if (!handle.IsValid())
            {
                return;
            }

            const uint16_t idx = handle.m_ArrayIndex;
            if (idx == InvalidIndex || idx >= m_Size)
            {
                HBL2_CORE_ASSERT(false, "Handle index out of range!");
                return;
            }

            if (m_Meta[idx].TryRetire(handle.m_GenerationalCounter))
            {
                m_FreeList.Push(idx);
            }
        }

        T* Get(Handle<H> handle) const
        {
            if (!handle.IsValid())
            {
                return nullptr;
            }

            const uint16_t idx = handle.m_ArrayIndex;
            if (idx == InvalidIndex || idx >= m_Size)
            {
                HBL2_CORE_ASSERT(false, "Handle index out of range!");
                return nullptr;
            }

            if (!m_Meta[idx].Matches(handle.m_GenerationalCounter))
            {
                return nullptr;
            }
            
            return &m_Data[idx];
        }

        bool Acquire(Handle<H> handle)
        {
            if (!handle.IsValid())
            {
                return false;
            }

            const uint16_t idx = handle.m_ArrayIndex;
            if (idx == InvalidIndex || idx >= m_Size)
            {
                HBL2_CORE_ASSERT(false, "Handle index out of range!");
                return false;
            }

            return m_Meta[idx].TryAddRef(handle.m_GenerationalCounter);
        }

        bool Release(Handle<H> handle)
        {
            if (!handle.IsValid())
            {
                return false;
            }

            const uint16_t idx = handle.m_ArrayIndex;
            if (idx == InvalidIndex || idx >= m_Size)
            {
                HBL2_CORE_ASSERT(false, "Handle index out of range!");
                return false;
            }

            return m_Meta[idx].TryReleaseRef(handle.m_GenerationalCounter);
        }
        
        bool Reopen(Handle<H> handle)
        {
            if (!handle.IsValid())
            {
                return false;
            }

            const uint16_t idx = handle.m_ArrayIndex;
            if (idx == InvalidIndex || idx >= m_Size)
            {
                return false;
            }

            return m_Meta[idx].TryReopen(handle.m_GenerationalCounter);
        }

        bool MarkClosed(Handle<H> handle)
        {
            if (!handle.IsValid())
            {
                return false;
            }

            const uint16_t idx = handle.m_ArrayIndex;
            if (idx == InvalidIndex || idx >= m_Size)
            {
                return false;
            }

            return m_Meta[idx].MarkClosed(handle.m_GenerationalCounter);
        }

        bool IsAlive(Handle<H> handle)
        {
            if (!handle.IsValid())
            {
                return false;
            }

            const uint16_t idx = handle.m_ArrayIndex;
            if (idx == InvalidIndex || idx >= m_Size)
            {
                HBL2_CORE_ASSERT(false, "Handle index out of range!");
                return false;
            }

            return m_Meta[idx].IsAlive(handle.m_GenerationalCounter);
        }

        bool IsClosing(Handle<H> handle)
        {
            if (!handle.IsValid())
            {
                return true;
            }

            const uint16_t idx = handle.m_ArrayIndex;
            if (idx == InvalidIndex || idx >= m_Size)
            {
                HBL2_CORE_ASSERT(false, "Handle index out of range!");
                return true;
            }

            return m_Meta[idx].IsClosing(handle.m_GenerationalCounter);
        }

        const Span<T> GetDataPool() const
        {
            return { m_Data, m_Size };
        }

        Handle<H> GetHandleFromIndex(uint16_t index) const
        {
            if (index == InvalidIndex || index >= m_Size)
            {
                return {};
            }

            return { index, m_Meta[index].Generation() };
        }

        uint32_t Capacity() const { return m_Size; }

        uint32_t FreeSlotCount() const { return m_FreeList.NonInvalidCount(); }

    private:
        LockFreeIndexStack m_FreeList;

        std::atomic<uint16_t>* m_NextFree = nullptr;
        T* m_Data = nullptr;
        PoolSlotMeta* m_Meta = nullptr;

        uint32_t m_Size = 32;

        PoolReservation* m_Reservation = nullptr;
        Arena m_PoolArena;
    };
}
