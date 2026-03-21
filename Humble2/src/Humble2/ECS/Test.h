
#include "Entity.h"
#include "IComponentStorage.h"
#include "SparseComponentStorage.h"
#include "DenseComponentStorage.h"
#include "SmallComponentStorage.h"
#include "SingletonComponentStorage.h"
#include "FilterQuery.h"
#include "ViewQuery.h"
#include "ExcludeQuery.h"
#include "Registry.h"
#include "TwoLevelBitset.h"
#include "Reflect.h"
#include "Core\Allocators.h"

#include <iostream>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <cstdint>

using namespace HBL2;

// =============================================================================
// Test infrastructure
// =============================================================================

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cout << "  FAILED: " << #cond << " at line " << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

#define RUN_TEST(func) \
    do { \
        std::cout << "  Running " << #func << "... "; \
        if (func()) std::cout << "PASSED\n"; \
        else std::cout << "FAILED\n"; \
    } while (0)

// =============================================================================
// Test components
// =============================================================================

constexpr uint32_t MAX_ENTITIES = 16384;
constexpr uint32_t MAX_COMPONENTS = 128;

struct Position
{
    float x = 0.f, y = 0.f, z = 0.f;

    static constexpr auto schema = HBL2::Reflect::Schema{
        HBL2::Reflect::Field{"x", &Position::x},
        HBL2::Reflect::Field{"y", &Position::y},
        HBL2::Reflect::Field{"z", &Position::z}
    };
};

struct Velocity
{
    float vx = 0.f, vy = 0.f;

    static constexpr auto schema = HBL2::Reflect::Schema{
        HBL2::Reflect::Field{"vx", &Velocity::vx},
        HBL2::Reflect::Field{"vy", &Velocity::vy}
    };
};

struct Health
{
    int hp  = 100;
    int max = 100;

    static constexpr auto schema = HBL2::Reflect::Schema{
        HBL2::Reflect::Field{"hp",  &Health::hp},
        HBL2::Reflect::Field{"max", &Health::max}
    };
};

struct Tag
{
    int value = 0;

    static constexpr auto schema = HBL2::Reflect::Schema{
        HBL2::Reflect::Field{"value", &Tag::value}
    };
};

// Dense storage — nested storage_type alias opts into DenseComponentStorage
struct Transform
{
    float m[16] = {};
    using storage_type = DenseComponentStorage<Transform>;

    static constexpr auto schema = HBL2::Reflect::Schema{
        HBL2::Reflect::Field{"m0", &Transform::m}
    };
};

// Singleton storage
struct GameConfig
{
    int maxPlayers = 4;
    using storage_type = SingletonComponentStorage<GameConfig>;

    static constexpr auto schema = HBL2::Reflect::Schema{
        HBL2::Reflect::Field{"maxPlayers", &GameConfig::maxPlayers}
    };
};

// Small storage — max 16 instances
struct RareMarker
{
    int id = 0;
    using storage_type = SmallComponentStorage<RareMarker, 16>;

    static constexpr auto schema = HBL2::Reflect::Schema{
        HBL2::Reflect::Field{"id", &RareMarker::id}
    };
};

struct Inner { float val = 0.f; };
struct Outer
{
    Inner inner;
    static constexpr auto schema = Reflect::Schema{
        Reflect::Field{"inner.val", &Outer::inner, &Inner::val}
    };
};

// =============================================================================
// EntityRef
// =============================================================================

bool test_entityref_equality()
{
    EntityRef a{ 1, 1 };
    EntityRef b{ 1, 1 };
    EntityRef c{ 1, 2 };
    EntityRef d{ 2, 1 };

    TEST_ASSERT(a == b);
    TEST_ASSERT(!(a == c));
    TEST_ASSERT(!(a == d));
    TEST_ASSERT(!(c == d));
    return true;
}

bool test_entityref_nill()
{
    EntityRef nill = EntityRef::Null;
    TEST_ASSERT(nill.Idx == 0);
    TEST_ASSERT(nill.Gen == 0);
    return true;
}

// =============================================================================
// Registry — entity lifecycle
// =============================================================================

bool test_registry_create_entity()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    TEST_ASSERT(e != EntityRef::Null);
    TEST_ASSERT(e.Idx > 0);
    TEST_ASSERT(e.Gen > 0);
    TEST_ASSERT(r.IsValid(e));

    r.Clear();
    return true;
}

bool test_registry_create_multiple_entities_unique()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    std::unordered_set<int32_t> indices;
    for (int i = 0; i < 100; ++i)
    {
        EntityRef e = r.CreateEntity();
        TEST_ASSERT(e != EntityRef::Null);
        TEST_ASSERT(indices.find(e.Idx) == indices.end());
        indices.insert(e.Idx);
    }

    r.Clear();
    return true;
}

