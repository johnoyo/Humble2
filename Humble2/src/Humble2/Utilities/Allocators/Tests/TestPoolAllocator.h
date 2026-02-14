#pragma once

// PoolArena_GlobalArena_Tests.cpp
//
// Correctness-oriented tests for PoolArena (lock-free fixed-block allocator)
// Style matches your TLSF tests: simple macros, bool tests, RUN_TEST runner.
//
// Focus:
//  - Basic alloc/free
//  - Alignment correctness
//  - Exhaustion (OOM) + recovery after frees
//  - Reuse behavior (same blocks come back)
//  - Pattern/canary integrity (no overwrite across allocations)
//  - Multi-thread smoke test (MPMC), including cross-thread frees
//  - Strict error cases: oversize request, too-large alignment
//  - Stress test with random alloc/free within block constraints
//
// IMPORTANT:
//  - PoolArena is fixed-block: Alloc(size) must be <= blockSize, align <= poolBlockAlign.
//  - PoolArena::Alloc throws std::bad_alloc on exhaustion or invalid request.
//  - PoolArena::Free asserts on invalid pointers (pointer not from pool / misaligned).
//

#include "Utilities/Allocators/Arena.h"

#include <vector>
#include <random>
#include <chrono>
#include <iostream>
#include <thread>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <exception>

using namespace HBL2;

// ----------------------------
// Test framework macros
// ----------------------------
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cout << "FAILED: " << #condition << " at line " << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        std::cout << "Running " << #test_func << "... "; \
        if (test_func()) { \
            std::cout << "PASSED" << std::endl; \
        } else { \
            std::cout << "FAILED" << std::endl; \
            return 1; \
        } \
    } while(0)

static inline bool is_aligned(void* p, size_t a)
{
    return (reinterpret_cast<uintptr_t>(p) & (a - 1)) == 0;
}

static inline void fill_pattern(uint8_t* p, size_t n, uint8_t seed)
{
    for (size_t i = 0; i < n; ++i)
        p[i] = (uint8_t)(seed + (uint8_t)i);
}

static inline bool check_pattern(const uint8_t* p, size_t n, uint8_t seed)
{
    for (size_t i = 0; i < n; ++i)
        if (p[i] != (uint8_t)(seed + (uint8_t)i))
            return false;
    return true;
}

struct CanaryBlock
{
    void* Ptr = nullptr;
    size_t  Size = 0;
    uint8_t Seed = 0;
};

// Helper: create MainArena + PoolArena with optional reservation
static inline void make_arena_and_pool(
    MainArena& g,
    PoolReservation*& res,
    PoolArena& pool,
    size_t totalBytes,
    size_t metaBytes,
    size_t poolBytes,
    size_t blockSize,
    bool useReservation = true)
{
    g.Initialize(totalBytes, metaBytes);

    if (useReservation)
    {
        res = g.Reserve("PoolArenaHeap", poolBytes);
        pool.Initialize(&g, poolBytes, blockSize, res);
    }
    else
    {
        res = nullptr;
        pool.Initialize(&g, poolBytes, blockSize, nullptr);
    }
}

static inline uint32_t ptr_to_pool_index(const PoolArena& pool, void* p)
{
#ifdef ARENA_DEBUG
    uint8_t* base = pool.DebugBase();
    size_t total = pool.DebugTotalBytes();
    size_t bs = pool.DebugBlockSize();

    uint8_t* up = static_cast<uint8_t*>(p);
    if (up < base || up >= base + total) return 0xFFFFFFFFu;

    size_t off = (size_t)(up - base);
    if ((off % bs) != 0) return 0xFFFFFFFFu;

    uint32_t idx = (uint32_t)(off / bs);
    if (idx >= pool.DebugBlockCount()) return 0xFFFFFFFFu;

    return idx;
#else
    (void)pool; (void)p;
    return 0xFFFFFFFFu;
#endif
}


// -------------------------------------
// Tests
// -------------------------------------

