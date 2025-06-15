#pragma once

#include <entt.hpp>

#include <stdint.h>

namespace HBL2
{
	namespace Physics
	{
		enum class BodyType
		{
			Static = 0,
			Kinematic,
			Dynamic,
		};

		using ID = uint64_t;

		static constexpr uint64_t InvalidID = UINT64_MAX;

		enum class CollisionEventType
		{
			Enter = 0,
			Stay,
			Exit,
			Hit,
		};

		struct CollisionEnterEvent
		{
			entt::entity entityA;
			entt::entity entityB;
		};

		struct CollisionStayEvent
		{
			entt::entity entityA;
			entt::entity entityB;
		};

		struct CollisionExitEvent
		{
			entt::entity entityA;
			entt::entity entityB;
		};

		struct CollisionHitEvent
		{
			entt::entity entityA;
			entt::entity entityB;
		};

		struct TriggerEnterEvent
		{
			entt::entity entityA;
			entt::entity entityB;
		};

		struct TriggerStayEvent
		{
			entt::entity entityA;
			entt::entity entityB;
		};

		struct TriggerExitEvent
		{
			entt::entity entityA;
			entt::entity entityB;
		};
	}
}