bool test_registry_destroy_entity_invalidates()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    TEST_ASSERT(r.IsValid(e));
    r.DestroyEntity(e);
    TEST_ASSERT(!r.IsValid(e));

    r.Clear();
    return true;
}

bool test_registry_create_entity_with_hint()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef hint{ 5, 0 };
    EntityRef e = r.CreateEntity(hint);
    TEST_ASSERT(e.Idx == 5);
    TEST_ASSERT(r.IsValid(e));

    r.Clear();
    return true;
}

bool test_registry_slot_reuse_after_destroy()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e1 = r.CreateEntity();
    int32_t   slot = e1.Idx;
    r.DestroyEntity(e1);

    EntityRef e2 = r.CreateEntity();
    TEST_ASSERT(e2.Idx == slot);
    TEST_ASSERT(e2.Gen != e1.Gen);
    TEST_ASSERT(!r.IsValid(e1));
    TEST_ASSERT(r.IsValid(e2));

    r.Clear();
    return true;
}

// =============================================================================
// Registry — component add / get / has / remove
// =============================================================================

bool test_registry_add_and_get_component()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    Position& p = r.AddComponent<Position>(e);
    p.x = 1.f; p.y = 2.f; p.z = 3.f;

    Position& got = r.GetComponent<Position>(e);
    TEST_ASSERT(got.x == 1.f);
    TEST_ASSERT(got.y == 2.f);
    TEST_ASSERT(got.z == 3.f);

    r.Clear();
    return true;
}

bool test_registry_has_component()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    TEST_ASSERT(!r.HasComponent<Position>(e));
    r.AddComponent<Position>(e);
    TEST_ASSERT(r.HasComponent<Position>(e));

    r.Clear();
    return true;
}

bool test_registry_remove_component()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    r.AddComponent<Position>(e);
    TEST_ASSERT(r.HasComponent<Position>(e));
    r.RemoveComponent<Position>(e);
    TEST_ASSERT(!r.HasComponent<Position>(e));

    r.Clear();
    return true;
}

bool test_registry_try_add_invalid_entity_returns_null()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef invalid = EntityRef::Null;
    Position* p = r.TryAddComponent<Position>(invalid);
    TEST_ASSERT(p == nullptr);

    r.Clear();
    return true;
}

bool test_registry_try_get_missing_component_returns_null()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    Position* p = r.TryGetComponent<Position>(e);
    TEST_ASSERT(p == nullptr);

    r.Clear();
    return true;
}

bool test_registry_emplace_component()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    Health& h = r.EmplaceComponent<Health>(e);
    h.hp = 50;
    TEST_ASSERT(r.GetComponent<Health>(e).hp == 50);

    r.Clear();
    return true;
}

bool test_registry_add_component_with_value()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    Position init{ 7.f, 8.f, 9.f };
    r.AddComponent<Position>(e, std::move(init));

    Position& got = r.GetComponent<Position>(e);
    TEST_ASSERT(got.x == 7.f);
    TEST_ASSERT(got.y == 8.f);
    TEST_ASSERT(got.z == 9.f);

    r.Clear();
    return true;
}

bool test_registry_multiple_components_on_entity()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    r.AddComponent<Position>(e).x = 1.f;
    r.AddComponent<Velocity>(e).vx = 5.f;
    r.AddComponent<Health>(e).hp = 75;

    TEST_ASSERT(r.HasComponent<Position>(e));
    TEST_ASSERT(r.HasComponent<Velocity>(e));
    TEST_ASSERT(r.HasComponent<Health>(e));
    TEST_ASSERT(r.GetComponent<Position>(e).x  == 1.f);
    TEST_ASSERT(r.GetComponent<Velocity>(e).vx == 5.f);
    TEST_ASSERT(r.GetComponent<Health>(e).hp   == 75);

    r.Clear();
    return true;
}

bool test_registry_components_independent_across_entities()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e1 = r.CreateEntity();
    EntityRef e2 = r.CreateEntity();
    r.AddComponent<Position>(e1).x = 1.f;
    r.AddComponent<Position>(e2).x = 2.f;

    TEST_ASSERT(r.GetComponent<Position>(e1).x == 1.f);
    TEST_ASSERT(r.GetComponent<Position>(e2).x == 2.f);

    r.Clear();
    return true;
}

// =============================================================================
// Registry — storage types
// =============================================================================

bool test_registry_dense_storage()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    Transform& t = r.AddComponent<Transform>(e);
    t.m[0] = 1.f;

    TEST_ASSERT(r.HasComponent<Transform>(e));
    TEST_ASSERT(r.GetComponent<Transform>(e).m[0] == 1.f);

    r.Clear();
    return true;
}

