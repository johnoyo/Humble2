#pragma once

#include <cstdint>
#include <atomic>

namespace HBL2
{
	struct RefCounted
	{
        static constexpr uint16_t CLOSING = 0x8000u;
        static constexpr uint16_t COUNT_MASK = 0x7FFFu;

        std::atomic<uint16_t> RefCount{ 0 };

        bool NewRefsAreAllowed() const
        {
            uint16_t cur = RefCount.load(std::memory_order_relaxed);

            if (cur & CLOSING)
            {
                return false;                 // no new refs allowed
            }

            uint16_t cnt = cur & COUNT_MASK;
            if (cnt == COUNT_MASK)
            {
                return false;             // overflow guard
            }

            return true;
        }

        // Try to acquire a reference (Create / AddRef)
        bool TryAddRef()
        {
            uint16_t cur = RefCount.load(std::memory_order_relaxed);
            for (;;)
            {
                if (cur & CLOSING)
                {
                    return false;                 // no new refs allowed
                }

                uint16_t cnt = cur & COUNT_MASK;
                if (cnt == COUNT_MASK)
                {
                    return false;             // overflow guard
                }

                uint16_t next = (cur & CLOSING) | (cnt + 1);
                if (RefCount.compare_exchange_weak(cur, next, std::memory_order_acquire, std::memory_order_relaxed))
                {
                    return true;
                }
                // cur updated by compare_exchange_weak; loop
            }
        }

        // Release a reference (Delete / Release)
        // Returns true if caller should delete the object now.
        bool TryReleaseRef()
        {
            uint16_t prev = RefCount.fetch_sub(1, std::memory_order_release);
            uint16_t prev_cnt = prev & COUNT_MASK;

            // prev_cnt should be > 0; if it was 0, you had an underflow bug.
            if (prev_cnt != 1)
            {
                return false; // still refs remaining
            }

            // We were the one that brought count to 0.
            // Now atomically "close" to block any late add_ref attempts.
            uint16_t expected = 0; // count==0, closing==0
            if (!RefCount.compare_exchange_strong(expected, CLOSING, std::memory_order_acq_rel, std::memory_order_relaxed))
            {
                // If this fails, it means someone managed to increment from 0
                // before we closed (i.e., you allow try_add_ref when count==0).
                return false;
            }

            std::atomic_thread_fence(std::memory_order_acquire);
            return true; // safe point: no one can add_ref anymore
        }
    };
}
