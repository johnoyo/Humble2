#pragma once

#include "Components.h"

#include "ECS\Entity.h"
#include "ECS\Registry.h"
#include "ECS\Reflect.h"

#include "Utilities\Random.h"
#include "Utilities\Allocators\Arena.h"
#include "Utilities\Collections\Collections.h"
#include "Utilities\Collections\HashMap.h"
#include "Utilities\Collections\StaticString.h"

#include <entt.hpp>

namespace HBL2
{
	class ISystem;
	class StructuralCommandBuffer;

	enum class HBL2_API SystemType
	{
		Core = 0,
		Runtime,
		User,
	};

	struct SceneDescriptor
	{
		StaticString<64> name;

		uint32_t maxEntities = 4096;
		uint32_t maxComponents = 64;
		uint32_t maxSystems = 64;
		uint32_t maxStructuralCommandsPerFramePerThread = 256;
		uint32_t maxJobsPerSystem = 16;

		bool useStructuralCommandBuffer = true;
	};

	enum class EntityDuplicationNaming
	{
		APPEND_CLONE_RECURSIVE = 0,
		APPEND_CLONE_TO_BASE_ONLY = 1,
		DONT_APPEND_CLONE = 2,
	};

	class Scene;

	template<class T>
	struct LookupRO
	{
		IComponentStorage* storage = nullptr;
		const Scene* scene = nullptr;
		uint64_t epoch = 0;

		bool Has(Entity e) const
		{
#if !DIST
			if (!scene || scene->Epoch() != epoch)
			{
				return false;
			}
#endif
			return scene && scene->Epoch() == epoch && storage->Has(e);
		}

		const T* TryGet(Entity e) const
		{
#if !DIST
			if (!scene || scene->Epoch() != epoch)
			{
				return nullptr;
			}
#endif
			return storage->Has(e) ? (T*)storage->Get(e) : nullptr;
		}
	};

	template<class T>
	struct LookupRW
	{
		IComponentStorage* storage = nullptr;
		Scene* scene = nullptr;
		uint64_t epoch = 0;

		bool Has(Entity e) const
		{
#if !DIST
			if (!scene || scene->Epoch() != epoch)
			{
				return false;
			}
#endif
			return scene && scene->Epoch() == epoch && storage->Has(e);
		}

		T* TryGet(Entity e) const
		{
#if !DIST
			if (!scene || scene->Epoch() != epoch)
			{
				return nullptr;
			}
#endif
			return storage->Has(e) ? (T*)storage->Get(e) : nullptr;
		}
	};

	class HBL2_API Scene
	{
	public:
		Scene() = default;
		Scene(const SceneDescriptor&& desc);

		static Scene* Copy(Scene* other);
		static void Copy(Scene* src, Scene* dst);

		void Clear();

		Entity CreateEntity();

		Entity CreateEntity(const StaticString<64>& tag);

		Entity CreateEntityWithUUID(UUID uuid, const StaticString<64>& tag = "New Entity");

		Entity FindEntityByUUID(UUID uuid);

		void DestroyEntity(Entity entity);

		Entity DuplicateEntity(Entity entity, EntityDuplicationNaming namingConvention = EntityDuplicationNaming::APPEND_CLONE_TO_BASE_ONLY);

		Entity DuplicateEntityFromScene(Entity entity, Scene* otherScene, EntityDuplicationNaming namingConvention = EntityDuplicationNaming::APPEND_CLONE_TO_BASE_ONLY);

		const uint32_t GetEntityCount() const
		{
			return m_EntityMap.size();
		}

		template<typename T>
		T& GetComponent(Entity entity)
		{
			return m_Registry.GetComponent<T>(entity);
		}

		template<typename T>
		T* TryGetComponent(Entity entity)
		{
			return m_Registry.TryGetComponent<T>(entity);
		}

		template<typename T>
		bool HasComponent(Entity entity)
		{
			return m_Registry.HasComponent<T>(entity);
		}

