#pragma once

#include "ISystem.h"
#include "Components.h"

#include <entt.hpp>

namespace HBL2
{
	struct SceneDescriptor
	{
		std::string name;
		std::filesystem::path path;
	};

	class Scene
	{
	public:
		Scene() = default;
		Scene(const SceneDescriptor& desc);
		~Scene();

		static Scene* Copy(Scene* other);

		HBL2::Scene& operator=(const HBL2::Scene&);

		entt::entity CreateEntity()
		{
			entt::entity entity = m_Registry.create();

			m_Registry.emplace<Component::Tag>(entity);
			m_Registry.emplace<Component::Transform>(entity);

			return entity;
		}

		entt::entity CreateEntity(const std::string& tag)
		{
			entt::entity entity = m_Registry.create();

			m_Registry.emplace<Component::Tag>(entity).Name = tag;
			m_Registry.emplace<Component::Transform>(entity);

			return entity;
		}

		void DestroyEntity(entt::entity entity)
		{
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

		void RegisterSystem(ISystem* system)
		{
			system->SetContext(this);
			m_Systems.push_back(system);
		}

		const std::vector<ISystem*>& GetSystems() const
		{
			return m_Systems;
		}

		entt::registry& GetRegistry()
		{
			return m_Registry;
		}

		const std::string& GetName() const
		{
			return m_Name;
		}

		entt::entity MainCamera = entt::null;

	private:
		std::string m_Name;
		entt::registry m_Registry;
		std::vector<ISystem*> m_Systems;
	};
}