bool test_basic_allocation_and_free()
{
    MainArena g;
    PoolReservation* res = nullptr;
    PoolArena pool;

    make_arena_and_pool(g, res, pool,
        8 * 1024 * 1024, 512 * 1024,
        1 * 1024 * 1024, 256);

    void* p1 = pool.Alloc(64, 16);
    TEST_ASSERT(p1 != nullptr);
    TEST_ASSERT(is_aligned(p1, 16));

    void* p2 = pool.Alloc(128, 8);
    TEST_ASSERT(p2 != nullptr);
    TEST_ASSERT(p2 != p1);
    TEST_ASSERT(is_aligned(p2, 8));

    // Write/read
    std::memset(p1, 0xAB, 64);
    std::memset(p2, 0xCD, 128);
    TEST_ASSERT(((uint8_t*)p1)[0] == 0xAB);
    TEST_ASSERT(((uint8_t*)p2)[0] == 0xCD);

    pool.Free(p2);
    pool.Free(p1);

    // Allocate again; should succeed
    void* p3 = pool.Alloc(32, 8);
    TEST_ASSERT(p3 != nullptr);
    pool.Free(p3);

    return true;
}

bool test_alignment_various()
{
    MainArena g;
    PoolReservation* res = nullptr;
    PoolArena pool;

    // block size aligned up to alignof(max_align_t); typically 16
    make_arena_and_pool(g, res, pool,
        8 * 1024 * 1024, 512 * 1024,
        1 * 1024 * 1024, 256);

    // alignment <= alignof(max_align_t) should work
    void* p8 = pool.Alloc(17, 8);
    void* p16 = pool.Alloc(33, 16);

    TEST_ASSERT(p8 && p16);
    TEST_ASSERT(is_aligned(p8, 8));
    TEST_ASSERT(is_aligned(p16, 16));

    pool.Free(p8);
    pool.Free(p16);

    return true;
}

bool test_strict_invalid_requests()
{
    MainArena g;
    PoolReservation* res = nullptr;
    PoolArena pool;

    make_arena_and_pool(g, res, pool,
        8 * 1024 * 1024, 512 * 1024,
        1 * 1024 * 1024, 128);

    // Oversize request must throw
    bool threwOversize = false;
    try
    {
        (void)pool.Alloc(129, 8);
    }
    catch (const std::bad_alloc&)
    {
        threwOversize = true;
    }
    catch (...)
    {
        TEST_ASSERT(false);
    }
    TEST_ASSERT(threwOversize);

    // Too-large alignment must throw (PoolArena guarantees up to alignof(max_align_t))
    bool threwAlign = false;
    try
    {
        (void)pool.Alloc(64, 64); // likely > max_align_t on most platforms
    }
    catch (const std::bad_alloc&)
    {
        threwAlign = true;
    }
    catch (...)
    {
        TEST_ASSERT(false);
    }
    TEST_ASSERT(threwAlign);

    return true;
}

bool test_exhaustion_and_recovery()
{
    MainArena g;
    PoolReservation* res = nullptr;
    PoolArena pool;

    // Small pool: 64 KiB, blocks 256 => 256 blocks
    const size_t poolBytes = 64 * 1024;
    const size_t blockSize = 256;

    make_arena_and_pool(g, res, pool,
        4 * 1024 * 1024, 256 * 1024,
        poolBytes, blockSize);

    std::vector<void*> ptrs;
    ptrs.reserve(1024);

    // Allocate until exhausted (expect throw)
    bool gotThrow = false;
    while (true)
    {
        try
        {
            void* p = pool.Alloc(128, 16);
            ptrs.push_back(p);
        }
        catch (const std::bad_alloc&)
        {
            gotThrow = true;
            break;
        }
    }

    TEST_ASSERT(gotThrow);
    TEST_ASSERT(!ptrs.empty());

    // Free half
    for (size_t i = 0; i < ptrs.size(); i += 2)
    {
        pool.Free(ptrs[i]);
        ptrs[i] = nullptr;
    }

    // Now allocations should succeed again
    for (int i = 0; i < 64; ++i)
    {
        void* p = pool.Alloc(64, 8);
        TEST_ASSERT(p != nullptr);
        pool.Free(p);
    }

    // Cleanup remaining
    for (void* p : ptrs)
        if (p) pool.Free(p);

    return true;
}

