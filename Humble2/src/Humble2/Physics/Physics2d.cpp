#include "Physics2d.h"

namespace HBL2
{
	static std::vector<std::function<void(Physics2D::CollisionEnterEvent*)>> g_CollisionEnterEvents;
	static std::vector<std::function<void(Physics2D::CollisionExitEvent*)>> g_CollisionExitEvents;
	static std::vector<std::function<void(Physics2D::CollisionHitEvent*)>> g_CollisionHitEvents;

	static std::vector<std::function<void(Physics2D::TriggerEnterEvent*)>> g_TriggerEnterEvents;
	static std::vector<std::function<void(Physics2D::TriggerExitEvent*)>> g_TriggerExitEvents;

	bool Physics2D::BodiesAreEqual(Physics::ID bodyA, Physics::ID bodyB)
	{
		b2BodyId b2BodyIdA = b2LoadBodyId(bodyA);
		b2BodyId b2BodyIdB = b2LoadBodyId(bodyB);
		return B2_ID_EQUALS(b2BodyIdA, b2BodyIdB);
	}

	bool Physics2D::ShapesAreEqual(Physics::ID shapeA, Physics::ID shapeB)
	{
		b2ShapeId b2ShapeIdB = b2LoadShapeId(shapeB);
		b2ShapeId b2ShapeIdA = b2LoadShapeId(shapeA);
		return B2_ID_EQUALS(b2ShapeIdA, b2ShapeIdB);
	}

	entt::entity Physics2D::GetEntityFromBodyId(Physics::ID body)
	{
		return static_cast<entt::entity>(reinterpret_cast<intptr_t>(b2Body_GetUserData(b2LoadBodyId(body))));
	}

	entt::entity Physics2D::GetEntityFromShapeId(Physics::ID shape)
	{
		return static_cast<entt::entity>(reinterpret_cast<intptr_t>(b2Shape_GetUserData(b2LoadShapeId(shape))));
	}

	void Physics2D::ApplyForce(Component::Rigidbody2D& rb2d, const glm::vec2& force, bool wake)
	{
		b2Body_ApplyForceToCenter(b2LoadBodyId(rb2d.BodyId), { force.x, force.y }, wake);
	}

	void Physics2D::ApplyForce(Component::Rigidbody2D& rb2d, const glm::vec2& force, const glm::vec2& worldPosition, bool wake)
	{
		b2Body_ApplyForce(b2LoadBodyId(rb2d.BodyId), { force.x, force.y }, { worldPosition.x, worldPosition.y }, wake);
	}

