#pragma once

// EnTT_TLSFAllocator_Tests.cpp
// -----------------------------------------------------------------------------
// Correctness-oriented integration tests for EnTT using TLSF-backed STL allocator.
// -----------------------------------------------------------------------------
//
// Style: Minimal macro-based runner (no external test framework).
//
// Focus:
//  - Registry construction with custom allocator
//  - Entity lifecycle (create/destroy)
//  - Component add/remove/replace
//  - Views/groups iteration correctness
//  - Storage growth/shrink patterns (reserve, many emplace, destroy)
//  - Clear/reset behavior
//  - `on_construct` / `on_destroy` signals
//  - Basic snapshot/restore (if available)
//  - OOM behavior (small heap) should not corrupt allocator
//  - Stress sanity (many ops), and allocator ValidateHeap() checks
//
// -----------------------------------------------------------------------------


#include <entt/include/entt.hpp>

#include "Utilities/Allocators/MainArena.h"
#include "Utilities/Allocators/OffsetArena.h"
#include "Utilities/Allocators/OffsetAllocator.h"

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <algorithm>

using namespace HBL2;

// ----------------------------
// Minimal test macros
// ----------------------------
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
        else { std::cout << "FAILED\n"; return 1; } \
    } while (0)

// ----------------------------
// Helpers
// ----------------------------

// IMPORTANT: pass maxAllocs so tests can scale meta accordingly.
static inline bool make_tlsf(
    MainArena & g,
    PoolReservation * &res,
    OffsetArena & tlsf,
    size_t totalBytes,
    size_t metaBytes,
    size_t heapBytes,
    uint32_t maxAllocs)
{
    g.Initialize(totalBytes, metaBytes);
    res = g.Reserve("ECS_TLSF", heapBytes);

    // OffsetArena now allocates Node/free arrays from META:
    tlsf.Initialize(&g, (uint32_t)heapBytes, res, maxAllocs);

    TEST_ASSERT(tlsf.Validate());
    return true;
}

static inline bool validate_tlsf(OffsetArena & tlsf)
{
    TEST_ASSERT(tlsf.Validate());
    return true;
}

struct Position { float x, y; };
struct Velocity { float vx, vy; };
struct Health { int hp; };

struct BigComponent
{
    alignas(16) uint8_t blob[256];
    uint32_t id = 0;
};

