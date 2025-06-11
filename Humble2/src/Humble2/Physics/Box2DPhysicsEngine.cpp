#include "Box2DPhysicsEngine.h"

namespace HBL2
{
	void Box2DPhysicsEngine::Initialize()
	{
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = { 0, m_GravityForce };
		worldDef.restitutionThreshold = 0.5f;

		m_PhysicsWorld = b2CreateWorld(&worldDef);
	}

	void Box2DPhysicsEngine::Step(float timeStep)
	{
		b2World_Step(m_PhysicsWorld, timeStep, m_SubStepCount);
	}

	void Box2DPhysicsEngine::Shutdown()
	{
		// Clear collision events.
		m_CollisionEnterEvents.clear();
		m_CollisionExitEvents.clear();
		m_CollisionHitEvents.clear();

		// Clear trigger events.
		m_TriggerEnterEvents.clear();
		m_TriggerExitEvents.clear();

		if (b2World_IsValid(m_PhysicsWorld))
		{
			b2DestroyWorld(m_PhysicsWorld);
		}
	}

	void Box2DPhysicsEngine::DispatchCollisionEvent(Physics::CollisionEventType collisionEventType, void* collisionEventData)
	{
		switch (collisionEventType)
		{
		case Physics::CollisionEventType::Enter:
			if (!m_CollisionEnterEvents.empty())
			{
				Physics::CollisionEnterEvent* collisionEnterEvent = (Physics::CollisionEnterEvent*)collisionEventData;

				for (const auto& callback : m_CollisionEnterEvents)
				{
					callback(collisionEnterEvent);
				}
			}
			break;
		case Physics::CollisionEventType::Exit:
			if (!m_CollisionExitEvents.empty())
			{
				Physics::CollisionExitEvent* collisionExitEvent = (Physics::CollisionExitEvent*)collisionEventData;

				for (const auto& callback : m_CollisionExitEvents)
				{
					callback(collisionExitEvent);
				}
			}
			break;
		case Physics::CollisionEventType::Hit:
			if (!m_CollisionHitEvents.empty())
			{
				Physics::CollisionHitEvent* collisionHitEvent = (Physics::CollisionHitEvent*)collisionEventData;

				for (const auto& callback : m_CollisionHitEvents)
				{
					callback(collisionHitEvent);
				}
			}
			break;
		}
	}

	void Box2DPhysicsEngine::DispatchTriggerEvent(Physics::CollisionEventType collisionEventType, void* triggerEventData)
	{
		switch (collisionEventType)
		{
		case Physics::CollisionEventType::Enter:
			if (!m_TriggerEnterEvents.empty())
			{
				Physics::TriggerEnterEvent* triggerEnterEvent = (Physics::TriggerEnterEvent*)triggerEventData;

				for (const auto& callback : m_TriggerEnterEvents)
				{
					callback(triggerEnterEvent);
				}
			}
			break;
		case Physics::CollisionEventType::Exit:
			if (!m_TriggerExitEvents.empty())
			{
				Physics::TriggerExitEvent* triggerExitEvent = (Physics::TriggerExitEvent*)triggerEventData;

				for (const auto& callback : m_TriggerExitEvents)
				{
					callback(triggerExitEvent);
				}
			}
			break;
		}
	}

	void Box2DPhysicsEngine::OnCollisionEnterEvent(std::function<void(Physics::CollisionEnterEvent*)>&& enterEventFunc)
	{
		m_CollisionEnterEvents.emplace_back(enterEventFunc);
	}

	void Box2DPhysicsEngine::OnCollisionStayEvent(std::function<void(Physics::CollisionStayEvent*)>&& stayEventFunc)
	{
		// Unused in box2d
	}

	void Box2DPhysicsEngine::OnCollisionExitEvent(std::function<void(Physics::CollisionExitEvent*)>&& exitEventFunc)
	{
		m_CollisionExitEvents.emplace_back(exitEventFunc);
	}

	void Box2DPhysicsEngine::OnCollisionHitEvent(std::function<void(Physics::CollisionHitEvent*)>&& hitEventFunc)
	{
		m_CollisionHitEvents.emplace_back(hitEventFunc);
	}

	void Box2DPhysicsEngine::OnTriggerEnterEvent(std::function<void(Physics::TriggerEnterEvent*)>&& enterEventFunc)
	{
		m_TriggerEnterEvents.emplace_back(enterEventFunc);
	}

	void Box2DPhysicsEngine::OnTriggerStayEvent(std::function<void(Physics::TriggerStayEvent*)>&& stayEventFunc)
	{
		// Unused in box2d
	}

	void Box2DPhysicsEngine::OnTriggerExitEvent(std::function<void(Physics::TriggerExitEvent*)>&& exitEventFunc)
	{
		m_TriggerExitEvents.emplace_back(exitEventFunc);
	}

	void Box2DPhysicsEngine::Teleport(Component::Rigidbody2D& rb2d, const glm::vec2& position, glm::vec2& rotation)
	{
		b2Body_SetTransform(b2LoadBodyId(rb2d.BodyId), { position.x, position.y }, { rotation.x, rotation.y });
	}

	void Box2DPhysicsEngine::ApplyForce(Component::Rigidbody2D& rb2d, const glm::vec2& force, bool wake)
	{
		b2Body_ApplyForceToCenter(b2LoadBodyId(rb2d.BodyId), { force.x, force.y }, wake);
	}

	void Box2DPhysicsEngine::ApplyForce(Component::Rigidbody2D& rb2d, const glm::vec2& force, const glm::vec2& worldPosition, bool wake)
	{
		b2Body_ApplyForce(b2LoadBodyId(rb2d.BodyId), { force.x, force.y }, { worldPosition.x, worldPosition.y }, wake);
	}

	void Box2DPhysicsEngine::ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, bool wake)
	{
		b2Body_ApplyLinearImpulseToCenter(b2LoadBodyId(rb2d.BodyId), { velocity.x, velocity.y }, wake);
	}

	void Box2DPhysicsEngine::ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, const glm::vec2& worldPosition, bool wake)
	{
		b2Body_ApplyLinearImpulse(b2LoadBodyId(rb2d.BodyId), { velocity.x, velocity.y }, { worldPosition.x, worldPosition.y }, wake);
	}

	void Box2DPhysicsEngine::ApplyAngularImpulse(Component::Rigidbody2D& rb2d, float impulse, bool wake)
	{
		b2Body_ApplyAngularImpulse(b2LoadBodyId(rb2d.BodyId), impulse, wake);
	}

	void Box2DPhysicsEngine::SetLinearVelocity(Component::Rigidbody2D& rb2d, const glm::vec2& velocity)
	{
		b2Body_SetLinearVelocity(b2LoadBodyId(rb2d.BodyId), { velocity.x, velocity.y });
	}

	glm::vec2 Box2DPhysicsEngine::GetLinearVelocity(Component::Rigidbody2D& rb2d)
	{
		const auto& linearVelocity = b2Body_GetLinearVelocity(b2LoadBodyId(rb2d.BodyId));
		return { linearVelocity.x, linearVelocity.y };
	}

	void Box2DPhysicsEngine::SetAngularVelocity(Component::Rigidbody2D& rb2d, float velocity)
	{
		b2Body_SetAngularVelocity(b2LoadBodyId(rb2d.BodyId), velocity);
	}

	float Box2DPhysicsEngine::GetAngularVelocity(Component::Rigidbody2D& rb2d)
	{
		return b2Body_GetAngularVelocity(b2LoadBodyId(rb2d.BodyId));
	}
}
