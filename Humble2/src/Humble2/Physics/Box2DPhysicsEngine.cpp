#include "Box2DPhysicsEngine.h"

#include "Core\Time.h"

namespace HBL2
{
	static Entity GetEntityFromBodyId(Physics::ID body)
	{
		return static_cast<uint32_t>(reinterpret_cast<intptr_t>(b2Body_GetUserData(b2LoadBodyId(body))));
	}

	static Entity GetEntityFromShapeId(Physics::ID shape)
	{
		return static_cast<uint32_t>(reinterpret_cast<intptr_t>(b2Shape_GetUserData(b2LoadShapeId(shape))));
	}

	static b2BodyType BodyTypeTob2BodyType(Physics::BodyType bodyType)
	{
		switch (bodyType)
		{
		case Physics::BodyType::Static:    return b2BodyType::b2_staticBody;
		case Physics::BodyType::Kinematic: return b2BodyType::b2_kinematicBody;
		case Physics::BodyType::Dynamic:   return b2BodyType::b2_dynamicBody;
		}

		HBL2_CORE_ASSERT(false, "Unknown Rigidbody2D type.");
		return b2BodyType::b2_staticBody;
	}

	void Box2DPhysicsEngine::Initialize(Scene* ctx)
	{
		m_Context = ctx;

		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = { 0, m_GravityForce };
		worldDef.restitutionThreshold = 0.5f;

		m_PhysicsWorld = b2CreateWorld(&worldDef);

		m_DebugDraw = b2DefaultDebugDraw();
		// TODO: Attach debug draw methods to function pointers.

		m_Context->Group<Component::Rigidbody2D>(Get<Component::Transform>)
			.Each([this](Entity entity, Component::Rigidbody2D& rb2d, Component::Transform& transform)
			{
				rb2d.BodyId = CreateRigidbody(entity, rb2d, transform);

				if (m_Context->HasComponent<Component::BoxCollider2D>(entity))
				{
					Component::BoxCollider2D& bc2d = m_Context->GetComponent<Component::BoxCollider2D>(entity);
					bc2d.ShapeId = CreateBoxCollider(entity, bc2d, rb2d, transform);
				}
			});
	}

	void Box2DPhysicsEngine::Update()
	{
		// Handle runtime creations and properties update.
		m_Context->Group<Component::Rigidbody2D>(Get<Component::Transform>)
			.Each([this](Entity entity, Component::Rigidbody2D& rb2d, Component::Transform& transform)
				{
					// Create rigidbody if it was added and is uninitialized.
					if (rb2d.BodyId == Physics::InvalidID)
					{
						rb2d.BodyId = CreateRigidbody(entity, rb2d, transform);
					}

					// Update body type if its marked dirty.
					b2BodyId bodyId = b2LoadBodyId(rb2d.BodyId);
					if (rb2d.Dirty)
					{
						b2Body_SetType(bodyId, BodyTypeTob2BodyType(rb2d.Type));
						b2Body_SetAwake(bodyId, true);
						rb2d.Dirty = false;
					}

					if (m_Context->HasComponent<Component::BoxCollider2D>(entity))
					{
						Component::BoxCollider2D& bc2d = m_Context->GetComponent<Component::BoxCollider2D>(entity);

						// Create collider if it was added and is uninitialized.
						if (bc2d.ShapeId == Physics::InvalidID)
						{
							bc2d.ShapeId = CreateBoxCollider(entity, bc2d, rb2d, transform);
						}

						// Update shape properties.
						b2ShapeId shapeId = b2LoadShapeId(bc2d.ShapeId);

						b2Shape_SetFriction(shapeId, bc2d.Friction);
						b2Shape_SetRestitution(shapeId, bc2d.Restitution);
						b2Shape_SetDensity(shapeId, bc2d.Density, true);

						// Update shape polygon (collider) if its marked dirty.
						if (bc2d.Dirty)
						{
							const b2Polygon& polygon = b2MakeBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y);
							b2Shape_SetPolygon(shapeId, &polygon);
							bc2d.Dirty = false;
						}
					}
				});

		// Progress the simulation.
		b2World_Step(m_PhysicsWorld, Time::FixedTimeStep, m_SubStepCount);

		// Update the transform of rigidbodies. Consider using b2World_GetBodyEvents.
		m_Context->Group<Component::Rigidbody2D>(Get<Component::Transform>)
			.Each([](Entity entity, Component::Rigidbody2D& rb2d, Component::Transform& transform)
			{
				b2BodyId bodyId = b2LoadBodyId(rb2d.BodyId);

				const auto& position = b2Body_GetPosition(bodyId);
				transform.Translation.x = position.x;
				transform.Translation.y = position.y;

				const auto& rotation = b2Body_GetRotation(bodyId);
				transform.Rotation.z = glm::degrees(b2Rot_GetAngle(rotation));
			});