bool test_registry_singleton_only_one_instance()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e1 = r.CreateEntity();
    EntityRef e2 = r.CreateEntity();

    GameConfig& cfg = r.AddComponent<GameConfig>(e1);
    cfg.maxPlayers = 8;

    // Second entity trying to add a singleton must fail
    GameConfig* cfg2 = r.TryAddComponent<GameConfig>(e2);
    TEST_ASSERT(cfg2 == nullptr);

    TEST_ASSERT(r.GetComponent<GameConfig>(e1).maxPlayers == 8);

    r.Clear();
    return true;
}

bool test_registry_small_storage()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    RareMarker& m = r.AddComponent<RareMarker>(e);
    m.id = 42;

    TEST_ASSERT(r.HasComponent<RareMarker>(e));
    TEST_ASSERT(r.GetComponent<RareMarker>(e).id == 42);

    r.Clear();
    return true;
}

bool test_registry_small_storage_respects_max()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    for (int i = 0; i < 16; ++i)
    {
        EntityRef e = r.CreateEntity();
        RareMarker* m = r.TryAddComponent<RareMarker>(e);
        TEST_ASSERT(m != nullptr);
    }

    // 17th add must fail
    EntityRef extra = r.CreateEntity();
    RareMarker* overflow = r.TryAddComponent<RareMarker>(extra);
    TEST_ASSERT(overflow == nullptr);

    r.Clear();
    return true;
}

// =============================================================================
// SparseComponentStorage
// =============================================================================

bool test_sparse_remove_preserves_others()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e1 = r.CreateEntity();
    EntityRef e2 = r.CreateEntity();
    EntityRef e3 = r.CreateEntity();

    r.AddComponent<Position>(e1).x = 1.f;
    r.AddComponent<Position>(e2).x = 2.f;
    r.AddComponent<Position>(e3).x = 3.f;

    r.RemoveComponent<Position>(e2);

    TEST_ASSERT( r.HasComponent<Position>(e1));
    TEST_ASSERT(!r.HasComponent<Position>(e2));
    TEST_ASSERT( r.HasComponent<Position>(e3));
    TEST_ASSERT(r.GetComponent<Position>(e1).x == 1.f);
    TEST_ASSERT(r.GetComponent<Position>(e3).x == 3.f);

    r.Clear();
    return true;
}

bool test_sparse_re_add_after_remove()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    r.AddComponent<Position>(e).x = 10.f;
    r.RemoveComponent<Position>(e);
    TEST_ASSERT(!r.HasComponent<Position>(e));

    r.AddComponent<Position>(e).x = 20.f;
    TEST_ASSERT(r.HasComponent<Position>(e));
    TEST_ASSERT(r.GetComponent<Position>(e).x == 20.f);

    r.Clear();
    return true;
}

// =============================================================================
// ViewQuery
// =============================================================================

bool test_view_query_iterates_all()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e1 = r.CreateEntity();
    EntityRef e2 = r.CreateEntity();
    EntityRef e3 = r.CreateEntity();

    r.AddComponent<Position>(e1).x = 1.f;
    r.AddComponent<Position>(e2).x = 2.f;
    r.AddComponent<Position>(e3).x = 3.f;

    float sum = 0.f;
    r.Filter<Position>().ForEach([&](Position& p) { sum += p.x; });
    TEST_ASSERT(sum == 6.f);

    r.Clear();
    return true;
}

bool test_view_query_skips_entities_without_component()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e1 = r.CreateEntity();
    EntityRef e2 = r.CreateEntity(); // no Position
    EntityRef e3 = r.CreateEntity();

    r.AddComponent<Position>(e1).x = 1.f;
    r.AddComponent<Position>(e3).x = 3.f;

    int count = 0;
    r.Filter<Position>().ForEach([&](Position&) { ++count; });
    TEST_ASSERT(count == 2);

    r.Clear();
    return true;
}

bool test_view_query_with_entity_ref()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    r.AddComponent<Position>(e).x = 99.f;

    bool found = false;
    r.Filter<Position>().ForEach([&](EntityRef ref, Position& p)
    {
        TEST_ASSERT(ref == e);
        TEST_ASSERT(p.x == 99.f);
        found = true;
    });
    TEST_ASSERT(found);

    r.Clear();
    return true;
}

bool test_view_query_mutation_visible()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    r.AddComponent<Position>(e).x = 0.f;

    r.Filter<Position>().ForEach([](Position& p) { p.x = 42.f; });
    TEST_ASSERT(r.GetComponent<Position>(e).x == 42.f);

    r.Clear();
    return true;
}

bool test_view_query_empty_storage()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    int count = 0;
    r.Filter<Position>().ForEach([&](Position&) { ++count; });
    TEST_ASSERT(count == 0);

    r.Clear();
    return true;
}

// =============================================================================
// FilterQuery
// =============================================================================

