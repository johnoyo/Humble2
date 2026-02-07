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

        void Initialize(std::atomic<uint16_t>* next, uint32_t count);
        uint16_t Pop();
        void Push(uint16_t idx);
        bool Empty() const;
        void ClearToEmpty();
        uint32_t Count() const;
        uint32_t NonInvalidCount() const;

    private:
        static uint32_t PackHead(uint16_t index, uint16_t tag);
        static uint16_t UnpackIndex(uint32_t head);
        static uint16_t UnpackTag(uint32_t head);

    private:
        std::atomic<uint32_t> m_Head{ PackHead(InvalidIndex, 0u) };
        std::atomic<uint16_t>* m_Next = nullptr;
        uint32_t m_Count = 0;
    };
}