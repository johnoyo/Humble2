#pragma once

#include "Handle.h"
#include "LockFreeIndexStack.h"
#include "Utilities/Collections/Span.h"

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
            : m_Size(size)
        {
            if (m_Size == 0 || m_Size > 0xFFFEu)
            {
                m_Size = 32;
            }

            m_HotData = new THot[m_Size];
            m_ColdData = new TCold[m_Size];

            m_GenerationalCounter = new std::atomic<uint16_t>[m_Size];
            m_NextFree = new std::atomic<uint16_t>[m_Size];

            for (uint32_t i = 0; i < m_Size; ++i)
            {
                m_GenerationalCounter[i].store(1, std::memory_order_relaxed);
            }

            m_FreeList.Initialize(m_NextFree, m_Size);
        }

        ~SplitPool()
        {
            delete[] m_NextFree;
            delete[] m_GenerationalCounter;
        }

        Handle<H> Insert(const THot&& hotInit, const TCold&& coldInit)
        {
            const uint16_t index = m_FreeList.Pop();
            if (index == InvalidIndex)
            {
                HBL2_CORE_ASSERT(false, "Exhausted available Pool indeces!");
                return {};
            }

            new (&m_HotData[index]) THot(std::forward<THot>(hotInit);
            new (&m_ColdData[index] TCold(std::forward<TCold>(coldInit);

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

        bool Get(Handle<H> handle, THot* outHot, TCold* outCold) const
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

            outHot = &m_HotData[idx];
            outCold = &m_ColdData[idx];

            return false;
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

    private:
        LockFreeIndexStack m_FreeList;
        std::atomic<uint16_t>* m_NextFree = nullptr;

        THot* m_HotData = nullptr;
        TCold* m_ColdData = nullptr;

        std::atomic<uint16_t>* m_GenerationalCounter = nullptr;

        uint32_t m_Size = 32;
    };
}