bool test_filter_query_two_components()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e1 = r.CreateEntity();
    EntityRef e2 = r.CreateEntity();
    EntityRef e3 = r.CreateEntity();

    r.AddComponent<Position>(e1).x = 1.f;
    r.AddComponent<Velocity>(e1);
    r.AddComponent<Position>(e2).x = 2.f; // no Velocity
    r.AddComponent<Position>(e3).x = 3.f;
    r.AddComponent<Velocity>(e3);

    int count = 0;
    float sumX = 0.f;
    r.Filter<Position, Velocity>().ForEach([&](Position& p, Velocity&)
    {
        sumX += p.x;
        ++count;
    });

    TEST_ASSERT(count == 2);
    TEST_ASSERT(sumX == 4.f);

    r.Clear();
    return true;
}

bool test_filter_query_three_components()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e1 = r.CreateEntity();
    EntityRef e2 = r.CreateEntity();

    r.AddComponent<Position>(e1);
    r.AddComponent<Velocity>(e1);
    r.AddComponent<Health>(e1).hp = 50;

    r.AddComponent<Position>(e2);
    r.AddComponent<Velocity>(e2);
    // e2 has no Health

    int count = 0;
    r.Filter<Position, Velocity, Health>().ForEach([&](Position&, Velocity&, Health&) { ++count; });
    TEST_ASSERT(count == 1);

    r.Clear();
    return true;
}

bool test_filter_query_with_entity_ref()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    r.AddComponent<Position>(e).x = 5.f;
    r.AddComponent<Velocity>(e).vx = 3.f;

    bool found = false;
    r.Filter<Position, Velocity>().ForEach([&](EntityRef ref, Position& p, Velocity& v)
    {
        TEST_ASSERT(ref == e);
        TEST_ASSERT(p.x == 5.f);
        TEST_ASSERT(v.vx == 3.f);
        found = true;
    });
    TEST_ASSERT(found);

    r.Clear();
    return true;
}

bool test_filter_query_mutation_visible()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    r.AddComponent<Position>(e).x = 0.f;
    r.AddComponent<Velocity>(e).vx = 5.f;

    r.Filter<Position, Velocity>().ForEach([](Position& p, Velocity& v) { p.x += v.vx; });
    TEST_ASSERT(r.GetComponent<Position>(e).x == 5.f);

    r.Clear();
    return true;
}

bool test_filter_query_no_false_positives()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    // i = 0, 3, 6, 9 get Velocity → 4 entities have both
    for (int i = 0; i < 10; ++i)
    {
        EntityRef e = r.CreateEntity();
        r.AddComponent<Position>(e).x = (float)i;
        if (i % 3 == 0)
            r.AddComponent<Velocity>(e);
    }

    int count = 0;
    r.Filter<Position, Velocity>().ForEach([&](Position&, Velocity&) { ++count; });
    TEST_ASSERT(count == 4);

    r.Clear();
    return true;
}

// =============================================================================
// ExcludeQuery
// =============================================================================

bool test_exclude_query_basic()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e1 = r.CreateEntity();
    EntityRef e2 = r.CreateEntity();
    EntityRef e3 = r.CreateEntity();

    r.AddComponent<Position>(e1); r.AddComponent<Velocity>(e1);
    r.AddComponent<Position>(e2); r.AddComponent<Velocity>(e2); r.AddComponent<Tag>(e2);
    r.AddComponent<Position>(e3); r.AddComponent<Velocity>(e3);

    int count = 0;
    r.Filter<Position, Velocity>()
        .Exclude<Tag>()
        .ForEach([&](Position&, Velocity&) { ++count; });

    TEST_ASSERT(count == 2); // e2 excluded

    r.Clear();
    return true;
}

bool test_exclude_query_no_tagged_entities()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    for (int i = 0; i < 5; ++i)
    {
        EntityRef e = r.CreateEntity();
        r.AddComponent<Position>(e);
        r.AddComponent<Velocity>(e);
    }

    int count = 0;
    r.Filter<Position, Velocity>()
        .Exclude<Tag>()
        .ForEach([&](Position&, Velocity&) { ++count; });

    TEST_ASSERT(count == 5);

    r.Clear();
    return true;
}

// =============================================================================
// TwoLevelBitset
// =============================================================================

bool test_bitset_set_and_test()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);
    EntityRef e = r.CreateEntity();
    r.AddComponent<Position>(e);
    TEST_ASSERT(r.GetStorage<Position>()->Mask().test(e.Idx));
    r.Clear();
    return true;
}

bool test_bitset_clear_after_remove()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);
    EntityRef e = r.CreateEntity();
    r.AddComponent<Position>(e);
    r.RemoveComponent<Position>(e);
    TEST_ASSERT(!r.GetStorage<Position>()->Mask().test(e.Idx));
    r.Clear();
    return true;
}

