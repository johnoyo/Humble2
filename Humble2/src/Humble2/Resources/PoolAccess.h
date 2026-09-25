#pragma once

#include "Humble2API.h"

#include "BaseTypeDefinitions.h"
#include "Handle.h"

namespace HBL2
{
	struct Asset;
	struct Mesh;
	struct Material;
	class Scene;
	class Script;
	class Sound;
	class Prefab;

	template<typename T> inline constexpr bool always_false_v = false;

	template<typename T>
	struct PoolAccess
	{
		static void Acquire(Handle<T>) { static_assert(always_false_v<T>, "No PoolAccess specialization visible for this T"); }
		static void Release(Handle<T>) { static_assert(always_false_v<T>, "No PoolAccess specialization visible for this T"); }
	};

	HBL2_API void AcquireAsset(Handle<Asset> handle);
	HBL2_API void ReleaseAsset(Handle<Asset> handle);

	HBL2_API void AcquireResource(uint32_t packedHandle, ResourceType type);
	HBL2_API void ReleaseResource(uint32_t packedHandle, ResourceType type);

	template<> inline void PoolAccess<Asset>::Acquire(Handle<Asset> handle) { AcquireAsset(handle); }
	template<> inline void PoolAccess<Asset>::Release(Handle<Asset> handle) { ReleaseAsset(handle); }

	template<> inline void PoolAccess<Mesh>::Acquire(Handle<Mesh> handle) { AcquireResource(handle.Pack(), ResourceType::Mesh); }
	template<> inline void PoolAccess<Mesh>::Release(Handle<Mesh> handle) { ReleaseResource(handle.Pack(), ResourceType::Mesh); }

	template<> inline void PoolAccess<Material>::Acquire(Handle<Material> handle) { AcquireResource(handle.Pack(), ResourceType::Material); }
	template<> inline void PoolAccess<Material>::Release(Handle<Material> handle) { ReleaseResource(handle.Pack(), ResourceType::Material); }

	template<> inline void PoolAccess<Scene>::Acquire(Handle<Scene> handle) { AcquireResource(handle.Pack(), ResourceType::Scene); }
	template<> inline void PoolAccess<Scene>::Release(Handle<Scene> handle) { ReleaseResource(handle.Pack(), ResourceType::Scene); }

	template<> inline void PoolAccess<Script>::Acquire(Handle<Script> handle) { AcquireResource(handle.Pack(), ResourceType::Script); }
	template<> inline void PoolAccess<Script>::Release(Handle<Script> handle) { ReleaseResource(handle.Pack(), ResourceType::Script); }

	template<> inline void PoolAccess<Sound>::Acquire(Handle<Sound> handle) { AcquireResource(handle.Pack(), ResourceType::Sound); }
	template<> inline void PoolAccess<Sound>::Release(Handle<Sound> handle) { ReleaseResource(handle.Pack(), ResourceType::Sound); }

	template<> inline void PoolAccess<Prefab>::Acquire(Handle<Prefab> handle) { AcquireResource(handle.Pack(), ResourceType::Prefab); }
	template<> inline void PoolAccess<Prefab>::Release(Handle<Prefab> handle) { ReleaseResource(handle.Pack(), ResourceType::Prefab); }

	template<> inline void PoolAccess<BindGroup>::Acquire(Handle<BindGroup> handle) { AcquireResource(handle.Pack(), ResourceType::BindGroup); }
	template<> inline void PoolAccess<BindGroup>::Release(Handle<BindGroup> handle) { ReleaseResource(handle.Pack(), ResourceType::BindGroup); }

	template<> inline void PoolAccess<BindGroupLayout>::Acquire(Handle<BindGroupLayout> handle) { AcquireResource(handle.Pack(), ResourceType::BindGroupLayout); }
	template<> inline void PoolAccess<BindGroupLayout>::Release(Handle<BindGroupLayout> handle) { ReleaseResource(handle.Pack(), ResourceType::BindGroupLayout); }

	template<> inline void PoolAccess<Shader>::Acquire(Handle<Shader> handle) { AcquireResource(handle.Pack(), ResourceType::Shader); }
	template<> inline void PoolAccess<Shader>::Release(Handle<Shader> handle) { ReleaseResource(handle.Pack(), ResourceType::Shader); }

	template<> inline void PoolAccess<Texture>::Acquire(Handle<Texture> handle) { AcquireResource(handle.Pack(), ResourceType::Texture); }
	template<> inline void PoolAccess<Texture>::Release(Handle<Texture> handle) { ReleaseResource(handle.Pack(), ResourceType::Texture); }

	template<> inline void PoolAccess<Buffer>::Acquire(Handle<Buffer> handle) { AcquireResource(handle.Pack(), ResourceType::Buffer); }
	template<> inline void PoolAccess<Buffer>::Release(Handle<Buffer> handle) { ReleaseResource(handle.Pack(), ResourceType::Buffer); }

	template<> inline void PoolAccess<RenderPass>::Acquire(Handle<RenderPass> handle) { AcquireResource(handle.Pack(), ResourceType::RenderPass); }
	template<> inline void PoolAccess<RenderPass>::Release(Handle<RenderPass> handle) { ReleaseResource(handle.Pack(), ResourceType::RenderPass); }

	template<> inline void PoolAccess<RenderPassLayout>::Acquire(Handle<RenderPassLayout> handle) { AcquireResource(handle.Pack(), ResourceType::RenderPassLayout); }
	template<> inline void PoolAccess<RenderPassLayout>::Release(Handle<RenderPassLayout> handle) { ReleaseResource(handle.Pack(), ResourceType::RenderPassLayout); }
}