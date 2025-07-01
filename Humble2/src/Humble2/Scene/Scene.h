#pragma once

#include "Entity.h"
#include "ISystem.h"
#include "Components.h"

#include "View.h"
#include "Group.h"

#include "Utilities\Random.h"

#include <entt.hpp>

namespace HBL2
{
	template <typename... Cs>
	inline constexpr entt::get_t<Cs...> Get{};

	template <typename... Cs>
	inline constexpr entt::exclude_t<Cs...> Exclude{};

	struct SceneDescriptor
	{
		std::string name;
	};

	class HBL2_API Scene
	{
	public:
		Scene() = default;
		Scene(const SceneDescriptor&& desc);

		static Scene* Copy(Scene* other);
		static void Copy(Scene* src, Scene* dst);

		void Clear();

		Entity CreateEntity()
		{
			return CreateEntityWithUUID(Random::UInt64());
		}

		Entity CreateEntity(const std::string& tag)
		{
			return CreateEntityWithUUID(Random::UInt64(), tag);
		}

		Entity CreateEntityWithUUID(UUID uuid, const std::string& tag = "New Entity")
		{
			Entity entity = m_Registry.create();

			m_Registry.emplace<Component::Tag>(entity).Name = tag;
			m_Registry.emplace<Component::ID>(entity).Identifier = uuid;
			m_Registry.emplace<Component::Transform>(entity);

			m_EntityMap[uuid] = entity;

			return entity;
		}

		Entity FindEntityByUUID(UUID uuid)
		{
			if (m_EntityMap.find(uuid) != m_EntityMap.end())
			{
				return m_EntityMap.at(uuid);
			}

			return entt::null;
		}

		void DestroyEntity(Entity entity);

		Entity DuplicateEntity(Entity entity);

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

		void DeregisterSystem(const std::string& systemName)
		{
			ISystem* systemToBeDeleted = nullptr;

			for (ISystem* system : m_Systems)
			{
				if (system->Name == systemName)
				{
					systemToBeDeleted = system;
					break;
				}
			}

			DeregisterSystem(systemToBeDeleted);
		}

		void DeregisterSystem(ISystem* system);

		void RegisterSystem(ISystem* system, SystemType type = SystemType::Core)
		{
			system->SetType(type);
			system->SetContext(this);
			m_Systems.push_back(system);

			switch (type)
			{
			case HBL2::SystemType::Core:
				m_CoreSystems.push_back(system);
				break;
			case HBL2::SystemType::Runtime:
				m_RuntimeSystems.push_back(system);
				break;
			case HBL2::SystemType::User:
				m_RuntimeSystems.push_back(system);
				break;
			}
		}

		const std::vector<ISystem*>& GetSystems() const
		{
			return m_Systems;
		}

		const std::vector<ISystem*>& GetCoreSystems() const
		{
			return m_CoreSystems;
		}

		const std::vector<ISystem*>& GetRuntimeSystems() const
		{
			return m_RuntimeSystems;
		}

		entt::registry& GetRegistry()
		{
			return m_Registry;
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

		entt::meta_ctx& GetMetaContext()
		{
			return m_MetaContext;
		}

		const std::string& GetName() const
		{
			return m_Name;
		}

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

	private:
		void InternalDestroyEntity(Entity entity, bool isRootCall);

	private:
		std::string m_Name;
		entt::registry m_Registry;
		entt::meta_ctx m_MetaContext;
		std::vector<ISystem*> m_Systems;
		std::vector<ISystem*> m_CoreSystems;
		std::vector<ISystem*> m_RuntimeSystems;
		std::unordered_map<UUID, Entity> m_EntityMap;

		void operator=(const HBL2::Scene&);

		friend class Pool<Scene, Scene>;
	};
}