bool test_bitset_foreach_visits_correct_count()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e1 = r.CreateEntity();
    EntityRef e2 = r.CreateEntity();
    EntityRef e3 = r.CreateEntity();

    r.AddComponent<Position>(e1);
    r.AddComponent<Position>(e2);
    r.AddComponent<Position>(e3);

    std::vector<uint32_t> visited;
    r.GetStorage<Position>()->Mask().forEach([&](uint32_t idx) { visited.push_back(idx); });
    TEST_ASSERT(visited.size() == 3);

    r.Clear();
    return true;
}

// =============================================================================
// TypeInfo
// =============================================================================

bool test_type_info_correct()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);
    EntityRef e = r.CreateEntity();
    r.AddComponent<Position>(e);
    TEST_ASSERT(r.GetStorage<Position>()->TypeInfo() == typeid(Position));
    r.Clear();
    return true;
}

// =============================================================================
// StorageFor — compile-time selection
// =============================================================================

bool test_storage_for_defaults_to_sparse()
{
    TEST_ASSERT((std::is_same_v<StorageFor<Position>, SparseComponentStorage<Position>>));
    return true;
}

bool test_storage_for_dense()
{
    TEST_ASSERT((std::is_same_v<StorageFor<Transform>, DenseComponentStorage<Transform>>));
    return true;
}

bool test_storage_for_singleton()
{
    TEST_ASSERT((std::is_same_v<StorageFor<GameConfig>, SingletonComponentStorage<GameConfig>>));
    return true;
}

bool test_storage_for_small()
{
    TEST_ASSERT((std::is_same_v<StorageFor<RareMarker>, SmallComponentStorage<RareMarker, 16>>));
    return true;
}

// =============================================================================
// Reflect — Schema / Field / Any
// =============================================================================

bool test_reflect_foreach_visits_all_fields()
{
    Position p{ 1.f, 2.f, 3.f };

    std::vector<std::string_view> names;
    Reflect::ForEach(p, [&](std::string_view name, auto&) { names.push_back(name); });

    TEST_ASSERT(names.size() == 3);
    TEST_ASSERT(names[0] == "x");
    TEST_ASSERT(names[1] == "y");
    TEST_ASSERT(names[2] == "z");
    return true;
}

bool test_reflect_get_field_by_name()
{
    Position p{ 4.f, 5.f, 6.f };

    TEST_ASSERT(*Reflect::Get(p, "x").TryGetAs<float>() == 4.f);
    TEST_ASSERT(*Reflect::Get(p, "y").TryGetAs<float>() == 5.f);
    TEST_ASSERT(*Reflect::Get(p, "z").TryGetAs<float>() == 6.f);
    return true;
}

bool test_reflect_get_invalid_name_returns_empty()
{
    Position p{};
    Reflect::Any a = Reflect::Get(p, "nonexistent");
    TEST_ASSERT(!a.IsValid());
    return true;
}

bool test_reflect_set_field_typed()
{
    Position p{};
    TEST_ASSERT(Reflect::Set(p, "x", 99.f));
    TEST_ASSERT(p.x == 99.f);
    return true;
}

bool test_reflect_set_field_from_any()
{
    Position p{};
    float val = 77.f;
    Reflect::Any a = Reflect::Any::From(val);
    TEST_ASSERT(Reflect::Set(p, "y", a));
    TEST_ASSERT(p.y == 77.f);
    return true;
}

bool test_reflect_set_wrong_type_returns_false()
{
    Position p{};
    int wrong = 5;
    Reflect::Any a = Reflect::Any::From(wrong);
    TEST_ASSERT(!Reflect::Set(p, "x", a)); // int into float — must fail
    return true;
}

bool test_reflect_set_nonexistent_field_returns_false()
{
    Position p{};
    TEST_ASSERT(!Reflect::Set(p, "nonexistent", 1.f));
    return true;
}

bool test_reflect_schema_has()
{
    TEST_ASSERT( Position::schema.Has("x"));
    TEST_ASSERT( Position::schema.Has("y"));
    TEST_ASSERT( Position::schema.Has("z"));
    TEST_ASSERT(!Position::schema.Has("w"));
    return true;
}

bool test_reflect_field_count()
{
    TEST_ASSERT(Position::schema.FieldCount() == 3);
    TEST_ASSERT(Velocity::schema.FieldCount() == 2);
    TEST_ASSERT(Health::schema.FieldCount()   == 2);
    return true;
}

bool test_reflect_any_is()
{
    float f = 1.f;
    Reflect::Any a = Reflect::Any::From(f);
    TEST_ASSERT( a.Is<float>());
    TEST_ASSERT(!a.Is<int>());
    return true;
}

bool test_reflect_forward_as_meta()
{
    Position p{ 1.f, 2.f, 3.f };
    Reflect::Any a = Reflect::ForwardAsMeta(p);
    TEST_ASSERT(a.IsValid());
    TEST_ASSERT(a.Is<Position>());
    TEST_ASSERT(a.TryGetAs<Position>()->x == 1.f);
    return true;
}

