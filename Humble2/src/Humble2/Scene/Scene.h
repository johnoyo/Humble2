#pragma once

#include "ISystem.h"
#include "Components.h"

#include "Utilities\Random.h"

#include <entt.hpp>

namespace HBL2
{
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

		void Clear()
		{
			m_Registry.clear();

			entt::meta_reset(m_MetaContext);

			m_EntityMap.clear();

			for (ISystem* system : m_Systems)
			{
				if (system != nullptr)
				{
					delete system;
				}
			}

			m_Systems.clear();
			m_CoreSystems.clear();
			m_RuntimeSystems.clear();
		}

		entt::entity CreateEntity()
		{
			return CreateEntityWithUUID(Random::UInt64());
		}

		entt::entity CreateEntity(const std::string& tag)
		{
			return CreateEntityWithUUID(Random::UInt64(), tag);
		}

		entt::entity CreateEntityWithUUID(UUID uuid, const std::string& tag = "New Entity")
		{
			entt::entity entity = m_Registry.create();

			m_Registry.emplace<Component::Tag>(entity).Name = tag;
			m_Registry.emplace<Component::ID>(entity).Identifier = uuid;
			m_Registry.emplace<Component::Transform>(entity);

			m_EntityMap[uuid] = entity;

			return entity;
		}

		entt::entity FindEntityByUUID(UUID uuid)
		{
			if (m_EntityMap.find(uuid) != m_EntityMap.end())
			{
				return m_EntityMap.at(uuid);
			}

			return entt::null;
		}

		void DestroyEntity(entt::entity entity)
		{
			m_EntityMap.erase(m_Registry.get<Component::ID>(entity).Identifier);
			m_Registry.destroy(entity);
		}

		template<typename T>
		T& GetComponent(entt::entity entity)
		{
			return m_Registry.get<T>(entity);
		}

		template<typename T>
		bool HasComponent(entt::entity entity)
		{
			return m_Registry.try_get<T>(entity) == nullptr ? false : true;
		}

		template<typename T>
		T& AddComponent(entt::entity entity)
		{
			return m_Registry.emplace<T>(entity);
		}

		template<typename T>
		void RemoveComponent(entt::entity entity)
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

		entt::entity MainCamera = entt::null;

	private:
		std::string m_Name;
		entt::registry m_Registry;
		entt::meta_ctx m_MetaContext;
		std::vector<ISystem*> m_Systems;
		std::vector<ISystem*> m_CoreSystems;
		std::vector<ISystem*> m_RuntimeSystems;
		std::unordered_map<UUID, entt::entity> m_EntityMap;

		void operator=(const HBL2::Scene&);

		friend class Pool<Scene, Scene>;
	};
}