bool test_reuse_behavior_lifoish()
{
    MainArena g;
    PoolReservation* res = nullptr;
    PoolArena pool;

    make_arena_and_pool(g, res, pool,
        8 * 1024 * 1024, 512 * 1024,
        256 * 1024, 128);

    // Allocate a few blocks
    void* a = pool.Alloc(64, 16);
    void* b = pool.Alloc(64, 16);
    void* c = pool.Alloc(64, 16);

    TEST_ASSERT(a && b && c);
    TEST_ASSERT(a != b && b != c && a != c);

    // Free in order b, c
    pool.Free(b);
    pool.Free(c);

    // Treiber stack is LIFO, so next alloc should usually return c (then b)
    void* x = pool.Alloc(64, 16);
    void* y = pool.Alloc(64, 16);

    TEST_ASSERT(x && y);
    TEST_ASSERT(x == c); // "usually" but for single-thread it should be deterministic
    TEST_ASSERT(y == b);

    pool.Free(x);
    pool.Free(y);
    pool.Free(a);

    return true;
}

bool test_canary_integrity_single_thread()
{
    MainArena g;
    PoolReservation* res = nullptr;
    PoolArena pool;

    make_arena_and_pool(g, res, pool,
        16 * 1024 * 1024, 512 * 1024,
        2 * 1024 * 1024, 256);

    std::vector<CanaryBlock> live;
    live.reserve(4096);

    // Allocate a bunch
    for (int i = 0; i < 2048; ++i)
    {
        void* p = pool.Alloc(200, 16);
        TEST_ASSERT(p != nullptr);
        TEST_ASSERT(is_aligned(p, 16));

        CanaryBlock b;
        b.Ptr = p;
        b.Size = 200;
        b.Seed = (uint8_t)(i & 0xFF);
        fill_pattern((uint8_t*)p, b.Size, b.Seed);
        live.push_back(b);
    }

    // Verify all
    for (auto& b : live)
        TEST_ASSERT(check_pattern((const uint8_t*)b.Ptr, b.Size, b.Seed));

    // Free half, verify the rest remain intact
    for (size_t i = 0; i < live.size(); i += 2)
    {
        pool.Free(live[i].Ptr);
        live[i].Ptr = nullptr;
    }

    for (size_t i = 1; i < live.size(); i += 2)
        TEST_ASSERT(check_pattern((const uint8_t*)live[i].Ptr, live[i].Size, live[i].Seed));

    // Cleanup
    for (auto& b : live)
        if (b.Ptr) pool.Free(b.Ptr);

    return true;
}

bool test_randomized_stress_canaries()
{
    MainArena g;
    PoolReservation* res = nullptr;
    PoolArena pool;

    make_arena_and_pool(g, res, pool,
        32 * 1024 * 1024, 1 * 1024 * 1024,
        8 * 1024 * 1024, 256);

    std::mt19937 rng(0x1234567u);
    std::uniform_int_distribution<int> sizeDist(1, 256);
    std::uniform_int_distribution<int> alignDist(0, 1); // 8..16
    std::uniform_int_distribution<int> opDist(0, 99);

    auto pick_align = [&](int k) -> size_t
        {
            static const size_t aligns[] = { 8, 16 };
            return aligns[std::clamp(k, 0, 1)];
        };

    std::vector<CanaryBlock> live;
    live.reserve(20000);

    const int OPS = 200000;

    for (int i = 0; i < OPS; ++i)
    {
        int op = opDist(rng);

        if (op < 60) // allocate
        {
            size_t sz = (size_t)sizeDist(rng);
            size_t al = pick_align(alignDist(rng));

            void* p = nullptr;
            try
            {
                p = pool.Alloc(sz, al);
            }
            catch (const std::bad_alloc&)
            {
                // If exhausted, free a batch and continue
                if (!live.empty())
                {
                    size_t k = std::min<size_t>(live.size(), 256);
                    for (size_t j = 0; j < k; ++j)
                    {
                        size_t idx = (size_t)(rng() % live.size());
                        TEST_ASSERT(check_pattern((const uint8_t*)live[idx].Ptr, live[idx].Size, live[idx].Seed));
                        pool.Free(live[idx].Ptr);
                        live[idx] = live.back();
                        live.pop_back();
                    }
                    continue;
                }
                TEST_ASSERT(false);
            }

            TEST_ASSERT(p != nullptr);
            TEST_ASSERT(is_aligned(p, al));

            CanaryBlock b;
            b.Ptr = p;
            b.Size = sz;
            b.Seed = (uint8_t)(rng() & 0xFF);
            fill_pattern((uint8_t*)p, sz, b.Seed);
            live.push_back(b);
        }
        else // free
        {
            if (live.empty()) continue;
            size_t idx = (size_t)(rng() % live.size());

            TEST_ASSERT(check_pattern((const uint8_t*)live[idx].Ptr, live[idx].Size, live[idx].Seed));
            pool.Free(live[idx].Ptr);

            live[idx] = live.back();
            live.pop_back();
        }

        // Periodic spot-check
        if ((i % 5000) == 0)
        {
            for (int k = 0; k < 8 && !live.empty(); ++k)
            {
                size_t idx = (size_t)(rng() % live.size());
                TEST_ASSERT(check_pattern((const uint8_t*)live[idx].Ptr, live[idx].Size, live[idx].Seed));
            }
        }
    }

    for (auto& b : live)
    {
        TEST_ASSERT(check_pattern((const uint8_t*)b.Ptr, b.Size, b.Seed));
        pool.Free(b.Ptr);
    }

    return true;
}

