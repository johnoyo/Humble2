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
    template <typename T, typename H>
    class Pool
    {
    public:
        static constexpr uint16_t InvalidIndex = LockFreeIndexStack::InvalidIndex;

        Pool() = default;

        explicit Pool(uint32_t size)
            : m_Size(size)
        {
            if (m_Size == 0 || m_Size > 0xFFFEu)
            {
                m_Size = 32;
            }

            m_Data = new T[m_Size];

            m_GenerationalCounter = new std::atomic<uint16_t>[m_Size];
            m_NextFree = new std::atomic<uint16_t>[m_Size];

            for (uint32_t i = 0; i < m_Size; ++i)
            {
                m_GenerationalCounter[i].store(1, std::memory_order_relaxed);
            }

            m_FreeList.Initialize(m_NextFree, m_Size);
        }

        ~Pool()
        {
            delete[] m_NextFree;
            delete[] m_GenerationalCounter;
        }

        Handle<H> Insert(const T& data)
        {
            const uint16_t index = m_FreeList.Pop();
            if (index == InvalidIndex)
            {
                HBL2_CORE_ASSERT(false, "Exhausted available Pool indeces!");
                return {};
            }

            m_Data[index] = data;

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

        T* Get(Handle<H> handle) const
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

            return &m_Data[idx];
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

            return { index, m_GenerationalCounter[index].load(std::memory_order_acquire) };
        }

        uint32_t Capacity() const { return m_Size; }

    private:
        LockFreeIndexStack m_FreeList;

        std::atomic<uint16_t>* m_NextFree = nullptr;
        T* m_Data = nullptr;
        std::atomic<uint16_t>* m_GenerationalCounter = nullptr;

        uint32_t m_Size = 32;
    };
}
