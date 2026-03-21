#pragma once

/**
 * TwoLevelBitset.h
 *
 * Two-level bitfield as described by Sebastian Aaltonen (ex-Ubisoft / Unity).
 * MAX_ENTITIES is configured once at construction time and never changes.
 *
 * ┌────────────────────────────────────────────────────────────────────────┐
 * │  TOP LEVEL  (L0)                                                       │
 * │  Each 64-bit word summarises 64 L1 words (= 4096 entities).            │
 * │  Bit j of L0[i] is set  iff  L1[i*64 + j] != 0.                        │
 * │  A single 64-byte cache line of L0 covers ≈21 845 entities.            │
 * ├────────────────────────────────────────────────────────────────────────┤
 * │  2nd LEVEL  (L1)                                                       │
 * │  Each 64-bit word holds the actual presence bits for 64 entities.      │
 * └────────────────────────────────────────────────────────────────────────┘
 *
 * Iteration:
 *   forEach            – tzcnt loop, skips empty 4096-entity blocks.
 *   forEachN (static)  – compile-time-variadic AND + tzcnt inner join.
 *                        Pass any number of bitsets of the same capacity.
 *   forEachDynamic     – runtime std::span<> variant for fully dynamic queries.
 *
 * Requirements: C++20 (<bit>, <span>)
 */

#include "Core/Allocators.h"
#include "Utilities/Allocators/Arena.h"

#include <cstdint>
#include <cstring>
#include <cassert>
#include <memory>
#include <span>
#include <bit>
#include <limits>
#include <stdexcept>

namespace HBL2
{
    class TwoLevelBitset
    {
    public:
        static constexpr uint32_t INVALID = std::numeric_limits<uint32_t>::max();

        TwoLevelBitset() = default;

        explicit TwoLevelBitset(uint32_t maxEntities, PoolReservation* reservation)
        {
            Initialize(maxEntities, reservation);
        }

        void Initialize(uint32_t maxEntities, PoolReservation* reservation)
        {
            if (maxEntities == 0 || maxEntities % 4096 != 0)
            {
                throw std::invalid_argument("TwoLevelBitset: max_entities must be a non-zero multiple of 4096");
            }

            m_Max = maxEntities;
            m_L1Count = maxEntities / 64;
            m_L0Count = maxEntities / 4096;

            size_t bytes = (m_L0Count * sizeof(uint64_t) + 63) + (m_L1Count * sizeof(uint64_t) + 63);

            m_Arena.Initialize(&Allocator::Arena, bytes, reservation);

            uint8_t* l0Raw = (uint8_t*)m_Arena.Alloc(m_L0Count * sizeof(uint64_t) + 63);
            uint8_t* l1Raw = (uint8_t*)m_Arena.Alloc(m_L1Count * sizeof(uint64_t) + 63);

            m_L0 = Align64<uint64_t>(l0Raw);
            m_L1 = Align64<uint64_t>(l1Raw);

            reset();
        }

        TwoLevelBitset(const TwoLevelBitset&) = delete;
        TwoLevelBitset& operator=(const TwoLevelBitset&) = delete;
        TwoLevelBitset(TwoLevelBitset&&) = default;
        TwoLevelBitset& operator=(TwoLevelBitset&&) = default;

        [[nodiscard]] uint32_t capacity() const { return m_Max; }

        inline void set(uint32_t key)
        {
            assert(key < m_Max);
            const uint32_t w = key >> 6, b = key & 63;
            m_L1[w] |= 1ULL << b;
            m_L0[w >> 6] |= 1ULL << (w & 63);
        }

        inline void clear(uint32_t key)
        {
            assert(key < m_Max);
            const uint32_t w = key >> 6, b = key & 63;
            m_L1[w] &= ~(1ULL << b);
            if (m_L1[w] == 0)
            {
                m_L0[w >> 6] &= ~(1ULL << (w & 63));
            }
        }

        inline void reset()
        {
            memset(m_L0, 0, m_L0Count * sizeof(uint64_t));
            memset(m_L1, 0, m_L1Count * sizeof(uint64_t));
        }

        [[nodiscard]] inline bool test(uint32_t key) const
        {
            assert(key < m_Max);
            return (m_L1[key >> 6] >> (key & 63)) & 1;
        }

        void destroy()
        {
            reset();
            m_Arena.Destroy();
        }

        template<typename Func>
        void forEach(Func&& func) const
        {
            for (uint32_t i = 0; i < m_L0Count; ++i)
            {
                uint64_t l0w = m_L0[i];
                while (l0w)
                {
                    const uint32_t j = std::countr_zero(l0w);
                    l0w &= l0w - 1;
                    const uint32_t base = (i << 6) | j;
                    uint64_t l1w = m_L1[base];

                    while (l1w)
                    {
                        const uint32_t k = std::countr_zero(l1w);
                        l1w &= l1w - 1;
                        func((base << 6) | k);
                    }
                }
            }
        }

        template<typename Func, typename... Rest>
        static void forEachN(Func&& func, const TwoLevelBitset& first, const Rest&... rest)
        {
            static_assert((std::is_same_v<Rest, TwoLevelBitset> && ...), "forEachN: all arguments must be TwoLevelBitset");

            for (uint32_t i = 0; i < first.m_L0Count; ++i)
            {
                uint64_t l0w = first.m_L0[i];
                ((l0w &= rest.m_L0[i]), ...);    // compile-time AND fold

                while (l0w)
                {
                    const uint32_t j = std::countr_zero(l0w);
                    l0w &= l0w - 1;
                    const uint32_t base = (i << 6) | j;
                    uint64_t l1w = first.m_L1[base];
                    ((l1w &= rest.m_L1[base]), ...);

                    while (l1w)
                    {
                        const uint32_t k = std::countr_zero(l1w);
                        l1w &= l1w - 1;
                        func((base << 6) | k);
                    }
                }
            }
        }

        template<typename Func>
        static void forEachDynamic(std::span<const TwoLevelBitset* const> bitsets, Func&& func)
        {
            if (bitsets.empty())
            {
                return;
            }

            const TwoLevelBitset& first = *bitsets[0];

            for (uint32_t i = 0; i < first.m_L0Count; ++i)
            {
                uint64_t l0w = first.m_L0[i];
                for (size_t s = 1; s < bitsets.size(); ++s)
                {
                    l0w &= bitsets[s]->m_L0[i];
                }

                while (l0w)
                {
                    const uint32_t j = std::countr_zero(l0w);
                    l0w &= l0w - 1;
                    const uint32_t base = (i << 6) | j;
                    uint64_t l1w = first.m_L1[base];

                    for (size_t s = 1; s < bitsets.size(); ++s)
                    {
                        l1w &= bitsets[s]->m_L1[base];
                    }

                    while (l1w)
                    {
                        const uint32_t k = std::countr_zero(l1w);
                        l1w &= l1w - 1;
                        func((base << 6) | k);
                    }
                }
            }
        }

    private:
        uint32_t m_Max = 0;
        uint32_t m_L1Count = 0;
        uint32_t m_L0Count = 0;

        uint64_t* m_L0 = nullptr;
        uint64_t* m_L1 = nullptr;

        Arena m_Arena;

        template<typename T>
        static T* Align64(uint8_t* p)
        {
            uintptr_t addr = reinterpret_cast<uintptr_t>(p);
            addr = (addr + 63) & ~uintptr_t(63);
            return reinterpret_cast<T*>(addr);
        }
    };
}