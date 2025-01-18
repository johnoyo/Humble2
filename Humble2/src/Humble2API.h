#pragma once

#ifdef HBL2_BUILD_DLL
	#define HBL2_API __declspec(dllexport)
#else
	#define HBL2_API __declspec(dllimport)
#endif

// Macro to generate component registration and factory functions
#define HBL2_COMPONENT(TYPE, ...)                                                   \
    extern "C" __declspec(dllexport) const char* RegisterComponent() {              \
        using namespace entt::literals;                                             \
        entt::meta<TYPE>()                                                          \
            .type(entt::hashed_string(typeid(TYPE).name()))                         \
            __VA_ARGS__;                                                            \
        return typeid(TYPE).name();                                                 \
    }                                                                               \
                                                                                    \
    extern "C" __declspec(dllexport) entt::meta_any AddNewComponent(HBL2::Scene* ctx, entt::entity entity) { \
        auto& component = ctx->AddComponent<TYPE>(entity);                                \
        return entt::forward_as_meta(component);                                    \
    }                                                                               \
                                                                                    \
    extern "C" __declspec(dllexport) entt::meta_any GetNewComponent(HBL2::Scene* ctx, entt::entity entity) { \
        auto& component = ctx->GetComponent<TYPE>(entity);                                \
        return entt::forward_as_meta(component);                                    \
    }

#define HBL2_COMPONENT_MEMBER(type, member) .data<&##type::##member>(#member##_hs).prop("name"_hs, #member)