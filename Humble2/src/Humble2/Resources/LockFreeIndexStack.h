#pragma once

#include <atomic>
#include <cstdint>

namespace HBL2
{
    class LockFreeIndexStack
    {
    public:
        static constexpr uint16_t InvalidIndex = 0xFFFF;

        LockFreeIndexStack() = default;

        LockFreeIndexStack(const LockFreeIndexStack&) = delete;
        LockFreeIndexStack& operator=(const LockFreeIndexStack&) = delete;

        void Initialize(std::atomic<uint16_t>* next, uint32_t count)
        {
            m_Next = next;
            m_Count = count;

            // Build a simple forward chain: 0 -> 1 -> 2 -> ... -> Invalid
            for (uint32_t i = 0; i < count; ++i)
            {
                uint16_t nxt = (i + 1 < count) ? static_cast<uint16_t>(i + 1) : InvalidIndex;
                m_Next[i].store(nxt, std::memory_order_relaxed);
            }

            // Head = 0, tag = 0
            m_Head.store(PackHead(0u, 0u), std::memory_order_release);
        }

        uint16_t Pop()
        {
            // Returns InvalidIndex if empty
            while (true)
            {
                uint32_t oldHead = m_Head.load(std::memory_order_acquire);
                const uint16_t idx = UnpackIndex(oldHead);
                const uint16_t tag = UnpackTag(oldHead);

                if (idx == InvalidIndex)
                {
                    return InvalidIndex;
                }

                const uint16_t next = m_Next[idx].load(std::memory_order_relaxed);
                const uint32_t newHead = PackHead(next, static_cast<uint16_t>(tag + 1));

                if (m_Head.compare_exchange_weak(oldHead, newHead, std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    return idx;
                }
            }
        }

        void Push(uint16_t idx)
        {
            while (true)
            {
                uint32_t oldHead = m_Head.load(std::memory_order_acquire);
                const uint16_t headIdx = UnpackIndex(oldHead);
                const uint16_t tag = UnpackTag(oldHead);

                m_Next[idx].store(headIdx, std::memory_order_relaxed);
                const uint32_t newHead = PackHead(idx, static_cast<uint16_t>(tag + 1));

                if (m_Head.compare_exchange_weak(oldHead, newHead, std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    return;
                }
            }
        }

        bool Empty() const
        {
            const uint16_t idx = UnpackIndex(m_Head.load(std::memory_order_acquire));
            return idx == InvalidIndex;
        }

        void ClearToEmpty()
        {
            // Not lock-free "clear" in the semantic sense—meant for shutdown/setup only.
            m_Head.store(PackHead(InvalidIndex, 0u), std::memory_order_release);
        }

        uint32_t Count() const { return m_Count; }

    private:
        static uint32_t PackHead(uint16_t index, uint16_t tag)
        {
            return (static_cast<uint32_t>(tag) << 16) | static_cast<uint32_t>(index);
        }

        static uint16_t UnpackIndex(uint32_t head)
        {
            return static_cast<uint16_t>(head & 0xFFFFu);
        }

        static uint16_t UnpackTag(uint32_t head)
        {
            return static_cast<uint16_t>((head >> 16) & 0xFFFFu);
        }

    private:
        std::atomic<uint32_t> m_Head{ PackHead(InvalidIndex, 0u) };
        std::atomic<uint16_t>* m_Next = nullptr;
        uint32_t m_Count = 0;
    };
}