	void Physics2D::ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, bool wake)
	{
		b2Body_ApplyLinearImpulseToCenter(b2LoadBodyId(rb2d.BodyId), { velocity.x, velocity.y }, wake);
	}

	void Physics2D::ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, const glm::vec2& worldPosition, bool wake)
	{
		b2Body_ApplyLinearImpulse(b2LoadBodyId(rb2d.BodyId), { velocity.x, velocity.y }, { worldPosition.x, worldPosition.y }, wake);
	}

	void Physics2D::ApplyAngularImpulse(Component::Rigidbody2D& rb2d, float impulse, bool wake)
	{
		b2Body_ApplyAngularImpulse(b2LoadBodyId(rb2d.BodyId), impulse, wake);
	}

	glm::vec2 Physics2D::GetLinearVelocity(Component::Rigidbody2D& rb2d)
	{
		const auto& linearVelocity = b2Body_GetLinearVelocity(b2LoadBodyId(rb2d.BodyId));
		return { linearVelocity.x, linearVelocity.y };
	}

	float Physics2D::GetAngularVelocity(Component::Rigidbody2D& rb2d)
	{
		return b2Body_GetAngularVelocity(b2LoadBodyId(rb2d.BodyId));
	}

	void Physics2D::OnCollisionEnterEvent(std::function<void(CollisionEnterEvent*)>&& beginEventFunc)
	{
		// TODO: This is not thread safe, add lock when system scheduling is added.
		g_CollisionEnterEvents.emplace_back(beginEventFunc);
	}

	void Physics2D::OnCollisionExitEvent(std::function<void(CollisionExitEvent*)>&& endEventFunc)
	{
		// TODO: This is not thread safe, add lock when system scheduling is added.
		g_CollisionExitEvents.emplace_back(endEventFunc);
	}

	void Physics2D::OnCollisionHitEvent(std::function<void(CollisionHitEvent*)>&& hitEventFunc)
	{
		// TODO: This is not thread safe, add lock when system scheduling is added.
		g_CollisionHitEvents.emplace_back(hitEventFunc);
	}

	void Physics2D::OnTriggerEnterEvent(std::function<void(TriggerEnterEvent*)>&& beginEventFunc)
	{
		// TODO: This is not thread safe, add lock when system scheduling is added.
		g_TriggerEnterEvents.emplace_back(beginEventFunc);
	}

	void Physics2D::OnTriggerExitEvent(std::function<void(TriggerExitEvent*)>&& endEventFunc)
	{
		// TODO: This is not thread safe, add lock when system scheduling is added.
		g_TriggerExitEvents.emplace_back(endEventFunc);
	}

	void Physics2D::DispatchCollisionEvent(ContactEventType contactEventType, void* collisionEventData)
	{
		switch (contactEventType)
		{
		case HBL2::Physics2D::ContactEventType::BeginTouch:
			if (!g_CollisionEnterEvents.empty())
			{
				CollisionEnterEvent contactBeginTouchEvent{};
				contactBeginTouchEvent.payload = (b2ContactBeginTouchEvent*)collisionEventData;
				contactBeginTouchEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(contactBeginTouchEvent.payload->shapeIdA));
				contactBeginTouchEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(contactBeginTouchEvent.payload->shapeIdB));

				for (const auto& callback : g_CollisionEnterEvents)
				{
					callback(&contactBeginTouchEvent);
				}
			}
			break;
		case HBL2::Physics2D::ContactEventType::EndTouch:
			if (!g_CollisionExitEvents.empty())
			{
				CollisionExitEvent contactEndTouchEvent{};
				contactEndTouchEvent.payload = (b2ContactEndTouchEvent*)collisionEventData;
				contactEndTouchEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(contactEndTouchEvent.payload->shapeIdA));
				contactEndTouchEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(contactEndTouchEvent.payload->shapeIdB));
				
				for (const auto& callback : g_CollisionExitEvents)
				{
					callback(&contactEndTouchEvent);
				}
			}
			break;
		case HBL2::Physics2D::ContactEventType::Hit:
			if (!g_CollisionHitEvents.empty())
			{
				CollisionHitEvent contactHitEvent{};
				contactHitEvent.payload = (b2ContactHitEvent*)collisionEventData;
				contactHitEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(contactHitEvent.payload->shapeIdA));
				contactHitEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(contactHitEvent.payload->shapeIdB));
				
				for (const auto& callback : g_CollisionHitEvents)
				{
					callback(&contactHitEvent);
				}
			}
			break;
		}
	}

	void Physics2D::DispatchTriggerEvent(ContactEventType sensorEventType, void* triggerEventData)
	{
		switch (sensorEventType)
		{
		case HBL2::Physics2D::ContactEventType::BeginTouch:
			if (!g_TriggerEnterEvents.empty())
			{
				TriggerEnterEvent sensorBeginTouchEvent{};
				sensorBeginTouchEvent.payload = (b2SensorBeginTouchEvent*)triggerEventData;
				sensorBeginTouchEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(sensorBeginTouchEvent.payload->sensorShapeId));
				sensorBeginTouchEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(sensorBeginTouchEvent.payload->visitorShapeId));

				for (const auto& callback : g_TriggerEnterEvents)
				{
					callback(&sensorBeginTouchEvent);
				}
			}
			break;
		case HBL2::Physics2D::ContactEventType::EndTouch:
			if (!g_TriggerExitEvents.empty())
			{
				TriggerExitEvent sensorEndTouchEvent{};
				sensorEndTouchEvent.payload = (b2SensorEndTouchEvent*)triggerEventData;
				sensorEndTouchEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(sensorEndTouchEvent.payload->sensorShapeId));
				sensorEndTouchEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(sensorEndTouchEvent.payload->visitorShapeId));

				for (const auto& callback : g_TriggerExitEvents)
				{
					callback(&sensorEndTouchEvent);
				}
			}
			break;
		}
	}

	void Physics2D::ClearCollisionEvents()
	{
		g_CollisionEnterEvents.clear();
		g_CollisionExitEvents.clear();
		g_CollisionHitEvents.clear();
	}

	void Physics2D::ClearTriggerEvents()
	{
		g_TriggerEnterEvents.clear();
		g_TriggerExitEvents.clear();
	}
}