		template<typename T>
		T& AddComponent(Entity entity)
		{
			return m_Registry.AddComponent<T>(entity);
		}

		template<typename T>
		T* TryAddComponent(Entity entity)
		{
			return m_Registry.TryAddComponent<T>(entity);
		}

		template<typename T>
		void RemoveComponent(Entity entity)
		{
			m_Registry.RemoveComponent<T>(entity);
		}

		template<typename T>
		T& GetOrAddComponent(Entity e)
		{
			return m_Registry.GetOrAddComponent<T>(e);
		}

		template<typename T>
		T& AddOrReplaceComponent(Entity e, T&& comp = {})
		{
			return m_Registry.AddOrReplaceComponent<T>(e, std::forward<T>(comp));
		}

		void DeregisterSystem(const std::string& systemName);
		void DeregisterSystem(ISystem* system);

		template<typename T>
		void RegisterSystem(SystemType type = SystemType::Core)
		{
			RegisterSystem(m_SceneArena.AllocConstruct<T>(), type);
		}
		void RegisterSystem(ISystem* system, SystemType type = SystemType::Core);

		const DArray<ISystem*>& GetSystems() const { return m_Systems; }
		const DArray<ISystem*>& GetCoreSystems() const { return m_CoreSystems; }
		const DArray<ISystem*>& GetRuntimeSystems() const { return m_RuntimeSystems; }
		Registry& GetRegistry() { return m_Registry; }

		[[nodiscard]] inline auto Entities() noexcept
		{
			return m_Registry.Entities();
		}

		[[nodiscard]] inline auto Entities() const noexcept
		{
			return m_Registry.Entities();
		}

		template <typename... Inc>
		[[nodiscard]] inline auto Filter() noexcept
		{
			return m_Registry.Filter<Inc...>();
		}

		const StaticString<64>& GetName() const { return m_Descriptor.name; }
		const SceneDescriptor& GetDescriptor() const { return m_Descriptor; }
		SceneDescriptor& GetDescriptor() { return m_Descriptor; }

		Entity MainCamera = Entity::Null;

		StructuralCommandBuffer* Cmd() { return m_CmdBuffer; }
		uint64_t Epoch() const { return m_Epoch; }
		Arena* GetArena() { return &m_SceneArena; }

	private:
		void InternalDestroyEntity(Entity entity, bool isRootCall);
		Entity InternalDuplicateEntity(Entity entity, Scene* sourceEntityScene, Entity newEntity, bool appendCloneToName);

		Entity DuplicateEntityFromSceneAlt(Entity entity, Scene* sourceEntityScene, std::unordered_map<UUID, Entity>& preservedEntityIDs);
		Entity InternalDuplicateEntityFromSceneAlt(Entity entity, Scene* sourceEntityScene, Entity newEntity, std::unordered_map<UUID, Entity>& preservedEntityIDs);

		Entity DuplicateEntityWhilePreservingUUIDsFromEntityAndDestroy(Entity entity, Scene* prefabSourceScene, Entity entityToPreserveFrom);
		friend class PrefabUtilities;
		friend class SceneSerializer;

	private:
		friend class Pool<Scene, Scene>;

		SceneDescriptor m_Descriptor;
		Registry m_Registry;
		DArray<ISystem*> m_Systems = MakeEmptyDArray<ISystem*>();
		DArray<ISystem*> m_CoreSystems = MakeEmptyDArray<ISystem*>();
		DArray<ISystem*> m_RuntimeSystems = MakeEmptyDArray<ISystem*>();
		HashMap<UUID, Entity> m_EntityMap;

		Arena m_SceneArena;
		PoolReservation* m_Reservation = nullptr;

	private:
		friend class ISystem;

		uint64_t m_Epoch = 0;
		StructuralCommandBuffer* m_CmdBuffer = nullptr;
		void PlaybackStructuralChanges();
		void AdvanceEpoch();
	};
}