bool test_reflect_any_mutation_through_ref()
{
    Position p{ 0.f, 0.f, 0.f };
    Reflect::Any a = Reflect::Any::From(p.x);
    *a.TryGetAs<float>() = 55.f;
    TEST_ASSERT(p.x == 55.f); // non-owning — writes back to original
    return true;
}

bool test_reflect_chained_field_accessor()
{
    Outer o;
    o.inner.val = 3.14f;

    TEST_ASSERT(*Reflect::Get(o, "inner.val").TryGetAs<float>() == 3.14f);

    TEST_ASSERT(Reflect::Set(o, "inner.val", 2.71f));
    TEST_ASSERT(o.inner.val == 2.71f);

    return true;
}

// =============================================================================
// Reflect — Runtime registry
// =============================================================================

bool test_reflect_register_populates_registry()
{
    Reflect::Register<Position>();

    const Reflect::TypeEntry* entry = Reflect::FindType(typeid(Position).name());
    TEST_ASSERT(entry != nullptr);
    TEST_ASSERT(entry->size      == sizeof(Position));
    TEST_ASSERT(entry->alignment == alignof(Position));
    TEST_ASSERT(entry->fields.size() == 3);
    TEST_ASSERT(entry->fields[0].name == "x");
    TEST_ASSERT(entry->fields[1].name == "y");
    TEST_ASSERT(entry->fields[2].name == "z");
    return true;
}

bool test_reflect_register_idempotent()
{
    Reflect::Register<Position>(); // ensure already registered
    size_t before = Reflect::GetRegistry().size();
    Reflect::Register<Position>(); // register again
    size_t after = Reflect::GetRegistry().size();
    TEST_ASSERT(before == after);
    return true;
}

bool test_reflect_type_entry_get()
{
    Reflect::Register<Health>();
    const Reflect::TypeEntry* entry = Reflect::FindType(typeid(Health).name());
    TEST_ASSERT(entry != nullptr);

    Health h{ 75, 100 };
    Reflect::Any hp = entry->get(&h, "hp");
    TEST_ASSERT(hp.IsValid());
    TEST_ASSERT(*hp.TryGetAs<int>() == 75);
    return true;
}

bool test_reflect_type_entry_set_from_any()
{
    Reflect::Register<Health>();
    const Reflect::TypeEntry* entry = Reflect::FindType(typeid(Health).name());
    TEST_ASSERT(entry != nullptr);

    Health h{ 50, 100 };
    int newVal = 25;
    Reflect::Any val = Reflect::Any::From(newVal);
    TEST_ASSERT(entry->setFromAny(&h, "hp", val));
    TEST_ASSERT(h.hp == 25);
    return true;
}

bool test_reflect_type_entry_foreach()
{
    Reflect::Register<Position>();
    const Reflect::TypeEntry* entry = Reflect::FindType(typeid(Position).name());
    TEST_ASSERT(entry != nullptr);

    Position p{ 10.f, 20.f, 30.f };

    std::vector<std::string_view> names;
    entry->forEach(&p, &names, [](void* ud, std::string_view name, Reflect::Any)
    {
        static_cast<std::vector<std::string_view>*>(ud)->push_back(name);
    });

    TEST_ASSERT(names.size() == 3);
    TEST_ASSERT(names[0] == "x");
    return true;
}

bool test_reflect_foreach_registered_types()
{
    Reflect::Register<Position>();
    Reflect::Register<Velocity>();
    Reflect::Register<Health>();

    bool foundPos = false, foundVel = false, foundHealth = false;
    Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& e)
    {
        if (e.typeName == typeid(Position).name()) foundPos    = true;
        if (e.typeName == typeid(Velocity).name()) foundVel    = true;
        if (e.typeName == typeid(Health).name())   foundHealth = true;
    });

    TEST_ASSERT(foundPos);
    TEST_ASSERT(foundVel);
    TEST_ASSERT(foundHealth);
    return true;
}

bool test_reflect_registry_ops_add_get_has_remove()
{
    Reflect::Register<Position>();
    const Reflect::TypeEntry* entry = Reflect::FindType(typeid(Position).name());
    TEST_ASSERT(entry != nullptr);

    Registry r(MAX_ENTITIES, MAX_COMPONENTS);
    EntityRef e = r.CreateEntity();

    Reflect::Any added = entry->addToRegistry(&r, e);
    TEST_ASSERT(added.IsValid());
    TEST_ASSERT(added.Is<Position>());
    TEST_ASSERT(entry->hasInRegistry(&r, e));

    Reflect::Any got = entry->getFromRegistry(&r, e);
    TEST_ASSERT(got.IsValid());

    entry->removeFromRegistry(&r, e);
    TEST_ASSERT(!entry->hasInRegistry(&r, e));

    r.Clear();
    return true;
}

