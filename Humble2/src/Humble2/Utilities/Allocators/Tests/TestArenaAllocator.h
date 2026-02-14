// ArenaAllocatorTests.cpp
// -----------------------------------------------------------------------------
// Unit tests for ArenaAllocator.hpp (v3)
// -----------------------------------------------------------------------------
//
// Style: Minimal, macro-based test framework (no external dependencies).
// Focus: Core correctness of MainArena, PoolReservation, Arena, ScratchArena.
//
// -----------------------------------------------------------------------------
#pragma once

#include "Base.h"
#include "../Arena.h"

#include <iostream>
#include <vector>
#include <cstring>
#include <chrono>
#include <stdexcept>

using namespace HBL2;

// Minimal test macros
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cout << "FAILED: " << #cond << " at line " << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

#define RUN_TEST(func) \
    do { \
        std::cout << "Running " << #func << "... "; \
        if (func()) std::cout << "PASSED\n"; \
        else std::cout << "FAILED\n"; \
    } while (0)

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

bool test_global_arena_initialization()
{
    MainArena global(1_MB, 128_KB);
    TEST_ASSERT(global.MetaSize() > 0);
    TEST_ASSERT(global.DataSize() > 0);
    TEST_ASSERT(global.MetaCarved() == 0);
    TEST_ASSERT(global.DataCarved() == 0);
    return true;
}

bool test_reservation_allocation()
{
    MainArena global(1_MB, 128_KB);

    PoolReservation* r1 = global.Reserve("Render", 256_KB);
    TEST_ASSERT(r1 != nullptr);
    TEST_ASSERT(std::strcmp(r1->Name, "Render") == 0);
    TEST_ASSERT(r1->Size == 256_KB);

    PoolReservation* r2 = global.GetReservation("Render");
    TEST_ASSERT(r2 == r1);

    TEST_ASSERT(global.DataCarved() >= 256_KB);
    return true;
}

bool test_chunk_allocation_and_reuse()
{
    MainArena global(512 * 1024, 64 * 1024);
    PoolReservation* r = global.Reserve("Physics", 128 * 1024);

    ArenaChunk* c1 = global.AllocateChunkStruct(32 * 1024, r);
    TEST_ASSERT(c1);
    TEST_ASSERT(c1->Capacity >= 32 * 1024);
    TEST_ASSERT(c1->Reservation == r);

    // Free and reallocate — should reuse
    global.FreeChunkStruct(c1);
    ArenaChunk* c2 = global.AllocateChunkStruct(16 * 1024, r);
    TEST_ASSERT(c1 == c2); // reused from free list

    // Global fallback reuse
    ArenaChunk* c3 = global.AllocateChunkStruct(16 * 1024, nullptr);
    global.FreeChunkStruct(c3);
    ArenaChunk* c4 = global.AllocateChunkStruct(8 * 1024, nullptr);
    TEST_ASSERT(c3 == c4);

    return true;
}

bool test_arena_basic_allocation()
{
    MainArena global(2_MB, 256_KB);
    PoolReservation* r = global.Reserve("Main", 512_KB);
    Arena arena(&global, 128_KB, r);

    int* p1 = static_cast<int*>(arena.Alloc(sizeof(int), alignof(int)));
    TEST_ASSERT(p1);
    *p1 = 1234;

    double* p2 = static_cast<double*>(arena.Alloc(sizeof(double), alignof(double)));
    TEST_ASSERT(p2);
    *p2 = 3.14159;

    TEST_ASSERT(*p1 == 1234);
    TEST_ASSERT(*p2 == 3.14159);

    return true;
}

bool test_arena_reset_and_restore()
{
    MainArena global(1_MB, 128_KB);
    PoolReservation* r = global.Reserve("Scratch", 256_KB);
    Arena arena(&global, 128_KB, r);

    auto m1 = arena.Mark();
    void* a1 = arena.Alloc(64);
    void* a2 = arena.Alloc(64);
    TEST_ASSERT(a1 && a2);

    auto m2 = arena.Mark();
    void* a3 = arena.Alloc(64);
    TEST_ASSERT(a3);

    arena.Restore(m2); // should remove a3
    void* a4 = arena.Alloc(64);
    TEST_ASSERT(a4 == a3); // reused from same position

    arena.Reset();
    void* a5 = arena.Alloc(64);
    TEST_ASSERT(a5 != nullptr);

    return true;
}