bool test_multithread_smoke_and_cross_thread_free()
{
    MainArena g;
    PoolReservation* res = nullptr;
    PoolArena pool;

    // Keep 32 MiB if you want; bounded-live means no guaranteed OOM.
    make_arena_and_pool(g, res, pool,
        128 * 1024 * 1024, 2 * 1024 * 1024,
        32 * 1024 * 1024, 256);

    constexpr int THREADS = 8;
    constexpr int OPS_PER_THREAD = 200000;
    constexpr int MAX_LIVE_PER_THREAD = 2048; // bounded peak live blocks

    std::vector<std::vector<CanaryBlock>> survivors(THREADS);
    for (int t = 0; t < THREADS; ++t) survivors[t].reserve(MAX_LIVE_PER_THREAD);

    std::atomic<bool> ok{ true };
    std::atomic<int> ready{ 0 };
    std::atomic<bool> go{ false };

    auto worker = [&](int tid)
    {
        try
        {
            std::mt19937 rng((uint32_t)(0xBEEF + tid));
            std::uniform_int_distribution<int> sizeDist(1, 256);
            std::uniform_int_distribution<int> alignDist(0, 1); // 8..16
            std::uniform_int_distribution<int> opDist(0, 99);

            auto pick_align = [&](int k) -> size_t
            {
                static const size_t a[] = { 8, 16 };
                return a[std::clamp(k, 0, 1)];
            };

            std::vector<CanaryBlock> live;
            live.reserve(MAX_LIVE_PER_THREAD);

            ready.fetch_add(1, std::memory_order_relaxed);
            while (!go.load(std::memory_order_acquire)) {}

            for (int i = 0; i < OPS_PER_THREAD; ++i)
            {
                int op = opDist(rng);

                // Bias toward allocate if live is small; bias toward free if live is large
                if (live.empty()) op = 0;
                if ((int)live.size() >= MAX_LIVE_PER_THREAD) op = 99;

                if (op < 60) // allocate
                {
                    size_t sz = (size_t)sizeDist(rng);
                    size_t al = pick_align(alignDist(rng));

                    void* p = nullptr;
                    try
                    {
                        p = pool.Alloc(sz, al);
                    }
                    catch (const std::bad_alloc&)
                    {
                        // Under contention we allow temporary exhaustion; just retry later
                        continue;
                    }

                    if (!p || !is_aligned(p, al))
                    {
                        ok.store(false, std::memory_order_relaxed);
                        break;
                    }

                    CanaryBlock b;
                    b.Ptr = p;
                    b.Size = sz;
                    b.Seed = (uint8_t)(tid ^ (i & 0xFF));
                    fill_pattern((uint8_t*)p, sz, b.Seed);
                    live.push_back(b);
                }
                else // free
                {
                    if (live.empty())
                        continue;

                    size_t idx = (size_t)(rng() % live.size());
                    if (!check_pattern((const uint8_t*)live[idx].Ptr, live[idx].Size, live[idx].Seed))
                    {
                        ok.store(false, std::memory_order_relaxed);
                        break;
                    }

                    pool.Free(live[idx].Ptr);
                    live[idx] = live.back();
                    live.pop_back();
                }

                // periodic spot-check
                if ((i % 5000) == 0 && !live.empty())
                {
                    size_t idx = (size_t)(rng() % live.size());
                    if (!check_pattern((const uint8_t*)live[idx].Ptr, live[idx].Size, live[idx].Seed))
                    {
                        ok.store(false, std::memory_order_relaxed);
                        break;
                    }
                }
            }

            // Keep remaining live blocks for cross-thread free by main thread
            survivors[tid].swap(live);
        }
        catch (...)
        {
            ok.store(false, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t)
        threads.emplace_back(worker, t);

    while (ready.load(std::memory_order_acquire) != THREADS) {}
    go.store(true, std::memory_order_release);

    for (auto& th : threads) th.join();

    TEST_ASSERT(ok.load(std::memory_order_acquire));

    // Cross-thread free: main thread frees all survivors
    for (int t = 0; t < THREADS; ++t)
    {
        for (auto& b : survivors[t])
        {
            TEST_ASSERT(check_pattern((const uint8_t*)b.Ptr, b.Size, b.Seed));
            pool.Free(b.Ptr);
        }
        survivors[t].clear();
    }

    // Still works
    for (int i = 0; i < 50000; ++i)
    {
        void* p = pool.Alloc(64, 16);
        TEST_ASSERT(p != nullptr);
        pool.Free(p);
    }

    return true;
}


bool test_performance_sanity()
{
    MainArena g;
    PoolReservation* res = nullptr;
    PoolArena pool;

    make_arena_and_pool(g, res, pool,
        512 * 1024 * 1024, 8 * 1024 * 1024,
        128 * 1024 * 1024, 256);

    const int N = 500000;
    std::vector<void*> ptrs;
    ptrs.resize(N);

    std::mt19937 rng(123);
    std::uniform_int_distribution<int> sizeDist(1, 256);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < N; ++i)
    {
        size_t sz = (size_t)sizeDist(rng);
        ptrs[i] = pool.Alloc(sz, 16);
        TEST_ASSERT(ptrs[i] != nullptr);
    }

    auto mid = std::chrono::high_resolution_clock::now();

    // Free in reverse order
    for (int i = N - 1; i >= 0; --i)
        pool.Free(ptrs[i]);

    auto end = std::chrono::high_resolution_clock::now();

    auto allocTime = std::chrono::duration_cast<std::chrono::microseconds>(mid - start);
    auto freeTime = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);

    std::cout << "\n  Allocation time: " << allocTime.count() << " μs";
    std::cout << "\n  Deallocation time: " << freeTime.count() << " μs";

    return true;
}

