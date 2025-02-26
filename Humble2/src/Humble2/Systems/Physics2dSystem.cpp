#include "Physics2dSystem.h"

#include "Core\Time.h"

namespace HBL2
{
	static b2BodyType BodyTypeTob2BodyType(Component::Rigidbody2D::BodyType bodyType)
	{
		switch (bodyType)
		{
		case HBL2::Component::Rigidbody2D::BodyType::Static:    return b2BodyType::b2_staticBody;
		case HBL2::Component::Rigidbody2D::BodyType::Dynamic:   return b2BodyType::b2_dynamicBody;
		case HBL2::Component::Rigidbody2D::BodyType::Kinematic: return b2BodyType::b2_kinematicBody;
		}

		HBL2_CORE_ASSERT(false, "Unknown Rigidbody2D type.");
		return b2BodyType::b2_staticBody;
	}

	void Physics2dSystem::OnCreate()
	{
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = { 0, m_GravityForce };
		worldDef.restitutionThreshold = 0.5f;

		m_PhysicsWorld = b2CreateWorld(&worldDef);

		m_Context->GetRegistry()
			.group<Component::Rigidbody2D>(entt::get<Component::Transform>)
			.each([this](entt::entity entity, Component::Rigidbody2D& rb2d, Component::Transform& transform)
			{
				b2BodyDef bodyDef = b2DefaultBodyDef();
				bodyDef.type = BodyTypeTob2BodyType(rb2d.Type);
				bodyDef.position = { transform.Translation.x, transform.Translation.y };
				bodyDef.rotation = b2MakeRot(glm::radians(transform.Rotation.z));
				bodyDef.fixedRotation = rb2d.FixedRotation;

				rb2d.BodyId = b2CreateBody(m_PhysicsWorld, &bodyDef);

				if (m_Context->HasComponent<Component::BoxCollider2D>(entity))
				{
					Component::BoxCollider2D& bc2d = m_Context->GetComponent<Component::BoxCollider2D>(entity);

					b2ShapeDef shapeDef = b2DefaultShapeDef();
					shapeDef.friction = bc2d.Friction;
					shapeDef.restitution = bc2d.Restitution;
					shapeDef.density = bc2d.Density;

					const b2Polygon polygon = b2MakeBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y);

					bc2d.ShapeId = b2CreatePolygonShape(rb2d.BodyId, &shapeDef, &polygon);					
				}
			});
	}

	void Physics2dSystem::OnUpdate(float ts)
	{
		// Progress the simulation.
		b2World_Step(m_PhysicsWorld, Time::FixedTimeStep, m_SubStepCount);

		// Update the transform of rigidbodies
		m_Context->GetRegistry()
			.group<Component::Rigidbody2D>(entt::get<Component::Transform>)
			.each([this](entt::entity entity, Component::Rigidbody2D& rb2d, Component::Transform& transform)
			{
				const auto& position = b2Body_GetPosition(rb2d.BodyId);
				transform.Translation.x = position.x;
				transform.Translation.y = position.y;

				const auto& rotation = b2Body_GetRotation(rb2d.BodyId);
				transform.Rotation.z = glm::degrees(b2Rot_GetAngle(rotation));
			});
	}

	void Physics2dSystem::OnDestroy()
	{
		if (m_PhysicsWorld.index1 != 0)
		{
			b2DestroyWorld(m_PhysicsWorld);
		}
	}
}
