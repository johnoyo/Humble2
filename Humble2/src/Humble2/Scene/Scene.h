#pragma once

#include "ISystem.h"
#include "Components.h"

#include <entt.hpp>

namespace HBL2
{
	class Scene
	{
	public:
		Scene(const std::string& name);
		~Scene();

		static Scene* Copy(Scene* other);

		entt::entity CreateEntity()
		{
			entt::entity entity = m_Registry.create();

			m_Registry.emplace<Component::Tag>(entity);
			m_Registry.emplace<Component::Transform>(entity);

			return entity;
		}

		template<typename T>
		T& GetComponent(entt::entity entity)
		{
			return m_Registry.get<T>(entity);
		}

		template<typename T>
		T& AddComponent(entt::entity entity)
		{
			return m_Registry.emplace<T>(entity);
		}

		void RegisterSystem(ISystem* system)
		{
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

		entt::entity MainCamera = entt::null;

	private:
		std::string m_Name;
		entt::registry m_Registry;
		std::vector<ISystem*> m_Systems;
	};
}