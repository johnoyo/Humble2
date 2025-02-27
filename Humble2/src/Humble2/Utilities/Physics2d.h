#pragma once

#include "Humble2API.h"
#include "Scene\Components.h"

#include <box2d\box2d.h>

namespace HBL2
{
	namespace Physics2D
	{
		enum class HBL2_API ContactEventType
		{
			BeginTouch = 0,
			EndTouch,
			Hit,
		};

		struct HBL2_API ContactBeginTouchEvent
		{
			entt::entity entityA;
			entt::entity entityB;
			b2ContactBeginTouchEvent* payload;
		};

		struct HBL2_API ContactEndTouchEvent
		{
			entt::entity entityA;
			entt::entity entityB;
			b2ContactEndTouchEvent* payload;
		};

		struct HBL2_API ContactHitEvent
		{
			entt::entity entityA;
			entt::entity entityB;
			b2ContactHitEvent* payload;
		};

		HBL2_API bool Equals(b2BodyId bodyA, b2BodyId bodyB);
		HBL2_API bool Equals(b2ShapeId shapeA, b2ShapeId shapeB);

		HBL2_API entt::entity GetEntityFromBodyId(b2BodyId body);
		HBL2_API entt::entity GetEntityFromShapeId(b2ShapeId shape);

		HBL2_API void ApplyForce(Component::Rigidbody2D& rb2d, const glm::vec2& force, bool wake);
		HBL2_API void ApplyForce(Component::Rigidbody2D& rb2d, const glm::vec2& force, const glm::vec2& worldPosition, bool wake);

		HBL2_API void ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, bool wake);
		HBL2_API void ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, const glm::vec2& worldPosition, bool wake);

		HBL2_API void ApplyAngularImpulse(Component::Rigidbody2D& rb2d, float impulse, bool wake);

		HBL2_API glm::vec2 GetLinearVelocity(Component::Rigidbody2D& rb2d);
		HBL2_API float GetAngularVelocity(Component::Rigidbody2D& rb2d);

		HBL2_API void OnBeginTouchEvent(std::function<void(ContactBeginTouchEvent*)>&& beginEventFunc);
		HBL2_API void OnEndTouchEvent(std::function<void(ContactEndTouchEvent*)>&& endEventFunc);
		HBL2_API void OnHitEvent(std::function<void(ContactHitEvent*)>&& hitEventFunc);

		HBL2_API void DispatchContactEvent(ContactEventType contactEventType, void* contactEventData);
		HBL2_API void ClearContactEvents();
	}
}