bool test_scratch_scope_raii()
{
    MainArena global(512_KB, 64_KB);
    Arena arena(&global, 128_KB);
    {
        ScratchArena scratch(arena);
        void* a1 = scratch.Alloc(32);
        TEST_ASSERT(a1);
        void* a2 = scratch.Alloc(32);
        TEST_ASSERT(a2);
        TEST_ASSERT(a2 > a1);
    }
    // After scope, allocations should be rolled back
    void* a3 = arena.Alloc(64);
    TEST_ASSERT(a3); // Should reuse from beginning safely
    return true;
}

bool test_chunk_exhaustion_and_bad_alloc()
{
    bool caught = false;
    try
    {
        MainArena global(64_KB, 8_KB);
        PoolReservation* r = global.Reserve("Tiny", 8_KB);
        Arena arena(&global, 8_KB, r);
        for (int i = 0; i < 100; ++i)
            arena.Alloc(1024); // likely to fill up
    }
    catch (const std::bad_alloc&)
    {
        caught = true;
    }
    TEST_ASSERT(caught);
    return true;
}

bool test_free_list_scaling_under_stress()
{
    MainArena global(4_MB, 256_KB);
    PoolReservation* r1 = global.Reserve("Audio", 512_KB);
    PoolReservation* r2 = global.Reserve("Render", 512_KB);

    std::vector<ArenaChunk*> chunks;
    for (int i = 0; i < 20; ++i)
    {
        chunks.push_back(global.AllocateChunkStruct(32 * 1024, (i % 2) ? r1 : r2));
    }

    for (auto* ch : chunks)
    {
        global.FreeChunkStruct(ch);
    }

    ArenaChunk* reused = global.AllocateChunkStruct(16 * 1024, r1);
    TEST_ASSERT(reused != nullptr);
    TEST_ASSERT(reused->Reservation == r1);

    return true;
}

bool test_meta_data_accounting()
{
    MainArena global(1024 * 1024, 128 * 1024);
    PoolReservation* r = global.Reserve("MetaTest", 128 * 1024);
    Arena arena(&global, {}, r);

    size_t before_meta = global.MetaCarved();
    arena.Alloc(256);
    size_t after_meta = global.MetaCarved();

    TEST_ASSERT(after_meta >= before_meta);
    TEST_ASSERT(global.DataCarved() > 0);
    return true;
}

bool test_alignment_and_padding()
{
    MainArena global(1_MB, 128_KB);
    Arena arena(&global, 500_KB);

    void* p1 = arena.Alloc(1, 8);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(p1) % 8 == 0);

    void* p2 = arena.Alloc(4, 16);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(p2) % 16 == 0);

    void* p3 = arena.Alloc(8, 32);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(p3) % 32 == 0);

    return true;
}

bool stress_test_many_small_allocs()
{
    MainArena global(16_MB, 512_KB);
    PoolReservation* r = global.Reserve("Stress", 2_MB);
    Arena arena(&global, 2_MB, r);

    std::vector<void*> ptrs;
    for (int i = 0; i < 100000; ++i)
    {
        void* p = arena.Alloc(8);
        TEST_ASSERT(p);
        ptrs.push_back(p);
    }
    return true;
}

// -----------------------------------------------------------------------------
// Test runner
// -----------------------------------------------------------------------------

int TestArenaAllocator()
{
    std::cout << "Running ArenaAllocator Tests\n\n";

    RUN_TEST(test_global_arena_initialization);
    RUN_TEST(test_reservation_allocation);
    RUN_TEST(test_chunk_allocation_and_reuse);
    RUN_TEST(test_arena_basic_allocation);
    RUN_TEST(test_arena_reset_and_restore);
    RUN_TEST(test_scratch_scope_raii);
    RUN_TEST(test_chunk_exhaustion_and_bad_alloc);
    RUN_TEST(test_free_list_scaling_under_stress);
    RUN_TEST(test_meta_data_accounting);
    RUN_TEST(test_alignment_and_padding);
    RUN_TEST(stress_test_many_small_allocs);

    std::cout << "\nAll tests executed.\n";
    return 0;
}
