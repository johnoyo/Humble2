#pragma once

#include <entt.hpp>

namespace HBL2
{
	class Prefab
	{
	public:
		inline const entt::entity GetBaseEntity() const { return m_BaseEntity; }

	private:
		entt::entity m_BaseEntity;
	};
}