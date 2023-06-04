#pragma once

#include <mutex>

namespace HBL2
{
    template <typename T, size_t Capacity>
    class ThreadSafeRingBuffer
    {
    public:
        inline bool push_back(const T& item)
        {
            bool result = false;
            lock.lock();
            size_t next = (head + 1) % Capacity;
            if (next != tail)
            {
                data[head] = item;
                head = next;
                result = true;
            }
            lock.unlock();
            return result;
        }

        inline bool pop_front(T& item)
        {
            bool result = false;
            lock.lock();
            if (tail != head)
            {
                item = data[tail];
                tail = (tail + 1) % Capacity;
                result = true;
            }
            lock.unlock();
            return result;
        }

    private:
        T data[Capacity];
        size_t head = 0;
        size_t tail = 0;
        std::mutex lock;
    };
}