#include "ResourceDeletionQueue.h"

namespace HBL2
{
    void ResourceDeletionQueue::Push(uint32_t frame, std::function<void()>&& deletor)
    {
        if (!deletor)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_Size >= m_Capacity)
        {
            Resize();
        }

        m_Buffer[m_Tail] = { frame, std::move(deletor) };
        m_Tail = (m_Tail + 1) % m_Capacity;
        ++m_Size;
    }

    void ResourceDeletionQueue::Flush(uint32_t currentFrame)
    {
        m_FlushTemp.clear();

        {
            std::lock_guard<std::mutex> lock(m_Mutex);

            while (m_Size > 0)
            {
                const DeleteItem& item = m_Buffer[m_Head];
                if (item.Frame >= currentFrame)
                {
                    // Stop processing as the current item is not ready for deletion.
                    break;
                }

                m_FlushTemp.push_back(std::move(m_Buffer[m_Head]));
                m_Head = (m_Head + 1) % m_Capacity;
                --m_Size;
            }
        }

        // Run deletors outside the lock.
        for (DeleteItem& item : m_FlushTemp)
        {
            if (item.Deletor)
            {
                item.Deletor();
            }
        }
    }

    void ResourceDeletionQueue::FlushAll()
    {
        while (true)
        {
            m_FlushTemp.clear();

            {
                std::lock_guard<std::mutex> lock(m_Mutex);

                while (m_Size > 0)
                {
                    m_FlushTemp.push_back(std::move(m_Buffer[m_Head]));
                    m_Head = (m_Head + 1) % m_Capacity;
                    --m_Size;
                }
            }

            if (m_FlushTemp.empty())
            {
                break;
            }

            for (DeleteItem& item : m_FlushTemp)
            {
                if (item.Deletor)
                {
                    item.Deletor();
                }
            }
        }
    }

    void ResourceDeletionQueue::Resize()
    {
        size_t newCapacity = m_Capacity * 2;
        std::vector<DeleteItem> newBuffer(newCapacity);

        for (size_t i = 0; i < m_Size; ++i)
        {
            newBuffer[i] = std::move(m_Buffer[(m_Head + i) % m_Capacity]);
        }

        m_Buffer = std::move(newBuffer);
        m_Capacity = newCapacity;
        m_Head = 0;
        m_Tail = m_Size;

        HBL2_CORE_INFO("[ResourceDeletionQueue] Resized buffer to capacity: {0}", m_Capacity);
    }
}