#pragma once

#include "Base.h"

#include <vector>
#include <functional>
#include <cstdint>

struct DeleteItem
{
    uint32_t Frame = 0;
    std::function<void()> Deletor;
};

class ResourceDeletionQueue
{
public:
    ResourceDeletionQueue()
        : m_Capacity(512), m_Buffer(512), m_Head(0), m_Tail(0), m_Size(0) {}

    ResourceDeletionQueue(size_t initialCapacity)
        : m_Capacity(initialCapacity), m_Buffer(initialCapacity), m_Head(0), m_Tail(0), m_Size(0) {}

    void Push(uint32_t frame, const std::function<void()>& deletor)
    {
        if (m_Size >= m_Capacity)
        {
            Resize();
        }

        m_Buffer[m_Tail] = { frame, deletor };
        m_Tail = (m_Tail + 1) % m_Capacity;
        ++m_Size;
    }

    void Flush(uint32_t currentFrame)
    {
        while (m_Size > 0)
        {
            const DeleteItem& item = m_Buffer[m_Head];
            if (item.Frame >= currentFrame)
            {
                // Stop processing as the current item is not ready for deletion.
                break;
            }

            if (item.Deletor)
            {
                item.Deletor();
            }

            m_Head = (m_Head + 1) % m_Capacity;
            --m_Size;
        }
    }

    bool IsEmpty() const
    {
        return m_Size == 0;
    }

private:
    size_t m_Capacity;
    std::vector<DeleteItem> m_Buffer;
    size_t m_Head;  // Points to the next item to be processed.
    size_t m_Tail;  // Points to the next free slot.
    size_t m_Size;  // Number of elements currently in the buffer.

private:
    void Resize()
    {
        size_t newCapacity = m_Capacity * 2;
        std::vector<DeleteItem> newBuffer(newCapacity);

        for (size_t i = 0; i < m_Size; ++i)
        {
            newBuffer[i] = m_Buffer[(m_Head + i) % m_Capacity];
        }

        m_Buffer = std::move(newBuffer);
        m_Capacity = newCapacity;
        m_Head = 0;
        m_Tail = m_Size;

        HBL2_CORE_INFO("[ResourceDeletionQueue] Resized buffer to capacity: {0}", m_Capacity);
    }
};