bool test_reflect_clone_to_registry()
{
    Reflect::Register<Position>();
    const Reflect::TypeEntry* entry = Reflect::FindType(typeid(Position).name());

    Registry src(MAX_ENTITIES / 2, MAX_COMPONENTS);
    Registry dst(MAX_ENTITIES / 2, MAX_COMPONENTS);

    EntityRef e = src.CreateEntity();
    dst.CreateEntity(e); // same slot in dst

    src.AddComponent<Position>(e).x = 42.f;
    entry->cloneToRegistry(&src, &dst);

    TEST_ASSERT(dst.HasComponent<Position>(e));
    TEST_ASSERT(dst.GetComponent<Position>(e).x == 42.f);

    src.Clear();
    dst.Clear();
    return true;
}

// =============================================================================
// Stress / correctness at scale
// =============================================================================

bool test_many_entities_add_remove_cycle()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    const int N = 500;
    std::vector<EntityRef> entities;
    entities.reserve(N);

    for (int i = 0; i < N; ++i)
    {
        EntityRef e = r.CreateEntity();
        entities.push_back(e);
        r.AddComponent<Position>(e).x = (float)i;
        r.AddComponent<Velocity>(e).vx = (float)(i * 2);
    }

    // Remove Position from every other entity
    for (int i = 0; i < N; i += 2)
        r.RemoveComponent<Position>(entities[i]);

    int posCount = 0, velCount = 0;
    r.Filter<Position>().ForEach([&](Position&) { ++posCount; });
    r.Filter<Velocity>().ForEach([&](Velocity&) { ++velCount; });

    TEST_ASSERT(posCount == N / 2);
    TEST_ASSERT(velCount == N);

    r.Clear();
    return true;
}

bool test_filter_query_values_correct_after_removes()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e1 = r.CreateEntity();
    EntityRef e2 = r.CreateEntity();
    EntityRef e3 = r.CreateEntity();

    r.AddComponent<Position>(e1).x = 1.f; r.AddComponent<Velocity>(e1);
    r.AddComponent<Position>(e2).x = 2.f; r.AddComponent<Velocity>(e2);
    r.AddComponent<Position>(e3).x = 3.f; r.AddComponent<Velocity>(e3);

    r.RemoveComponent<Position>(e2);

    float sum = 0.f;
    r.Filter<Position, Velocity>().ForEach([&](Position& p, Velocity&) { sum += p.x; });
    TEST_ASSERT(sum == 4.f); // e1 + e3

    r.Clear();
    return true;
}

bool test_entity_generation_invalidates_stale_refs()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    EntityRef e = r.CreateEntity();
    r.AddComponent<Position>(e);
    r.DestroyEntity(e);

    EntityRef e2 = r.CreateEntity();
    TEST_ASSERT(e2.Idx == e.Idx);
    TEST_ASSERT(e2.Gen != e.Gen);
    TEST_ASSERT(!r.IsValid(e));
    TEST_ASSERT( r.IsValid(e2));

    Position* stale = r.TryGetComponent<Position>(e);
    TEST_ASSERT(stale == nullptr);

    r.Clear();
    return true;
}

bool test_filter_query_consistent_after_swap_remove()
{
    Registry r(MAX_ENTITIES, MAX_COMPONENTS);

    // Add 5 entities, remove the middle one (triggers swap-and-pop)
    std::vector<EntityRef> entities;
    for (int i = 0; i < 5; ++i)
    {
        EntityRef e = r.CreateEntity();
        entities.push_back(e);
        r.AddComponent<Position>(e).x = (float)(i + 1);
        r.AddComponent<Velocity>(e);
    }

    r.RemoveComponent<Position>(entities[2]); // swap-and-pop

    float sum = 0.f;
    int count = 0;
    r.Filter<Position, Velocity>().ForEach([&](Position& p, Velocity&)
    {
        sum += p.x;
        ++count;
    });

    TEST_ASSERT(count == 4);
    // sum = 1+2+4+5 = 12
    TEST_ASSERT(sum == 12.f);

    r.Clear();
    return true;
}

// =============================================================================
// Entry point
// =============================================================================

