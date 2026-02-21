#pragma once

#include "Entity.h"
#include "Components.h"

#include "View.h"
#include "Group.h"

#include "Utilities\Random.h"
#include "Utilities\Allocators\Arena.h"
#include "Utilities\Collections\Collections.h"

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

	template <typename... Cs>
	inline constexpr entt::get_t<Cs...> Get{};

	template <typename... Cs>
	inline constexpr entt::exclude_t<Cs...> Exclude{};

	struct SceneDescriptor
	{
		std::string name;
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
		entt::storage<T>* storage = nullptr;
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
			return scene && scene->Epoch() == epoch && storage->contains(e);
		}

		const T* TryGet(Entity e) const
		{
#if !DIST
			if (!scene || scene->Epoch() != epoch)
			{
				return nullptr;
			}
#endif
			return storage->contains(e) ? &storage->get(e) : nullptr;
		}
	};

	template<class T>
	struct LookupRW
	{
		entt::storage<T>* storage = nullptr;
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
			return scene && scene->Epoch() == epoch && storage->contains(e);
		}

		T* TryGet(Entity e) const
		{
#if !DIST
			if (!scene || scene->Epoch() != epoch)
			{
				return nullptr;
			}
#endif
			return storage->contains(e) ? &storage->get(e) : nullptr;
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

		Entity CreateEntity(const std::string& tag);

		Entity CreateEntityWithUUID(UUID uuid, const std::string& tag = "New Entity");

		Entity FindEntityByUUID(UUID uuid);

		void DestroyEntity(Entity entity);

		Entity DuplicateEntity(Entity entity, EntityDuplicationNaming namingConvention = EntityDuplicationNaming::APPEND_CLONE_TO_BASE_ONLY);

		Entity DuplicateEntityFromScene(Entity entity, Scene* otherScene, EntityDuplicationNaming namingConvention = EntityDuplicationNaming::APPEND_CLONE_TO_BASE_ONLY);

		template<typename... T>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<T...>();
		}

		const uint32_t GetEntityCount() const
		{
			return m_EntityMap.size();
		}

		template<typename T>
		T& GetComponent(Entity entity)
		{
			return m_Registry.get<T>(entity);
		}

		template<typename T>
		T* TryGetComponent(Entity entity)
		{
			return m_Registry.try_get<T>(entity);
		}

		template<typename T>
		T& GetOrAddComponent(Entity entity)
		{
			return m_Registry.get_or_emplace<T>(entity);
		}

		template<typename T>
		bool HasComponent(Entity entity)
		{
			return m_Registry.any_of<T>(entity);
		}

		template<typename T>
		T& AddComponent(Entity entity)
		{
			return m_Registry.emplace<T>(entity);
		}

		template<typename T>
		T& AddOrReplaceComponent(Entity entity)
		{
			return m_Registry.emplace_or_replace<T>(entity);
		}

		template<typename T>
		void RemoveComponent(Entity entity)
		{
			m_Registry.remove<T>(entity);
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
		entt::registry& GetRegistry() { return m_Registry; }

		[[nodiscard]] inline auto Entities() noexcept
		{
			return HBL2::View{ m_Registry.view<entt::entity>() };
		}

		[[nodiscard]] inline auto Entities() const noexcept
		{
			return HBL2::View{ m_Registry.view<entt::entity>() };
		}

		template <typename... Inc>
		[[nodiscard]] constexpr auto View() noexcept
		{
			return HBL2::View{ m_Registry.view<Inc...>() };
		}

		template <typename... Inc, typename... Exc>
		[[nodiscard]] constexpr auto View(entt::exclude_t<Exc...> ex) noexcept
		{
			return HBL2::View{ m_Registry.view<Inc...>(ex) };
		}

		template <typename... Owned>
		[[nodiscard]] constexpr auto Group() noexcept
		{
			auto g = m_Registry.template group<Owned...>();
			return HBL2::Group{ g };
		}

		template <typename... Owned, typename... Get>
		[[nodiscard]] constexpr auto Group(entt::get_t<Get...> getter) noexcept
		{
			auto g = m_Registry.template group<Owned...>(getter);
			return HBL2::Group{ g };
		}

		template <typename... Owned, typename... Get, typename... Ex>
		[[nodiscard]] constexpr auto Group(entt::get_t<Get...> getter, entt::exclude_t<Ex...> excl) noexcept
		{
			auto g = m_Registry.template group<Owned...>(getter, excl);
			return HBL2::Group{ g };
		}

		template <typename... Owned, typename... Ex>
		[[nodiscard]] constexpr auto Group(entt::exclude_t<Ex...> excl) noexcept
		{
			auto g = m_Registry.template group<Owned...>(excl);
			return HBL2::Group{ g };
		}

		entt::meta_ctx& GetMetaContext() { return m_MetaContext; }
		const std::string& GetName() const { return m_Name; }

		template <typename T>
		static std::vector<std::byte> Serialize(const T& component)
		{
			std::vector<std::byte> data(sizeof(T));
			std::memcpy(data.data(), &component, sizeof(T));
			return data;
		}

		template <typename T>
		static T Deserialize(const std::vector<std::byte>& data)
		{
			T component;
			std::memcpy(&component, data.data(), sizeof(T));
			return component;
		}

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

		std::string m_Name;
		entt::registry m_Registry;
		entt::meta_ctx m_MetaContext;
		DArray<ISystem*> m_Systems = MakeEmptyDArray<ISystem*>();
		DArray<ISystem*> m_CoreSystems = MakeEmptyDArray<ISystem*>();
		DArray<ISystem*> m_RuntimeSystems = MakeEmptyDArray<ISystem*>();
		HMap<UUID, Entity> m_EntityMap = MakeEmptyHMap<UUID, Entity>();

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