#pragma once

#include "Handle.h"
#include "RefCounted.h"
#include "LockFreeIndexStack.h"
#include "Utilities/Collections/Span.h"

#include "Core/Allocators.h"
#include "Utilities/Allocators/Arena.h"

#include <atomic>
#include <cstdint>
#include <new>
#include <type_traits>

namespace HBL2
{
    template <typename THot, typename TCold, typename H>
    class RefCountedSplitPool
    {
        struct PoolSlotMeta
        {
            std::atomic<uint16_t> GenerationalCounter{ 0 };
            RefCounted ReferenceCounter{ 0 };
        };

    public:
        static constexpr uint16_t InvalidIndex = LockFreeIndexStack::InvalidIndex;
        static constexpr uint16_t MaxReferenceCount = 4096;

        RefCountedSplitPool() = default;

        explicit RefCountedSplitPool(uint32_t size)
        {
            Initialize(size);
        }

        ~RefCountedSplitPool()
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
                .Add<THot>(m_Size)
                .template Add<TCold>(m_Size)
                .template Add<PoolSlotMeta>(m_Size)
                .template Add<std::atomic<uint16_t>>(m_Size)
                .Total();

            m_Reservation = Allocator::Arena.Reserve("PoolReservation", bytes);
            m_PoolArena.Initialize(&Allocator::Arena, bytes, m_Reservation);

            m_HotData = (THot*)m_PoolArena.Alloc(sizeof(THot) * m_Size, alignof(THot));
            m_ColdData = (TCold*)m_PoolArena.Alloc(sizeof(TCold) * m_Size, alignof(TCold));

            std::memset(m_HotData, 0, sizeof(THot) * m_Size);
            std::memset(m_ColdData, 0, sizeof(TCold) * m_Size);

            void* poolSlotMetaMem = m_PoolArena.Alloc(sizeof(PoolSlotMeta) * m_Size, alignof(PoolSlotMeta));
            m_Meta = m_PoolArena.ConstructArray<PoolSlotMeta>(poolSlotMetaMem, m_Size, 0);

            void* nextFreeMem = m_PoolArena.Alloc(sizeof(std::atomic<uint16_t>) * m_Size, alignof(std::atomic<uint16_t>));
            m_NextFree = m_PoolArena.ConstructArray<std::atomic<uint16_t>>(nextFreeMem, m_Size, 0);

            for (uint32_t i = 0; i < m_Size; ++i)
            {
                m_Meta[i].ReferenceCounter.RefCount.store(0, std::memory_order_relaxed);
                m_Meta[i].GenerationalCounter.store(1, std::memory_order_relaxed);
            }

            m_FreeList.Initialize(m_NextFree, m_Size);
        }

        Handle<H> Insert(THot** outHot, TCold** outCold)
        {
            const uint16_t index = m_FreeList.Pop();
            if (index == InvalidIndex)
            {
                HBL2_CORE_ASSERT(false, "Exhausted available Pool indeces!");
                return {};
            }

            new (&m_HotData[index]) THot;
            new (&m_ColdData[index]) TCold;

            *outHot = &m_HotData[index];
            *outCold = &m_ColdData[index];

            const uint16_t gen = m_Meta[index].GenerationalCounter.load(std::memory_order_relaxed);
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
                return;
            }

            const uint16_t cur = m_Meta[idx].GenerationalCounter.load(std::memory_order_acquire);
            if (cur != handle.m_GenerationalCounter)
            {
                return;
            }

            m_Meta[idx].GenerationalCounter.fetch_add(1, std::memory_order_acq_rel);
            m_Meta[idx].ReferenceCounter.RefCount.store(0, std::memory_order_release);

