#pragma once

#include "Scene\Components.h"
#include "Scene\ISystem.h"
#include "Scene\Scene.h"

#include "Core/Input.h"

#include "Core\Context.h"
#include "Asset\EditorAssetManager.h"
#include "Resources\ResourceManager.h"

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
    extern "C" __declspec(dllexport) void SerializeComponents_##TYPE(HBL2::Scene* ctx, ByteStorage& data, bool cleanRegistry)   \
    {                                                                                                                           \
        std::vector<entt::entity> entitiesToRemoveTheComponentFrom;                                                             \
                                                                                                                                \
        for (auto entity : ctx->GetRegistry().view<TYPE>())                                                                     \
        {                                                                                                                       \
            auto& component = ctx->GetComponent<TYPE>(entity);                                                                  \
            data[#TYPE][entity] = HBL2::Scene::Serialize(component);                                                            \
        }                                                                                                                       \
                                                                                                                                \
        if (cleanRegistry)                                                                                                      \
        {                                                                                                                       \
            for (auto entity : entitiesToRemoveTheComponentFrom)                                                                \
            {                                                                                                                   \
                ctx->RemoveComponent<TYPE>(entity);                                                                             \
            }                                                                                                                   \
                                                                                                                                \
            entitiesToRemoveTheComponentFrom.clear();                                                                           \
                                                                                                                                \
            ctx->GetRegistry().clear<TYPE>();                                                                                   \
            ctx->GetRegistry().storage<TYPE>().clear();                                                                         \
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