		// Dispatch any events that occured during this simulation step.
		b2ContactEvents contactEvents = b2World_GetContactEvents(m_PhysicsWorld);

		for (int i = 0; i < contactEvents.beginCount; ++i)
		{
			b2ContactBeginTouchEvent* beginEvent = contactEvents.beginEvents + i;

			Physics::CollisionEnterEvent collisionEnterEvent{};
			collisionEnterEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(beginEvent->shapeIdA));
			collisionEnterEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(beginEvent->shapeIdB));

			DispatchCollisionEvent(Physics::CollisionEventType::Enter, &collisionEnterEvent);
		}

		for (int i = 0; i < contactEvents.endCount; ++i)
		{
			b2ContactEndTouchEvent* endEvent = contactEvents.endEvents + i;

			Physics::CollisionEnterEvent collisionExitEvent{};
			collisionExitEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(endEvent->shapeIdA));
			collisionExitEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(endEvent->shapeIdB));

			DispatchCollisionEvent(Physics::CollisionEventType::Exit, &collisionExitEvent);
		}

		for (int i = 0; i < contactEvents.hitCount; ++i)
		{
			b2ContactHitEvent* hitEvent = contactEvents.hitEvents + i;

			Physics::CollisionHitEvent collisionHitEvent{};
			collisionHitEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(hitEvent->shapeIdA));
			collisionHitEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(hitEvent->shapeIdB));

			DispatchCollisionEvent(Physics::CollisionEventType::Hit, &collisionHitEvent);
		}

		// Dispatch any sensor events that occured during this simulation step.
		b2SensorEvents sensorEvents = b2World_GetSensorEvents(m_PhysicsWorld);

		for (int i = 0; i < sensorEvents.beginCount; ++i)
		{
			b2SensorBeginTouchEvent* beginEvent = sensorEvents.beginEvents + i;

			Physics::TriggerEnterEvent triggerEnterEvent{};
			triggerEnterEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(beginEvent->sensorShapeId));
			triggerEnterEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(beginEvent->visitorShapeId));

			DispatchTriggerEvent(Physics::CollisionEventType::Enter, &triggerEnterEvent);
		}

		for (int i = 0; i < sensorEvents.endCount; ++i)
		{
			b2SensorEndTouchEvent* endEvent = sensorEvents.endEvents + i;

			Physics::TriggerExitEvent triggerExitEvent{};
			triggerExitEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(endEvent->sensorShapeId));
			triggerExitEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(endEvent->visitorShapeId));

			DispatchTriggerEvent(Physics::CollisionEventType::Exit, &triggerExitEvent);
		}
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

	void Box2DPhysicsEngine::SetDebugDrawEnabled(bool enabled)
	{
		m_DebugDrawEnabled = enabled;
	}

	void Box2DPhysicsEngine::OnDebugDraw()
	{
		if (!m_DebugDrawEnabled || B2_IS_NULL(m_PhysicsWorld))
		{
			return;
		}

		b2World_Draw(m_PhysicsWorld, &m_DebugDraw);
	}

	Physics::ID Box2DPhysicsEngine::CreateRigidbody(Entity entity, Component::Rigidbody2D& rb2d, Component::Transform& transform)
	{
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = BodyTypeTob2BodyType(rb2d.Type);
		bodyDef.position = { transform.Translation.x, transform.Translation.y };
		bodyDef.rotation = b2MakeRot(glm::radians(transform.Rotation.z));
		bodyDef.fixedRotation = rb2d.FixedRotation; // bodyDef.motionLocks.angularZ = rb2d.FixedRotation;
		bodyDef.userData = reinterpret_cast<void*>(static_cast<intptr_t>((uint64_t)entity));

		b2BodyId bodyId = b2CreateBody(m_PhysicsWorld, &bodyDef);
		return b2StoreBodyId(bodyId);
	}

	Physics::ID Box2DPhysicsEngine::CreateBoxCollider(Entity entity, Component::BoxCollider2D& bc2d, Component::Rigidbody2D& rb2d, Component::Transform& transform)
	{
		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.material.friction = bc2d.Friction;
		shapeDef.material.restitution = bc2d.Restitution;
		shapeDef.density = bc2d.Density;
		shapeDef.enableContactEvents = !bc2d.Trigger;
		shapeDef.enableSensorEvents = bc2d.Trigger;
		shapeDef.isSensor = bc2d.Trigger;
		shapeDef.userData = reinterpret_cast<void*>(static_cast<intptr_t>((uint64_t)entity));

		const b2Polygon polygon = b2MakeBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y);

		b2ShapeId shapeId = b2CreatePolygonShape(b2LoadBodyId(rb2d.BodyId), &shapeDef, &polygon);
		return b2StoreShapeId(shapeId);
	}
}
