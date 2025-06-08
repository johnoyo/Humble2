#pragma once

#include "Scene\Components.h"
#include "Scene\ISystem.h"
#include "Scene\Scene.h"
#include "Scene\SceneManager.h"

#include "Core\Input.h"
#include "Core\InputMapping.h"

#include "Core\Context.h"
#include "Asset\AssetManager.h"
#include "Resources\ResourceManager.h"

#include "Physics\Physics.h"
#include "Physics\Physics2d.h"
#include "Physics\Physics3d.h"

#include "Utilities\Allocators\BaseAllocator.h"
#include "Utilities\Allocators\StandardAllocator.h"
#include "Utilities\Allocators\BumpAllocator.h"
#include "Utilities\Allocators\BinAllocator.h"
#include "Utilities\Allocators\FreeListAllocator.h"

#include "Utilities\Collections\StaticArray.h"
#include "Utilities\Collections\DynamicArray.h"
#include "Utilities\Collections\HashMap.h"
#include "Utilities\Collections\Set.h"
#include "Utilities\Collections\Stack.h"
#include "Utilities\Collections\Queue.h"
#include "Utilities\Collections\Deque.h"
#include "Utilities\Collections\BitFlags.h"

// Macro to generate system registration factory function
#define REGISTER_HBL2_SYSTEM(TYPE)                                                                                              \
    extern "C" __declspec(dllexport) void RegisterSystem_##TYPE(HBL2::Scene* ctx)                                               \
    {                                                                                                                           \
        HBL2::ISystem* new##TYPE = new TYPE;                                                                                    \
        new##TYPE->Name = #TYPE;                                                                                                \
        ctx->RegisterSystem(new##TYPE, HBL2::SystemType::User);                                                                 \
    }                                                                                                                           \

// Concatenation Macros
#define CONCAT_HELPER(x, y) x##_##y
#define CONCAT(x, y) CONCAT_HELPER(x, y)

// Create a Unique Struct Name
#define MAKE_UNIQUE_NAME(base) CONCAT(base, COMPONENT_NAME_HASH)

// Struct Definition Macro
#define DEFINITION(name, unique_name, ...) struct unique_name __VA_ARGS__ ; using name = unique_name;

// Component Macro
#define HBL2_COMPONENT(name, ...) DEFINITION(name, MAKE_UNIQUE_NAME(name), __VA_ARGS__)

// Macro to generate component registration and factory functions
#define REGISTER_HBL2_COMPONENT(TYPE, ...)                                                                                      \
    using ByteStorage = std::unordered_map<std::string, std::unordered_map<entt::entity, std::vector<std::byte>>>;              \
    extern "C" __declspec(dllexport) const char* RegisterComponent_##TYPE(HBL2::Scene* ctx)                                     \
    {                                                                                                                           \
        using namespace entt::literals;                                                                                         \
        entt::meta<TYPE>(ctx->GetMetaContext())                                                                                 \
            .type(entt::hashed_string(typeid(TYPE).name()))                                                                     \
            __VA_ARGS__;                                                                                                        \
        return typeid(TYPE).name();                                                                                             \
    }                                                                                                                           \
                                                                                                                                \
    extern "C" __declspec(dllexport) entt::meta_any AddComponent_##TYPE(HBL2::Scene* ctx, entt::entity entity)                  \
    {                                                                                                                           \
        auto& component = ctx->AddComponent<TYPE>(entity);                                                                      \
        return entt::forward_as_meta(ctx->GetMetaContext(), component);                                                         \
    }                                                                                                                           \
                                                                                                                                \
    extern "C" __declspec(dllexport) entt::meta_any GetComponent_##TYPE(HBL2::Scene* ctx, entt::entity entity)                  \
    {                                                                                                                           \
        auto& component = ctx->GetComponent<TYPE>(entity);                                                                      \
        return entt::forward_as_meta(ctx->GetMetaContext(), component);                                                         \
    }                                                                                                                           \
                                                                                                                                \
    extern "C" __declspec(dllexport) void RemoveComponent_##TYPE(HBL2::Scene* ctx, entt::entity entity)                         \
    {                                                                                                                           \
        ctx->RemoveComponent<TYPE>(entity);                                                                                     \
    }                                                                                                                           \
                                                                                                                                \
    extern "C" __declspec(dllexport) bool HasComponent_##TYPE(HBL2::Scene* ctx, entt::entity entity)                            \
    {                                                                                                                           \
        return ctx->HasComponent<TYPE>(entity);                                                                                 \
    }                                                                                                                           \
                                                                                                                                \
    extern "C" __declspec(dllexport) void ClearComponentStorage_##TYPE(HBL2::Scene* ctx)                                        \
    {                                                                                                                           \
        ctx->GetRegistry().clear<TYPE>();                                                                                       \
        ctx->GetRegistry().storage<TYPE>().clear();                                                                             \
        ctx->GetRegistry().compact<TYPE>();                                                                                     \
    }                                                                                                                           \
                                                                                                                                \
    extern "C" __declspec(dllexport) void SerializeComponents_##TYPE(HBL2::Scene* ctx, ByteStorage& data, bool cleanRegistry)   \
    {                                                                                                                           \
        for (auto entity : ctx->GetRegistry().view<TYPE>())                                                                     \
        {                                                                                                                       \
            auto& component = ctx->GetComponent<TYPE>(entity);                                                                  \
            data[#TYPE][entity] = HBL2::Scene::Serialize(component);                                                            \
        }                                                                                                                       \
                                                                                                                                \
        if (cleanRegistry)                                                                                                      \
        {                                                                                                                       \
            ctx->GetRegistry().clear<TYPE>();                                                                                   \
            ctx->GetRegistry().storage<TYPE>().clear();                                                                         \
            ctx->GetRegistry().compact<TYPE>();                                                                                 \
        }                                                                                                                       \
    }                                                                                                                           \
                                                                                                                                \
    extern "C" __declspec(dllexport) void DeserializeComponents_##TYPE(HBL2::Scene* ctx, ByteStorage& data)                     \
    {                                                                                                                           \
        for (auto& [entity, bytes] : data[#TYPE])                                                                               \
        {                                                                                                                       \
            auto& c = ctx->AddComponent<TYPE>(entity);                                                                          \
            c = HBL2::Scene::Deserialize<TYPE>(bytes);                                                                          \
        }                                                                                                                       \
    }                                                                                                                           \

#define HBL2_COMPONENT_MEMBER(type, member) .data<&##type::##member>(#member##_hs).prop("name"_hs, #member)