static inline uint64_t hash_u64(uint64_t x)
{
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

template<typename Registry>
struct PositionListener
{
    std::vector<entt::entity>* constructed = nullptr;
    std::vector<entt::entity>* destroyed = nullptr;

    void on_construct(Registry&, entt::entity e) { constructed->push_back(e); }
    void on_destroy(Registry&, entt::entity e) { destroyed->push_back(e); }
};

// ----------------------------
// Tests
// ----------------------------

bool test_registry_construction_and_basic_allocations()
{
    MainArena g;
    PoolReservation* res = nullptr;
    OffsetArena tlsf;

    // maxAllocs=256k -> ~10MB meta needed; use 16MB to be safe
    make_tlsf(g, res, tlsf,
        64ull * 1024 * 1024,
        16ull * 1024 * 1024,
        32ull * 1024 * 1024,
        256u * 1024u);

    using EcsAlloc = OffsetAllocator<entt::entity>;
    using Registry = entt::basic_registry<entt::entity, EcsAlloc>;

    Registry reg{ EcsAlloc{ &tlsf } };

    auto e = reg.create();
    TEST_ASSERT(reg.valid(e));

    reg.emplace<Position>(e, 1.f, 2.f);
    reg.emplace<Velocity>(e, 3.f, 4.f);

    Position* p = reg.try_get<Position>(e);
    Velocity* v = reg.try_get<Velocity>(e);
    TEST_ASSERT(p && v);
    TEST_ASSERT(p->x == 1.f && p->y == 2.f);
    TEST_ASSERT(v->vx == 3.f && v->vy == 4.f);

    reg.destroy(e);
    TEST_ASSERT(!reg.valid(e));

    validate_tlsf(tlsf);
    return true;
}

bool test_component_add_remove_replace()
{
    MainArena g;
    PoolReservation* res = nullptr;
    OffsetArena tlsf;

    make_tlsf(g, res, tlsf,
        64ull * 1024 * 1024,
        16ull * 1024 * 1024,
        32ull * 1024 * 1024,
        256u * 1024u);

    using EcsAlloc = OffsetAllocator<entt::entity>;
    entt::basic_registry<entt::entity, EcsAlloc> reg{ EcsAlloc{ &tlsf } };

    auto e = reg.create();
    TEST_ASSERT(reg.emplace<Health>(e, 100).hp == 100);
    TEST_ASSERT(reg.all_of<Health>(e));

    reg.replace<Health>(e, 50);
    TEST_ASSERT(reg.get<Health>(e).hp == 50);

    TEST_ASSERT(reg.remove<Health>(e) == 1u);
    TEST_ASSERT(!reg.all_of<Health>(e));

    validate_tlsf(tlsf);
    return true;
}

bool test_views_iteration_correctness()
{
    MainArena g;
    PoolReservation* res = nullptr;
    OffsetArena tlsf;

    // Large test: bump maxAllocs to reduce allocator metadata exhaustion
    // meta needs to grow too.
    make_tlsf(g, res, tlsf,
        128ull * 1024 * 1024,
        24ull * 1024 * 1024,   // increased
        64ull * 1024 * 1024,
        512u * 1024u);         // more node slots for lots of allocations

    HBL2::SetDefaultOffsetArena(&tlsf);

    using EcsAlloc = OffsetAllocator<entt::entity>;
    entt::basic_registry<entt::entity, EcsAlloc> reg{ EcsAlloc{ &tlsf } };

    constexpr int N = 100000;

    for (int i = 0; i < N; ++i)
    {
        auto e = reg.create();
        reg.emplace<Position>(e, float(i), float(i + 1));
        if ((i % 2) == 0) reg.emplace<Velocity>(e, 1.f, -1.f);
        if ((i % 3) == 0) reg.emplace<Health>(e, 100 + i);
    }

    size_t countP = 0;
    for (auto e : reg.view<Position>()) { (void)e; ++countP; }
    TEST_ASSERT(countP == (size_t)N);

    size_t countPV = 0;
    reg.view<Position, Velocity>().each([&](entt::entity e, Position&, Velocity&)
    {
        bool allOf = reg.all_of<Position, Velocity>(e);
        TEST_ASSERT(allOf);
        ++countPV;
    });
    TEST_ASSERT(countPV == (size_t)((N + 1) / 2));

    size_t countP_noV = 0;
    for (auto e : reg.view<Position>(entt::exclude<Velocity>))
    {
        TEST_ASSERT(reg.all_of<Position>(e));
        TEST_ASSERT(!reg.any_of<Velocity>(e));
        ++countP_noV;
    }
    TEST_ASSERT(countP_noV + countPV == (size_t)N);

    validate_tlsf(tlsf);
    return true;
}

bool test_groups_basic()
{
    MainArena g;
    PoolReservation* res = nullptr;
    OffsetArena tlsf;

    make_tlsf(g, res, tlsf,
        128ull * 1024 * 1024,
        24ull * 1024 * 1024,
        64ull * 1024 * 1024,
        512u * 1024u);

    HBL2::SetDefaultOffsetArena(&tlsf);

    using EcsAlloc = OffsetAllocator<entt::entity>;
    entt::basic_registry<entt::entity, EcsAlloc> reg{ EcsAlloc{ &tlsf } };

    constexpr int N = 20000;
    std::vector<entt::entity> entities;
    entities.reserve(N);

    for (int i = 0; i < N; ++i)
    {
        auto e = reg.create();
        entities.push_back(e);
        reg.emplace<Position>(e, float(i), float(i));
        reg.emplace<Velocity>(e, float(i % 5), float(i % 7));
        if ((i % 4) == 0) reg.emplace<Health>(e, 1000);
    }

    auto grp = reg.group<Position, Velocity>(entt::get<Health>);
    size_t cnt = 0;
    for (auto e : grp)
    {
        (void)grp.get<Position>(e);
        (void)grp.get<Velocity>(e);
        ++cnt;
    }
    TEST_ASSERT(cnt == (size_t)(N / 4));

    validate_tlsf(tlsf);
    return true;
}

bool test_signals_on_construct_on_destroy()
{
    MainArena g;
    PoolReservation* res = nullptr;
    OffsetArena tlsf;

    make_tlsf(g, res, tlsf,
        64ull * 1024 * 1024,
        16ull * 1024 * 1024,
        32ull * 1024 * 1024,
        256u * 1024u);

    using EcsAlloc = OffsetAllocator<entt::entity>;
    using Registry = entt::basic_registry<entt::entity, EcsAlloc>;
    Registry reg{ EcsAlloc{ &tlsf } };

    std::vector<entt::entity> constructed;
    std::vector<entt::entity> destroyed;

    PositionListener<Registry> listener{ &constructed, &destroyed };

    reg.on_construct<Position>().template connect<&PositionListener<Registry>::on_construct>(listener);
    reg.on_destroy<Position>().template connect<&PositionListener<Registry>::on_destroy>(listener);

    auto e1 = reg.create();
    auto e2 = reg.create();
    reg.emplace<Position>(e1, 1.f, 2.f);
    reg.emplace<Position>(e2, 3.f, 4.f);

    TEST_ASSERT(constructed.size() == 2);

    reg.remove<Position>(e1);
    reg.destroy(e2);

    TEST_ASSERT(destroyed.size() == 2);

    validate_tlsf(tlsf);
    return true;
}

bool test_storage_growth_shrink_and_allocator_integrity()
{
    MainArena g;
    PoolReservation* res = nullptr;
    OffsetArena tlsf;

    // Heavy allocations -> larger maxAllocs and meta
    make_tlsf(g, res, tlsf,
        256ull * 1024 * 1024,
        48ull * 1024 * 1024,     // increased
        128ull * 1024 * 1024,
        1024u * 1024u);          // 1M node slots

    using EcsAlloc = OffsetAllocator<entt::entity>;
    entt::basic_registry<entt::entity, EcsAlloc> reg{ EcsAlloc{ &tlsf } };

    constexpr int N = 200000;
    std::vector<entt::entity> ents;
    ents.reserve(N);

    reg.storage<Position>().reserve(N);
    reg.storage<BigComponent>().reserve(N / 4);

    for (int i = 0; i < N; ++i)
    {
        auto e = reg.create();
        ents.push_back(e);

        reg.emplace<Position>(e, float(i), float(i));
        if ((i % 4) == 0)
        {
            BigComponent bc{};
            bc.id = (uint32_t)i;
            reg.emplace<BigComponent>(e, bc);
        }
    }

    validate_tlsf(tlsf);

    for (int i = 0; i < N; i += 2)
        reg.destroy(ents[i]);

    validate_tlsf(tlsf);

    if constexpr (requires { reg.storage<Position>().shrink_to_fit(); })
        reg.storage<Position>().shrink_to_fit();

    if constexpr (requires { reg.storage<BigComponent>().shrink_to_fit(); })
        reg.storage<BigComponent>().shrink_to_fit();

    validate_tlsf(tlsf);
    return true;
}

bool test_registry_clear_and_reuse()
{
    MainArena g;
    PoolReservation* res = nullptr;
    OffsetArena tlsf;

    make_tlsf(g, res, tlsf,
        128ull * 1024 * 1024,
        24ull * 1024 * 1024,
        64ull * 1024 * 1024,
        512u * 1024u);

    using EcsAlloc = OffsetAllocator<entt::entity>;
    entt::basic_registry<entt::entity, EcsAlloc> reg{ EcsAlloc{ &tlsf } };

    for (int pass = 0; pass < 5; ++pass)
    {
        constexpr int N = 50000;
        for (int i = 0; i < N; ++i)
        {
            auto e = reg.create();
            reg.emplace<Position>(e, float(i), float(i));
            if ((i & 1) == 0) reg.emplace<Velocity>(e, 1.f, 2.f);
        }

        validate_tlsf(tlsf);

        reg.clear();
        validate_tlsf(tlsf);

        TEST_ASSERT(reg.view<entt::entity>().empty());
    }

    return true;
}

bool test_oom_behavior_small_heap_no_corruption()
{
    MainArena g;
    PoolReservation* res = nullptr;
    OffsetArena tlsf;

    // Even with small heap, meta still must be enough for allocator metadata.
    make_tlsf(g, res, tlsf,
        16ull * 1024 * 1024,
        14ull * 1024 * 1024,   // meta must remain large
        2ull * 1024 * 1024,
        256u * 1024u);

    using EcsAlloc = OffsetAllocator<entt::entity>;
    entt::basic_registry<entt::entity, EcsAlloc> reg{ EcsAlloc{ &tlsf } };

    bool caught = false;
    try
    {
        for (;;)
        {
            auto e = reg.create();
            BigComponent bc{};
            bc.id = 123;
            reg.emplace<BigComponent>(e, bc);
        }
    }
    catch (const std::bad_alloc&)
    {
        caught = true;
    }
    TEST_ASSERT(caught);

    validate_tlsf(tlsf);

    return true;
}

bool stress_test_random_ops_with_canaries()
{
    MainArena g;
    PoolReservation* res = nullptr;
    OffsetArena tlsf;

    make_tlsf(g, res, tlsf,
        256ull * 1024 * 1024,
        48ull * 1024 * 1024,
        128ull * 1024 * 1024,
        1024u * 1024u);

    using EcsAlloc = OffsetAllocator<entt::entity>;
    entt::basic_registry<entt::entity, EcsAlloc> reg{ EcsAlloc{ &tlsf } };

    std::mt19937 rng(0x1234u);
    std::uniform_int_distribution<int> opDist(0, 99);

    std::vector<entt::entity> live;
    live.reserve(200000);

    constexpr int OPS = 500000;

    for (int i = 0; i < OPS; ++i)
    {
        int op = opDist(rng);

        if (op < 40)
        {
            auto e = reg.create();
            live.push_back(e);

            uint64_t h = hash_u64((uint64_t)entt::to_integral(e));
            reg.emplace<Position>(e, float(h & 0xFFFF), float((h >> 16) & 0xFFFF));

            if ((h & 3) == 0) reg.emplace<Velocity>(e, 1.f, -1.f);
            if ((h & 7) == 0) reg.emplace<Health>(e, int(h & 1023));
        }
        else if (op < 70)
        {
            if (live.empty()) continue;
            auto e = live[(size_t)rng() % live.size()];
            if (!reg.valid(e)) continue;

            if (reg.any_of<Velocity>(e)) reg.remove<Velocity>(e);
            else reg.emplace<Velocity>(e, 2.f, 3.f);

            if (reg.any_of<Health>(e)) reg.remove<Health>(e);
            else reg.emplace<Health>(e, 77);
        }
        else
        {
            if (live.empty()) continue;
            size_t idx = (size_t)rng() % live.size();
            auto e = live[idx];
            live[idx] = live.back();
            live.pop_back();
            if (reg.valid(e)) reg.destroy(e);
        }

        if ((i % 10000) == 0)
        {
            validate_tlsf(tlsf);

            for (int k = 0; k < 16 && !live.empty(); ++k)
            {
                auto e = live[(size_t)rng() % live.size()];
                if (!reg.valid(e)) continue;

                auto& p = reg.get<Position>(e);
                uint64_t h = hash_u64((uint64_t)entt::to_integral(e));
                TEST_ASSERT(p.x == float(h & 0xFFFF));
                TEST_ASSERT(p.y == float((h >> 16) & 0xFFFF));
            }
        }
    }

    validate_tlsf(tlsf);
    return true;
}

// -----------------------------------------------------------------------------
// Test runner
// -----------------------------------------------------------------------------
inline int TestEnTT_OffsetAllocator()
{
    std::cout << "Running EnTT + OffsetAllocator integration tests\n\n";

    RUN_TEST(test_registry_construction_and_basic_allocations);
    RUN_TEST(test_component_add_remove_replace);
    RUN_TEST(test_views_iteration_correctness);
    RUN_TEST(test_groups_basic);
    RUN_TEST(test_signals_on_construct_on_destroy);
    RUN_TEST(test_storage_growth_shrink_and_allocator_integrity);
    RUN_TEST(test_registry_clear_and_reuse);
    RUN_TEST(test_oom_behavior_small_heap_no_corruption);
    RUN_TEST(stress_test_random_ops_with_canaries);

    std::cout << "\nAll tests passed!\n";
    return 0;
}
