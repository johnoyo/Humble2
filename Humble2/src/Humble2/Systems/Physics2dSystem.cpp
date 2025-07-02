#include "Physics2dSystem.h"

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

	void Physics2dSystem::OnCreate()
	{
		PhysicsEngine2D::Instance = new Box2DPhysicsEngine;
		m_PhysicsEngine = (Box2DPhysicsEngine*)PhysicsEngine2D::Instance;
		m_PhysicsEngine->Initialize();
		m_PhysicsWorld = m_PhysicsEngine->Get();		

		m_Context->Group<Component::Rigidbody2D>(Get<Component::Transform>)
			.Each([&](Entity entity, Component::Rigidbody2D& rb2d, Component::Transform& transform)
			{
				rb2d.BodyId = CreateRigidbody(entity, rb2d, transform);

				if (m_Context->HasComponent<Component::BoxCollider2D>(entity))
				{
					Component::BoxCollider2D& bc2d = m_Context->GetComponent<Component::BoxCollider2D>(entity);
					bc2d.ShapeId = CreateBoxCollider(entity, bc2d, rb2d, transform);
				}
			});

		m_Initialized = true;
	}

	void Physics2dSystem::OnFixedUpdate()
	{
		// Handle runtime creations and properties update.
		m_Context->Group<Component::Rigidbody2D>(Get<Component::Transform>)
			.Each([&](Entity entity, Component::Rigidbody2D& rb2d, Component::Transform& transform)
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
		m_PhysicsEngine->Step(Time::FixedTimeStep);

		// Update the transform of rigidbodies. Consider using b2World_GetBodyEvents.
		m_Context->Group<Component::Rigidbody2D>(Get<Component::Transform>)
			.Each([&](Entity entity, Component::Rigidbody2D& rb2d, Component::Transform& transform)
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

			m_PhysicsEngine->DispatchCollisionEvent(Physics::CollisionEventType::Enter, &collisionEnterEvent);
		}

		for (int i = 0; i < contactEvents.endCount; ++i)
		{
			b2ContactEndTouchEvent* endEvent = contactEvents.endEvents + i;

			Physics::CollisionEnterEvent collisionExitEvent{};
			collisionExitEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(endEvent->shapeIdA));
			collisionExitEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(endEvent->shapeIdB));

			m_PhysicsEngine->DispatchCollisionEvent(Physics::CollisionEventType::Exit, &collisionExitEvent);
		}

		for (int i = 0; i < contactEvents.hitCount; ++i)
		{
			b2ContactHitEvent* hitEvent = contactEvents.hitEvents + i;

			Physics::CollisionHitEvent collisionHitEvent{};
			collisionHitEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(hitEvent->shapeIdA));
			collisionHitEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(hitEvent->shapeIdB));

			m_PhysicsEngine->DispatchCollisionEvent(Physics::CollisionEventType::Hit, &collisionHitEvent);
		}

		// Dispatch any sensor events that occured during this simulation step.
		b2SensorEvents sensorEvents = b2World_GetSensorEvents(m_PhysicsWorld);

		for (int i = 0; i < sensorEvents.beginCount; ++i)
		{
			b2SensorBeginTouchEvent* beginEvent = sensorEvents.beginEvents + i;

			Physics::TriggerEnterEvent triggerEnterEvent{};
			triggerEnterEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(beginEvent->sensorShapeId));
			triggerEnterEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(beginEvent->visitorShapeId));

			m_PhysicsEngine->DispatchTriggerEvent(Physics::CollisionEventType::Enter, &triggerEnterEvent);
		}

		for (int i = 0; i < sensorEvents.endCount; ++i)
		{
			b2SensorEndTouchEvent* endEvent = sensorEvents.endEvents + i;

			Physics::TriggerExitEvent triggerExitEvent{};
			triggerExitEvent.entityA = GetEntityFromShapeId(b2StoreShapeId(endEvent->sensorShapeId));
			triggerExitEvent.entityB = GetEntityFromShapeId(b2StoreShapeId(endEvent->visitorShapeId));

			m_PhysicsEngine->DispatchTriggerEvent(Physics::CollisionEventType::Exit, &triggerExitEvent);
		}
	}

	void Physics2dSystem::OnDestroy()
	{
		if (!m_Initialized)
		{
			return;
		}

		m_PhysicsEngine->Shutdown();

		m_Initialized = false;
	}

	Physics::ID Physics2dSystem::CreateRigidbody(Entity entity, Component::Rigidbody2D& rb2d, Component::Transform& transform)
	{
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = BodyTypeTob2BodyType(rb2d.Type);
		bodyDef.position = { transform.Translation.x, transform.Translation.y };
		bodyDef.rotation = b2MakeRot(glm::radians(transform.Rotation.z));
		bodyDef.motionLocks.angularZ = rb2d.FixedRotation;
		bodyDef.userData = reinterpret_cast<void*>(static_cast<intptr_t>((uint64_t)entity));

		b2BodyId bodyId = b2CreateBody(m_PhysicsWorld, &bodyDef);
		return b2StoreBodyId(bodyId);
	}

	Physics::ID Physics2dSystem::CreateBoxCollider(Entity entity, Component::BoxCollider2D& bc2d, Component::Rigidbody2D& rb2d, Component::Transform& transform)
	{
		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.material.friction = bc2d.Friction;
		shapeDef.material.restitution = bc2d.Restitution;
		shapeDef.density = bc2d.Density;
		shapeDef.enableContactEvents = bc2d.Trigger ? false : true;
		shapeDef.enableSensorEvents = bc2d.Trigger ? true : false;
		shapeDef.isSensor = bc2d.Trigger;
		shapeDef.userData = reinterpret_cast<void*>(static_cast<intptr_t>((uint64_t)entity));

		const b2Polygon polygon = b2MakeBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y);

		b2ShapeId shapeId = b2CreatePolygonShape(b2LoadBodyId(rb2d.BodyId), &shapeDef, &polygon);
		return b2StoreShapeId(shapeId);
	}
}
