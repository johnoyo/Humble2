#pragma once

#include "Base.h"

#include <vector>
#include <functional>
#include <cstdint>

namespace HBL2
{
    struct DeleteItem
    {
        uint32_t Frame = 0;
        std::function<void()> Deletor;
    };

    class ResourceDeletionQueue
    {
    public:
        ResourceDeletionQueue()
            : m_Capacity(512), m_Buffer(512), m_Head(0), m_Tail(0), m_Size(0)
        {
            m_FlushTemp.reserve(256);
        }

        ResourceDeletionQueue(size_t initialCapacity)
            : m_Capacity(initialCapacity), m_Buffer(initialCapacity), m_Head(0), m_Tail(0), m_Size(0)
        {
            m_FlushTemp.reserve(256);
        }

        void Push(uint32_t frame, std::function<void()>&& deletor);
        void Flush(uint32_t currentFrame);
        void FlushAll();
        inline bool IsEmpty() const { return m_Size == 0; }

    private:
        size_t m_Capacity;
        std::vector<DeleteItem> m_Buffer;
        size_t m_Head;  // Points to the next item to be processed.
        size_t m_Tail;  // Points to the next free slot.
        size_t m_Size;  // Number of elements currently in the buffer.

        std::mutex m_Mutex;
        std::vector<DeleteItem> m_FlushTemp;

    private:
        void Resize();
    };
}