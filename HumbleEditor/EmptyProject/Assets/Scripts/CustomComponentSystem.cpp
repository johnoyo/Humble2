#include "Humble2Core.h"

struct CustomComponent
{
	float Value = 10.f;
};

// Factory function to register the component
extern "C" __declspec(dllexport) const char* RegisterComponent()
{
	using namespace entt::literals;

	entt::meta<CustomComponent>()
		.type(entt::hashed_string(typeid(CustomComponent).name()))
		.data<&CustomComponent::FValue>("FValue"_hs).prop("name"_hs, "FValue")
		.data<&CustomComponent::IValue>("IValue"_hs).prop("name"_hs, "IValue")

	return typeid(CustomComponent).name();
}

// Factory function to add the component
extern "C" __declspec(dllexport) entt::meta_any AddNewComponent(HBL2::Scene* ctx, entt::entity entity)
{
	auto& component = ctx->AddComponent<CustomComponent>(entity);
	return entt::forward_as_meta(component);
}

// Factory function to add the component
extern "C" __declspec(dllexport) entt::meta_any GetNewComponent(HBL2::Scene* ctx, entt::entity entity)
{
	auto& component = ctx->GetComponent<CustomComponent>(entity);
	return entt::forward_as_meta(component);
}