            m_FreeList.Push(idx);
        }

        THot* GetHot(Handle<H> handle) const
        {
            if (!handle.IsValid())
            {
                return nullptr;
            }

            const uint16_t idx = handle.m_ArrayIndex;
            if (idx == InvalidIndex || idx >= m_Size)
            {
                HBL2_CORE_ASSERT(false, "Exhausted available Pool indeces!");
                return nullptr;
            }

            const uint16_t gen = m_Meta[idx].GenerationalCounter.load(std::memory_order_acquire);
            if (gen != handle.m_GenerationalCounter)
            {
                return nullptr;
            }

            return &m_HotData[idx];
        }

        TCold* GetCold(Handle<H> handle) const
        {
            if (!handle.IsValid())
            {
                return nullptr;
            }

            const uint16_t idx = handle.m_ArrayIndex;
            if (idx == InvalidIndex || idx >= m_Size)
            {
                HBL2_CORE_ASSERT(false, "Exhausted available Pool indeces!");
                return nullptr;
            }

            const uint16_t gen = m_Meta[idx].GenerationalCounter.load(std::memory_order_acquire);
            if (gen != handle.m_GenerationalCounter)
            {
                return nullptr;
            }

            return &m_ColdData[idx];
        }

        bool Get(Handle<H> handle, THot** outHot, TCold** outCold) const
        {
            if (!handle.IsValid())
            {
                return false;
            }

            const uint16_t idx = handle.m_ArrayIndex;
            if (idx == InvalidIndex || idx >= m_Size)
            {
                HBL2_CORE_ASSERT(false, "Exhausted available Pool indeces!");
                return false;
            }

            const uint16_t gen = m_Meta[idx].GenerationalCounter.load(std::memory_order_acquire);
            if (gen != handle.m_GenerationalCounter)
            {
                return false;
            }

            *outHot = &m_HotData[idx];
            *outCold = &m_ColdData[idx];

            return true;
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
                return false;
            }

            const uint16_t cur = m_Meta[idx].GenerationalCounter.load(std::memory_order_acquire);
            if (cur != handle.m_GenerationalCounter)
            {
                return false;
            }

            return m_Meta[idx].ReferenceCounter.TryAddRef();
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
                return false;
            }

            const uint16_t cur = m_Meta[idx].GenerationalCounter.load(std::memory_order_acquire);
            if (cur != handle.m_GenerationalCounter)
            {
                return false;
            }

            return m_Meta[idx].ReferenceCounter.TryReleaseRef();
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
                return false;
            }

            const uint16_t cur = m_Meta[idx].GenerationalCounter.load(std::memory_order_acquire);
            if (cur != handle.m_GenerationalCounter)
            {
                return false;
            }

            const uint16_t rc = m_Meta[idx].ReferenceCounter.RefCount.load(std::memory_order_acquire);
            if (rc >= 1 && rc < MaxReferenceCount)
            {
                return true;
            }

            return false;
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
                return true;
            }

            const uint16_t cur = m_Meta[idx].GenerationalCounter.load(std::memory_order_acquire);
            if (cur != handle.m_GenerationalCounter)
            {
                return true;
            }

            return m_Meta[idx].ReferenceCounter.IsClosing();
        }

        const Span<THot> GetDataHotPool() const
        {
            return { m_HotData, m_Size };
        }

        const Span<TCold> GetDataColdPool() const
        {
            return { m_ColdData, m_Size };
        }

        Handle<H> GetHandleFromIndex(uint16_t index) const
        {
            if (index == InvalidIndex || index >= m_Size)
            {
                return {};
            }

            return { index, m_Meta[index].GenerationalCounter.load(std::memory_order_acquire) };
        }

        uint32_t Capacity() const { return m_Size; }

        uint32_t FreeSlotCount() const { return m_FreeList.NonInvalidCount(); }

    private:
        LockFreeIndexStack m_FreeList;

        std::atomic<uint16_t>* m_NextFree = nullptr;
        THot* m_HotData = nullptr;
        TCold* m_ColdData = nullptr;
        PoolSlotMeta* m_Meta = nullptr;

        uint32_t m_Size = 32;

        PoolReservation* m_Reservation = nullptr;
        Arena m_PoolArena;
    };
}
