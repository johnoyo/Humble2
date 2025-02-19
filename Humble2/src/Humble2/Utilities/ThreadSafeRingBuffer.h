#pragma once

#include <atomic>
#include <array>

namespace HBL2
{
    template <typename T, size_t Capacity>
    class ThreadSafeRingBuffer
    {
    public:
        ThreadSafeRingBuffer()
            : m_Head(0), m_Tail(0) {}

        ThreadSafeRingBuffer(const ThreadSafeRingBuffer<T, Capacity>& other)
            : m_Head(0), m_Tail(0) {}

        inline bool PushBack(const T& item)
        {
            size_t currentm_Head = m_Head.load(std::memory_order_relaxed);
            size_t next = (currentm_Head + 1) % Capacity;

            if (next == m_Tail.load(std::memory_order_acquire)) // Buffer full
            {
                return false;
            }

            m_Data[currentm_Head] = item;
            m_Head.store(next, std::memory_order_release);
            return true;
        }

        inline bool PopFront(T& item)
        {
            size_t currentm_Tail = m_Tail.load(std::memory_order_relaxed);

            if (currentm_Tail == m_Head.load(std::memory_order_acquire)) // Buffer empty
            {
                return false;
            }

            item = m_Data[currentm_Tail];
            m_Tail.store((currentm_Tail + 1) % Capacity, std::memory_order_release);
            return true;
        }

        inline void Reset()
        {
            m_Head.store(0, std::memory_order_relaxed);
            m_Tail.store(0, std::memory_order_relaxed);
        }

    private:
        std::array<T, Capacity> m_Data;
        std::atomic<size_t> m_Head;
        std::atomic<size_t> m_Tail;
    };
}