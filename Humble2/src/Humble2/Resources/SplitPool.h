#pragma once

#include "Handle.h"
#include "LockFreeIndexStack.h"
#include "Utilities/Collections/Span.h"

#include "Core\Allocators.h"
#include "Utilities\Allocators\Arena.h"

#include <atomic>
#include <cstdint>
#include <new>
#include <type_traits>

namespace HBL2
{
    template <typename THot, typename TCold, typename H>
    class SplitPool
    {
    public:
        static constexpr uint16_t InvalidIndex = LockFreeIndexStack::InvalidIndex;

        SplitPool() = default;

        explicit SplitPool(uint32_t size)
        {
            Initialize(size);
        }

        ~SplitPool()
        {
        }

        void Initialize(uint32_t size)
        {
            m_Size = size;

            if (m_Size == 0 || m_Size > 0xFFFEu)
            {
                m_Size = 32;
            }

            uint32_t byteSize = (sizeof(THot) + sizeof(TCold) + 2 * sizeof(std::atomic<uint16_t>)) * m_Size;
            m_Reservation = Allocator::Arena.Reserve(std::string("SplitPool-") + typeid(THot).name() + typeid(TCold).name(), byteSize);
            m_PoolArena.Initialize(&Allocator::Arena, byteSize, m_Reservation);

            m_HotData = (THot*)m_PoolArena.Alloc(sizeof(THot) * m_Size, alignof(THot));
            m_ColdData = (TCold*)m_PoolArena.Alloc(sizeof(TCold) * m_Size, alignof(TCold));

            void* generationalCounterMem = m_PoolArena.Alloc(sizeof(std::atomic<uint16_t>) * m_Size, alignof(std::atomic<uint16_t>));
            m_GenerationalCounter = m_PoolArena.ConstructArray<std::atomic<uint16_t>>(generationalCounterMem, m_Size, 0);

            void* nextFreeMem = m_PoolArena.Alloc(sizeof(std::atomic<uint16_t>) * m_Size, alignof(std::atomic<uint16_t>));
            m_NextFree = m_PoolArena.ConstructArray<std::atomic<uint16_t>>(nextFreeMem, m_Size, 0);

            for (uint32_t i = 0; i < m_Size; ++i)
            {
                m_GenerationalCounter[i].store(1, std::memory_order_relaxed);
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

            const uint16_t gen = m_GenerationalCounter[index].load(std::memory_order_relaxed);
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

            const uint16_t cur = m_GenerationalCounter[idx].load(std::memory_order_acquire);
            if (cur != handle.m_GenerationalCounter)
            {
                return;
            }

            m_GenerationalCounter[idx].fetch_add(1, std::memory_order_acq_rel);
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

            const uint16_t gen = m_GenerationalCounter[idx].load(std::memory_order_acquire);
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

            const uint16_t gen = m_GenerationalCounter[idx].load(std::memory_order_acquire);
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

            const uint16_t gen = m_GenerationalCounter[idx].load(std::memory_order_acquire);
            if (gen != handle.m_GenerationalCounter)
            {
                return false;
            }

            *outHot = &m_HotData[idx];
            *outCold = &m_ColdData[idx];

            return true;
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

            return { index, m_GenerationalCounter[index].load(std::memory_order_acquire) };
        }

        uint32_t Capacity() const { return m_Size; }

        uint32_t FreeSlotCount() const { return m_FreeList.NonInvalidCount(); }

    private:
        LockFreeIndexStack m_FreeList;
        std::atomic<uint16_t>* m_NextFree = nullptr;

        THot* m_HotData = nullptr;
        TCold* m_ColdData = nullptr;

        std::atomic<uint16_t>* m_GenerationalCounter = nullptr;

        uint32_t m_Size = 32;

        PoolReservation* m_Reservation = nullptr;
        Arena m_PoolArena;
    };
}