bool test_multithread_ownership_bitmap_stress()
{
    #ifndef ARENA_DEBUG
    std::cout << "\n  (skipped: requires ARENA_DEBUG for DebugBase/DebugBlockSize)" << std::endl;
    return true;
    #else
    MainArena g;
    PoolReservation* res = nullptr;
    PoolArena pool;

    // Use a small-ish pool to amplify reuse + contention
    make_arena_and_pool(g, res, pool,
        128 * 1024 * 1024, 2 * 1024 * 1024,
        16 * 1024 * 1024, 256);

    const uint32_t BLOCKS = pool.DebugBlockCount();
    TEST_ASSERT(BLOCKS > 0);

    // Ownership bitmap: 0 = free/unowned, 1 = currently owned (allocated, not yet freed)
    std::vector<std::atomic<uint8_t>> owned(BLOCKS);
    for (uint32_t i = 0; i < BLOCKS; ++i) owned[i].store(0, std::memory_order_relaxed);

    constexpr int THREADS = 8;
    constexpr int OPS_PER_THREAD = 300000;
    constexpr int MAX_LIVE_PER_THREAD = 1024;

    std::atomic<bool> ok{ true };
    std::atomic<int> ready{ 0 };
    std::atomic<bool> go{ false };

    auto worker = [&](int tid)
        {
            try
            {
                std::mt19937 rng((uint32_t)(0xC0FFEEu + tid));
                std::uniform_int_distribution<int> sizeDist(1, 256);
                std::uniform_int_distribution<int> alignDist(0, 1); // 8..16
                std::uniform_int_distribution<int> opDist(0, 99);

                auto pick_align = [&](int k) -> size_t
                    {
                        static const size_t a[] = { 8, 16 };
                        return a[std::clamp(k, 0, 1)];
                    };

                std::vector<CanaryBlock> live;
                live.reserve(MAX_LIVE_PER_THREAD);

                ready.fetch_add(1, std::memory_order_relaxed);
                while (!go.load(std::memory_order_acquire)) {}

                for (int i = 0; i < OPS_PER_THREAD; ++i)
                {
                    int op = opDist(rng);
                    if (live.empty()) op = 0;
                    if ((int)live.size() >= MAX_LIVE_PER_THREAD) op = 99;

                    if (op < 60) // alloc
                    {
                        size_t sz = (size_t)sizeDist(rng);
                        size_t al = pick_align(alignDist(rng));

                        void* p = nullptr;
                        try
                        {
                            p = pool.Alloc(sz, al);
                        }
                        catch (const std::bad_alloc&)
                        {
                            // allowed under contention: just skip
                            continue;
                        }

                        if (!p || !is_aligned(p, al)) { ok.store(false); break; }

                        uint32_t idx = ptr_to_pool_index(pool, p);
                        if (idx == 0xFFFFFFFFu) { ok.store(false); break; }

                        // Ownership must transition 0->1 exactly once
                        uint8_t prev = owned[idx].exchange(1, std::memory_order_acq_rel);
                        if (prev != 0) { ok.store(false); break; }

                        CanaryBlock b;
                        b.Ptr = p;
                        b.Size = sz;
                        b.Seed = (uint8_t)(tid ^ (i & 0xFF));
                        fill_pattern((uint8_t*)p, sz, b.Seed);
                        live.push_back(b);
                    }
                    else // free
                    {
                        if (live.empty()) continue;

                        size_t k = (size_t)(rng() % live.size());

                        if (!check_pattern((const uint8_t*)live[k].Ptr, live[k].Size, live[k].Seed))
                        {
                            ok.store(false);
                            break;
                        }

                        uint32_t idx = ptr_to_pool_index(pool, live[k].Ptr);
                        if (idx == 0xFFFFFFFFu) { ok.store(false); break; }

                        // Ownership must transition 1->0 exactly once
                        uint8_t prev = owned[idx].exchange(0, std::memory_order_acq_rel);
                        if (prev != 1) { ok.store(false); break; }

                        pool.Free(live[k].Ptr);
                        live[k] = live.back();
                        live.pop_back();
                    }

                    // occasional spot check
                    if ((i % 10000) == 0 && !live.empty())
                    {
                        size_t k = (size_t)(rng() % live.size());
                        if (!check_pattern((const uint8_t*)live[k].Ptr, live[k].Size, live[k].Seed))
                        {
                            ok.store(false);
                            break;
                        }
                    }
                }

                // free leftovers
                for (auto& b : live)
                {
                    uint32_t idx = ptr_to_pool_index(pool, b.Ptr);
                    if (idx == 0xFFFFFFFFu) { ok.store(false); break; }

                    if (!check_pattern((const uint8_t*)b.Ptr, b.Size, b.Seed))
                    {
                        ok.store(false);
                        break;
                    }

                    uint8_t prev = owned[idx].exchange(0, std::memory_order_acq_rel);
                    if (prev != 1) { ok.store(false); break; }

                    pool.Free(b.Ptr);
                }
            }
            catch (...)
            {
                ok.store(false);
            }
        };

    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t)
        threads.emplace_back(worker, t);

    while (ready.load(std::memory_order_acquire) != THREADS) {}
    go.store(true, std::memory_order_release);

    for (auto& th : threads) th.join();

    TEST_ASSERT(ok.load(std::memory_order_acquire));

    // At end, all blocks should be unowned (0)
    for (uint32_t i = 0; i < BLOCKS; ++i)
        TEST_ASSERT(owned[i].load(std::memory_order_acquire) == 0);

    return true;
    #endif
}

