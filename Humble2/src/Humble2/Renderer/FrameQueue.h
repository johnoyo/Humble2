#pragma once

#include <mutex>
#include <condition_variable>
#include <array>
#include <optional>

namespace HBL2
{
    template<typename T, size_t BufferCount = 2>
    class FrameQueue
    {
    public:
        FrameQueue() = default;

        void Push(const T& data)
        {
            std::unique_lock<std::mutex> lock(m_Mutex);

            // Wait if all buffers are full
            m_CanWrite.wait(lock, [&]() { return m_Count < BufferCount; });

            m_Buffer[m_WriteIndex] = data;
            m_WriteIndex = (m_WriteIndex + 1) % BufferCount;
            m_Count++;

            lock.unlock();
            m_CanRead.notify_one();
        }

        std::optional<T> Pop()
        {
            std::unique_lock<std::mutex> lock(m_Mutex);

            // Wait until there is something to read or the queue is stopping
            m_CanRead.wait(lock, [&]() { return m_Count > 0 || m_Stopping; });

            if (m_Stopping)
            {
                return std::nullopt;
            }

            T data = m_Buffer[m_ReadIndex];
            m_ReadIndex = (m_ReadIndex + 1) % BufferCount;
            m_Count--;

            lock.unlock();
            m_CanWrite.notify_one();

            return data;
        }

        void Stop()
        {
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                m_Stopping = true;
            }
            m_CanRead.notify_all();
        }

    private:
        std::array<T, BufferCount> m_Buffer;
        size_t m_WriteIndex = 0;
        size_t m_ReadIndex = 0;
        size_t m_Count = 0;
        bool m_Stopping = false;

        std::mutex m_Mutex;
        std::condition_variable m_CanRead;
        std::condition_variable m_CanWrite;
    };
}