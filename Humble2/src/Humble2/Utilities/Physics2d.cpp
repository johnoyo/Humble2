#include "Physics2d.h"

namespace HBL2
{
	static std::vector<std::function<void(Physics2D::ContactBeginTouchEvent*)>> s_BeginEventFunc;
	static std::vector<std::function<void(Physics2D::ContactEndTouchEvent*)>> s_EndEventFunc;
	static std::vector<std::function<void(Physics2D::ContactHitEvent*)>> s_HitEventFunc;

	bool Physics2D::Equals(b2BodyId bodyA, b2BodyId bodyB)
	{
		return (bodyA.index1 == bodyB.index1 && bodyA.generation == bodyB.generation && bodyA.world0 == bodyB.world0);
	}

	bool Physics2D::Equals(b2ShapeId shapeA, b2ShapeId shapeB)
	{
		return (shapeA.index1 == shapeB.index1 && shapeA.generation == shapeB.generation && shapeA.world0 == shapeB.world0);
	}

	entt::entity Physics2D::GetEntityFromBodyId(b2BodyId body)
	{
		return static_cast<entt::entity>(reinterpret_cast<intptr_t>(b2Body_GetUserData(body)));
	}

	entt::entity Physics2D::GetEntityFromShapeId(b2ShapeId shape)
	{
		return static_cast<entt::entity>(reinterpret_cast<intptr_t>(b2Shape_GetUserData(shape)));
	}

	void Physics2D::ApplyForce(Component::Rigidbody2D& rb2d, const glm::vec2& force, bool wake)
	{
		b2Body_ApplyForceToCenter(rb2d.BodyId, { force.x, force.y }, wake);
	}

	void Physics2D::ApplyForce(Component::Rigidbody2D& rb2d, const glm::vec2& force, const glm::vec2& worldPosition, bool wake)
	{
		b2Body_ApplyForce(rb2d.BodyId, { force.x, force.y }, { worldPosition.x, worldPosition.y }, wake);
	}

	void Physics2D::ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, bool wake)
	{
		b2Body_ApplyLinearImpulseToCenter(rb2d.BodyId, { velocity.x, velocity.y }, wake);
	}

	void Physics2D::ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, const glm::vec2& worldPosition, bool wake)
	{
		b2Body_ApplyLinearImpulse(rb2d.BodyId, { velocity.x, velocity.y }, { worldPosition.x, worldPosition.y }, wake);
	}

	void Physics2D::ApplyAngularImpulse(Component::Rigidbody2D& rb2d, float impulse, bool wake)
	{
		b2Body_ApplyAngularImpulse(rb2d.BodyId, impulse, wake);
	}

	glm::vec2 Physics2D::GetLinearVelocity(Component::Rigidbody2D& rb2d)
	{
		const auto& linearVelocity = b2Body_GetLinearVelocity(rb2d.BodyId);
		return { linearVelocity.x, linearVelocity.y };
	}

	float Physics2D::GetAngularVelocity(Component::Rigidbody2D& rb2d)
	{
		return b2Body_GetAngularVelocity(rb2d.BodyId);
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

	void Physics2D::DispatchContactEvent(ContactEventType contactEventType, void* contactEventData)
	{
		switch (contactEventType)
		{
		case HBL2::Physics2D::ContactEventType::BeginTouch:
			if (!s_BeginEventFunc.empty())
			{
				ContactBeginTouchEvent contactBeginTouchEvent{};
				contactBeginTouchEvent.payload = (b2ContactBeginTouchEvent*)contactEventData;
				contactBeginTouchEvent.entityA = GetEntityFromShapeId(contactBeginTouchEvent.payload->shapeIdA);
				contactBeginTouchEvent.entityB = GetEntityFromShapeId(contactBeginTouchEvent.payload->shapeIdB);

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
				contactEndTouchEvent.entityA = GetEntityFromShapeId(contactEndTouchEvent.payload->shapeIdA);
				contactEndTouchEvent.entityB = GetEntityFromShapeId(contactEndTouchEvent.payload->shapeIdB);
				
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
				contactHitEvent.entityA = GetEntityFromShapeId(contactHitEvent.payload->shapeIdA);
				contactHitEvent.entityB = GetEntityFromShapeId(contactHitEvent.payload->shapeIdB);
				
				for (const auto& callback : s_HitEventFunc)
				{
					callback(&contactHitEvent);
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
}