bool test_multithread_exhaust_refill_uniqueness()
{
    #ifndef ARENA_DEBUG
    std::cout << "\n  (skipped: requires ARENA_DEBUG for DebugBase/DebugBlockSize)" << std::endl;
    return true;
    #else
    MainArena g;
    PoolReservation* res = nullptr;
    PoolArena pool;

    // Make pool moderate; we will fill it completely.
    make_arena_and_pool(g, res, pool,
        128 * 1024 * 1024, 2 * 1024 * 1024,
        16 * 1024 * 1024, 256);

    const uint32_t BLOCKS = pool.DebugBlockCount();
    TEST_ASSERT(BLOCKS > 1024);

    // We'll collect *all* successful allocations across threads until pool is exhausted.
    // To avoid lock contention, each thread writes to its own vector, then we merge.
    constexpr int THREADS = 8;

    std::vector<std::vector<void*>> perThread(THREADS);
    for (int t = 0; t < THREADS; ++t) perThread[t].reserve((size_t)BLOCKS / THREADS + 1024);

    std::atomic<bool> ok{ true };
    std::atomic<int> ready{ 0 };
    std::atomic<bool> go{ false };

    // Phase 1: exhaust pool
    auto worker_exhaust = [&](int tid)
        {
            try
            {
                ready.fetch_add(1, std::memory_order_relaxed);
                while (!go.load(std::memory_order_acquire)) {}

                while (true)
                {
                    void* p = nullptr;
                    try
                    {
                        p = pool.Alloc(64, 16);
                    }
                    catch (const std::bad_alloc&)
                    {
                        break; // exhausted
                    }
                    catch (...)
                    {
                        ok.store(false);
                        break;
                    }

                    if (!p || !is_aligned(p, 16))
                    {
                        ok.store(false);
                        break;
                    }

                    perThread[tid].push_back(p);
                }
            }
            catch (...)
            {
                ok.store(false);
            }
        };

    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t)
        threads.emplace_back(worker_exhaust, t);

    while (ready.load(std::memory_order_acquire) != THREADS) {}
    go.store(true, std::memory_order_release);
    for (auto& th : threads) th.join();

    TEST_ASSERT(ok.load(std::memory_order_acquire));

    // Merge all pointers
    std::vector<void*> all;
    all.reserve((size_t)BLOCKS + 1024);
    for (int t = 0; t < THREADS; ++t)
        all.insert(all.end(), perThread[t].begin(), perThread[t].end());

    // We expect to have allocated exactly BLOCKS blocks (full exhaustion).
    // Under contention, it's possible we stop slightly short only if a thread hit an unexpected path,
    // but with strict Alloc throwing only on exhaustion, we should hit exactly BLOCKS.
    TEST_ASSERT(all.size() == (size_t)BLOCKS);

    // Uniqueness check: every block index appears exactly once.
    std::vector<uint8_t> seen(BLOCKS, 0);
    for (void* p : all)
    {
        uint32_t idx = ptr_to_pool_index(pool, p);
        TEST_ASSERT(idx != 0xFFFFFFFFu);
        TEST_ASSERT(seen[idx] == 0);
        seen[idx] = 1;
    }

    // Phase 2: free all blocks (single-thread is fine; we are validating recovery)
    for (void* p : all)
        pool.Free(p);

    // Phase 3: refill fully again, ensure we can allocate BLOCKS blocks again (no lost nodes)
    std::vector<void*> refill;
    refill.reserve(BLOCKS);

    while (true)
    {
        try
        {
            void* p = pool.Alloc(64, 16);
            refill.push_back(p);
        }
        catch (const std::bad_alloc&)
        {
            break;
        }
    }

    TEST_ASSERT(refill.size() == (size_t)BLOCKS);

    // Another uniqueness check on refill set
    std::fill(seen.begin(), seen.end(), 0);
    for (void* p : refill)
    {
        uint32_t idx = ptr_to_pool_index(pool, p);
        TEST_ASSERT(idx != 0xFFFFFFFFu);
        TEST_ASSERT(seen[idx] == 0);
        seen[idx] = 1;
    }

    // Cleanup
    for (void* p : refill)
        pool.Free(p);

    // Final: should be able to allocate a few blocks again
    for (int i = 0; i < 1024; ++i)
    {
        void* p = pool.Alloc(64, 16);
        TEST_ASSERT(p != nullptr);
        pool.Free(p);
    }

    return true;
    #endif
}

// -------------------------------------
// Runner
// -------------------------------------
inline int TestPoolArena_GlobalArena()
{
    std::cout << "Running PoolArena (GlobalArena-backed, lock-free free-list) Tests\n" << std::endl;

    RUN_TEST(test_basic_allocation_and_free);
    RUN_TEST(test_alignment_various);
    RUN_TEST(test_strict_invalid_requests);
    RUN_TEST(test_exhaustion_and_recovery);
    RUN_TEST(test_reuse_behavior_lifoish);
    RUN_TEST(test_canary_integrity_single_thread);
    RUN_TEST(test_randomized_stress_canaries);
    RUN_TEST(test_multithread_smoke_and_cross_thread_free);
    RUN_TEST(test_multithread_ownership_bitmap_stress);
    RUN_TEST(test_multithread_exhaust_refill_uniqueness);
    RUN_TEST(test_performance_sanity);

    std::cout << "\nAll tests passed!" << std::endl;
    return 0;
}
