#include "Physics2d.h"

namespace HBL2
{
	static std::vector<std::function<void(Physics2D::ContactBeginTouchEvent*)>> s_BeginEventFunc;
	static std::vector<std::function<void(Physics2D::ContactEndTouchEvent*)>> s_EndEventFunc;
	static std::vector<std::function<void(Physics2D::ContactHitEvent*)>> s_HitEventFunc;

	static std::vector<std::function<void(Physics2D::SensorBeginTouchEvent*)>> s_BeginSensorEventFunc;
	static std::vector<std::function<void(Physics2D::SensorEndTouchEvent*)>> s_EndSensorEventFunc;

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

	void Physics2D::OnBeginTouchEvent(std::function<void(ContactBeginTouchEvent*)>&& beginEventFunc)
	{
		// TODO: This is not thread safe, add lock when system scheduling is added.
		s_BeginEventFunc.emplace_back(beginEventFunc);
	}

	void Physics2D::OnEndTouchEvent(std::function<void(ContactEndTouchEvent*)>&& endEventFunc)
	{
		// TODO: This is not thread safe, add lock when system scheduling is added.
		s_EndEventFunc.emplace_back(endEventFunc);
	}

	void Physics2D::OnHitEvent(std::function<void(ContactHitEvent*)>&& hitEventFunc)
	{
		// TODO: This is not thread safe, add lock when system scheduling is added.
		s_HitEventFunc.emplace_back(hitEventFunc);
	}

	void Physics2D::OnBeginSensorEvent(std::function<void(SensorBeginTouchEvent*)>&& beginEventFunc)
	{
		// TODO: This is not thread safe, add lock when system scheduling is added.
		s_BeginSensorEventFunc.emplace_back(beginEventFunc);
	}

	void Physics2D::OnEndSensorEvent(std::function<void(SensorEndTouchEvent*)>&& endEventFunc)
	{
		// TODO: This is not thread safe, add lock when system scheduling is added.
		s_EndSensorEventFunc.emplace_back(endEventFunc);
	}

	void Physics2D::DispatchContactEvent(ContactEventType contactEventType, void* contactEventData)
	{
		switch (contactEventType)
		{
		case HBL2::Physics2D::ContactEventType::BeginTouch:
			if (!s_BeginEventFunc.empty())
			{
				ContactBeginTouchEvent contactBeginTouchEvent{};
				contactBeginTouchEvent.payload = (b2ContactBeginTouchEvent*)contactEventData;
				contactBeginTouchEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(contactBeginTouchEvent.payload->shapeIdA));
				contactBeginTouchEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(contactBeginTouchEvent.payload->shapeIdB));

				for (const auto& callback : s_BeginEventFunc)
				{
					callback(&contactBeginTouchEvent);
				}
			}
			break;
		case HBL2::Physics2D::ContactEventType::EndTouch:
			if (!s_EndEventFunc.empty())
			{
				ContactEndTouchEvent contactEndTouchEvent{};
				contactEndTouchEvent.payload = (b2ContactEndTouchEvent*)contactEventData;
				contactEndTouchEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(contactEndTouchEvent.payload->shapeIdA));
				contactEndTouchEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(contactEndTouchEvent.payload->shapeIdB));
				
				for (const auto& callback : s_EndEventFunc)
				{
					callback(&contactEndTouchEvent);
				}
			}
			break;
		case HBL2::Physics2D::ContactEventType::Hit:
			if (!s_HitEventFunc.empty())
			{
				ContactHitEvent contactHitEvent{};
				contactHitEvent.payload = (b2ContactHitEvent*)contactEventData;
				contactHitEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(contactHitEvent.payload->shapeIdA));
				contactHitEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(contactHitEvent.payload->shapeIdB));
				
				for (const auto& callback : s_HitEventFunc)
				{
					callback(&contactHitEvent);
				}
			}
			break;
		}
	}

	HBL2_API void Physics2D::DispatchSensorEvent(ContactEventType sensorEventType, void* sensorEventData)
	{
		switch (sensorEventType)
		{
		case HBL2::Physics2D::ContactEventType::BeginTouch:
			if (!s_BeginSensorEventFunc.empty())
			{
				SensorBeginTouchEvent sensorBeginTouchEvent{};
				sensorBeginTouchEvent.payload = (b2SensorBeginTouchEvent*)sensorEventData;
				sensorBeginTouchEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(sensorBeginTouchEvent.payload->sensorShapeId));
				sensorBeginTouchEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(sensorBeginTouchEvent.payload->visitorShapeId));

				for (const auto& callback : s_BeginSensorEventFunc)
				{
					callback(&sensorBeginTouchEvent);
				}
			}
			break;
		case HBL2::Physics2D::ContactEventType::EndTouch:
			if (!s_EndSensorEventFunc.empty())
			{
				SensorEndTouchEvent sensorEndTouchEvent{};
				sensorEndTouchEvent.payload = (b2SensorEndTouchEvent*)sensorEventData;
				sensorEndTouchEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(sensorEndTouchEvent.payload->sensorShapeId));
				sensorEndTouchEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(sensorEndTouchEvent.payload->visitorShapeId));

				for (const auto& callback : s_EndSensorEventFunc)
				{
					callback(&sensorEndTouchEvent);
				}
			}
			break;
		}
	}

	HBL2_API void Physics2D::ClearContactEvents()
	{
		s_BeginEventFunc.clear();
		s_EndEventFunc.clear();
		s_HitEventFunc.clear();
	}

	HBL2_API void Physics2D::ClearSensorEvents()
	{
		s_BeginSensorEventFunc.clear();
		s_EndSensorEventFunc.clear();
	}
}
