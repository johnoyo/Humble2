#pragma once

#include <atomic>
#include <cstdint>

namespace HBL2
{
	// Per-slot state for RefCountedPool / RefCountedSplitPool.
	// One 32-bit atomic packs the generation and the reference count, so every
	// operation is a single CAS that also validates the generation. A stale
	// handle can therefore never touch a newer object's count.
	//
	//   [31:16] generation
	//   [15]    CLOSING (no new refs allowed; slot is dying or free)
	//   [14:0]  reference count
	struct PoolSlotMeta
	{
		static constexpr uint32_t CLOSING    = 0x8000u;
		static constexpr uint32_t COUNT_MASK = 0x7FFFu;
		static constexpr uint32_t GEN_SHIFT  = 16;

		std::atomic<uint32_t> State{ 0 };

		static constexpr uint32_t Pack(uint16_t gen, uint32_t low) { return (uint32_t(gen) << GEN_SHIFT) | (low & 0xFFFFu); }
		static constexpr uint16_t GenOf(uint32_t s)                { return uint16_t(s >> GEN_SHIFT); }
		static constexpr uint32_t LowOf(uint32_t s)                { return s & 0xFFFFu; }
		static constexpr uint16_t NextGen(uint16_t g)              { return g == 0xFFFFu ? uint16_t(1) : uint16_t(g + 1); } // skip 0

		// Pool initialization only (single-threaded): slot is free, so it starts closed.
		void InitFree(uint16_t gen)
		{
			State.store(Pack(gen, CLOSING), std::memory_order_relaxed);
		}

		// Called by Insert after popping the slot from the free list (exclusive owner).
		// Clears CLOSING, count = 0, generation unchanged. Returns the generation for the handle.
		uint16_t Open()
		{
			const uint16_t gen = GenOf(State.load(std::memory_order_relaxed));
			State.store(Pack(gen, 0), std::memory_order_release);
			return gen;
		}

        // Bump the generation and close, only if there are no references.
        // Works whether the slot is closed or open with count 0 (e.g. never acquired).
        // Fails if someone re-acquired during the frame delay, so a live object isn't killed.
        bool TryRetire(uint16_t gen)
        {
            uint32_t cur = State.load(std::memory_order_acquire);
            for (;;)
            {
                if (GenOf(cur) != gen || (LowOf(cur) & COUNT_MASK) != 0)
                {
                    return false;
                }

                const uint32_t next = Pack(NextGen(gen), CLOSING);
                if (State.compare_exchange_weak(cur, next, std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    return true;
                }
            }
        }

		// Acquire a reference. Fails on generation mismatch, CLOSING, or count overflow.
		bool TryAddRef(uint16_t gen)
		{
			uint32_t cur = State.load(std::memory_order_relaxed);
			for (;;)
			{
				if (GenOf(cur) != gen)
				{
					return false;
				}

				const uint32_t low = LowOf(cur);
				if ((low & CLOSING) || (low & COUNT_MASK) == COUNT_MASK)
				{
					return false;
				}

				// +1 cannot carry into CLOSING because count < COUNT_MASK here.
				if (State.compare_exchange_weak(cur, cur + 1, std::memory_order_acquire, std::memory_order_relaxed))
				{
					return true;
				}
			}
		}

		// Release a reference. Returns true if this call dropped the last reference (count 1 -> CLOSING in a single CAS).
		// Fails (false) on generation mismatch, double release, or underflow.
        bool TryReleaseRef(uint16_t gen)
        {
            uint32_t cur = State.load(std::memory_order_relaxed);
            for (;;)
            {
                if (GenOf(cur) != gen)
                {
                    return false;
                }

                const uint32_t low = LowOf(cur);
                if (low & CLOSING)
                {
                    return false; // double release
                }

                const uint32_t cnt = low & COUNT_MASK;
                if (cnt == 0)
                {
                    return false; // underflow
                }

                const bool last = (cnt == 1);
                const uint32_t next = last ? Pack(gen, CLOSING) : (cur - 1);

                if (State.compare_exchange_weak(cur, next, std::memory_order_acq_rel, std::memory_order_relaxed))
                {
                    return last;
                }
            }
        }
        
        // Reopen a closed slot without freeing it: (gen, CLOSING, 0) -> (gen, open, 0).
        // Fails if the generation differs, the slot isn't closed, or it still has refs.
        bool TryReopen(uint16_t gen)
        {
            uint32_t cur = State.load(std::memory_order_acquire);
            for (;;)
            {
                if (GenOf(cur) != gen)
                {
                    return false;
                }

                const uint32_t low = LowOf(cur);
                if (!(low & CLOSING) || (low & COUNT_MASK) != 0)
                {
                    return false;
                }

                if (State.compare_exchange_weak(cur, Pack(gen, 0), std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    return true;
                }
            }
        }

		uint16_t Generation() const
		{
			return GenOf(State.load(std::memory_order_acquire));
		}

		bool Matches(uint16_t gen) const
		{
			return Generation() == gen;
		}

		bool IsAlive(uint16_t gen) const
		{
			const uint32_t s = State.load(std::memory_order_acquire);
			const uint32_t low = LowOf(s);
			return GenOf(s) == gen && !(low & CLOSING) && (low & COUNT_MASK) >= 1;
		}

		bool IsClosing(uint16_t gen) const
		{
			const uint32_t s = State.load(std::memory_order_acquire);
			const uint32_t low = LowOf(s);
			return GenOf(s) != gen || (low & CLOSING) || (low & COUNT_MASK) == COUNT_MASK;
		}
	};
}
