#pragma once

#include <Scene/Entity.h>

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
			Entity entityA;
			Entity entityB;
		};

		struct CollisionStayEvent
		{
			Entity entityA;
			Entity entityB;
		};

		struct CollisionExitEvent
		{
			Entity entityA;
			Entity entityB;
		};

		struct CollisionHitEvent
		{
			Entity entityA;
			Entity entityB;
		};

		struct TriggerEnterEvent
		{
			Entity entityA;
			Entity entityB;
		};

		struct TriggerStayEvent
		{
			Entity entityA;
			Entity entityB;
		};

		struct TriggerExitEvent
		{
			Entity entityA;
			Entity entityB;
		};
	}
}