int TestECS()
{
    Log::Initialize();

    Allocator::Arena.Initialize(500_MB, 16_MB);

    auto* frameArenaReservationDummy = Allocator::Arena.Reserve("FrameArenaReservationDummy", 8_KB);
    Allocator::DummyArena.Initialize(&Allocator::Arena, 8_KB, frameArenaReservationDummy);

    std::cout << "\n=== ECS Tests ===\n\n";

    std::cout << "-- EntityRef --\n";
    RUN_TEST(test_entityref_equality);
    RUN_TEST(test_entityref_nill);

    std::cout << "\n-- Registry: Entity Lifecycle --\n";
    RUN_TEST(test_registry_create_entity);
    RUN_TEST(test_registry_create_multiple_entities_unique);
    RUN_TEST(test_registry_destroy_entity_invalidates);
    RUN_TEST(test_registry_create_entity_with_hint);
    RUN_TEST(test_registry_slot_reuse_after_destroy);

    std::cout << "\n-- Registry: Component Operations --\n";
    RUN_TEST(test_registry_add_and_get_component);
    RUN_TEST(test_registry_has_component);
    RUN_TEST(test_registry_remove_component);
    RUN_TEST(test_registry_try_add_invalid_entity_returns_null);
    RUN_TEST(test_registry_try_get_missing_component_returns_null);
    RUN_TEST(test_registry_emplace_component);
    RUN_TEST(test_registry_add_component_with_value);
    RUN_TEST(test_registry_multiple_components_on_entity);
    RUN_TEST(test_registry_components_independent_across_entities);

    std::cout << "\n-- Registry: Storage Types --\n";
    RUN_TEST(test_registry_dense_storage);
    RUN_TEST(test_registry_singleton_only_one_instance);
    RUN_TEST(test_registry_small_storage);
    RUN_TEST(test_registry_small_storage_respects_max);

    std::cout << "\n-- SparseComponentStorage --\n";
    RUN_TEST(test_sparse_remove_preserves_others);
    RUN_TEST(test_sparse_re_add_after_remove);

    std::cout << "\n-- ViewQuery --\n";
    RUN_TEST(test_view_query_iterates_all);
    RUN_TEST(test_view_query_skips_entities_without_component);
    RUN_TEST(test_view_query_with_entity_ref);
    RUN_TEST(test_view_query_mutation_visible);
    RUN_TEST(test_view_query_empty_storage);

    std::cout << "\n-- FilterQuery --\n";
    RUN_TEST(test_filter_query_two_components);
    RUN_TEST(test_filter_query_three_components);
    RUN_TEST(test_filter_query_with_entity_ref);
    RUN_TEST(test_filter_query_mutation_visible);
    RUN_TEST(test_filter_query_no_false_positives);

    std::cout << "\n-- ExcludeQuery --\n";
    RUN_TEST(test_exclude_query_basic);
    RUN_TEST(test_exclude_query_no_tagged_entities);

    std::cout << "\n-- TwoLevelBitset --\n";
    RUN_TEST(test_bitset_set_and_test);
    RUN_TEST(test_bitset_clear_after_remove);
    RUN_TEST(test_bitset_foreach_visits_correct_count);

    std::cout << "\n-- TypeInfo --\n";
    RUN_TEST(test_type_info_correct);

    std::cout << "\n-- StorageFor (compile-time) --\n";
    RUN_TEST(test_storage_for_defaults_to_sparse);
    RUN_TEST(test_storage_for_dense);
    RUN_TEST(test_storage_for_singleton);
    RUN_TEST(test_storage_for_small);

    std::cout << "\n-- Reflect: Schema / Field / Any --\n";
    RUN_TEST(test_reflect_foreach_visits_all_fields);
    RUN_TEST(test_reflect_get_field_by_name);
    RUN_TEST(test_reflect_get_invalid_name_returns_empty);
    RUN_TEST(test_reflect_set_field_typed);
    RUN_TEST(test_reflect_set_field_from_any);
    RUN_TEST(test_reflect_set_wrong_type_returns_false);
    RUN_TEST(test_reflect_set_nonexistent_field_returns_false);
    RUN_TEST(test_reflect_schema_has);
    RUN_TEST(test_reflect_field_count);
    RUN_TEST(test_reflect_any_is);
    RUN_TEST(test_reflect_forward_as_meta);
    RUN_TEST(test_reflect_any_mutation_through_ref);
    RUN_TEST(test_reflect_chained_field_accessor);

    std::cout << "\n-- Reflect: Runtime Registry --\n";
    RUN_TEST(test_reflect_register_populates_registry);
    RUN_TEST(test_reflect_register_idempotent);
    RUN_TEST(test_reflect_type_entry_get);
    RUN_TEST(test_reflect_type_entry_set_from_any);
    RUN_TEST(test_reflect_type_entry_foreach);
    RUN_TEST(test_reflect_foreach_registered_types);
    RUN_TEST(test_reflect_registry_ops_add_get_has_remove);
    RUN_TEST(test_reflect_clone_to_registry);

    std::cout << "\n-- Stress / Correctness --\n";
    RUN_TEST(test_many_entities_add_remove_cycle);
    RUN_TEST(test_filter_query_values_correct_after_removes);
    RUN_TEST(test_entity_generation_invalidates_stale_refs);
    RUN_TEST(test_filter_query_consistent_after_swap_remove);

    std::cout << "\nAll ECS tests executed.\n";
    return 0;
}
