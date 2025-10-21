#pragma once

#include "PhysicsEngine3D.h"

// Jolt includes
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Layers
{
	static constexpr JPH::ObjectLayer TRIGGER = 0;
	static constexpr JPH::ObjectLayer NON_MOVING = 1;
	static constexpr JPH::ObjectLayer MOVING = 2;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 3;
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer TRIGGER(0);
	static constexpr JPH::BroadPhaseLayer STATIC(1);
	static constexpr JPH::BroadPhaseLayer DYNAMIC(2);
	static constexpr uint32_t NUM_LAYERS(3);
};

namespace HBL2
{
	/// Class that determines if two object layers can collide
	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
		{
			switch (inObject1)
			{
			case Layers::TRIGGER:
				return true; // Trigger collides with everything
			case Layers::NON_MOVING:
				return inObject2 == Layers::MOVING || inObject2 == Layers::TRIGGER; // Non moving only collides with moving and trigger
			case Layers::MOVING:
				return true; // Moving collides with everything
			default:
				JPH_ASSERT(false);
				return false;
			}
		}
	};

	// BroadPhaseLayerInterface implementation
	// This defines a mapping between object and broadphase layers.
	class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
	{
	public:
		BPLayerInterfaceImpl() = default;

		virtual uint32_t GetNumBroadPhaseLayers() const override
		{
			return BroadPhaseLayers::NUM_LAYERS;
		}

		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
		{
			switch (inLayer)
			{
			case Layers::TRIGGER:
				return BroadPhaseLayers::TRIGGER;
			case Layers::NON_MOVING:
				return BroadPhaseLayers::STATIC;
			case Layers::MOVING:
				return BroadPhaseLayers::DYNAMIC;
			}

			JPH_ASSERT(false);
			return BroadPhaseLayers::STATIC;
		}
	};

	/// Class that determines if an object layer can collide with a broadphase layer
	class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
		{
			switch (inLayer1)
			{
			case Layers::TRIGGER:
				return true;
			case Layers::NON_MOVING:
				return inLayer2 == BroadPhaseLayers::DYNAMIC;
			case Layers::MOVING:
				return inLayer2 == BroadPhaseLayers::DYNAMIC || inLayer2 == BroadPhaseLayers::STATIC;
			default:
				JPH_ASSERT(false);
				return false;
			}
		}
	};

	class JoltPhysicsEngine final : public PhysicsEngine3D
	{
	public:
		virtual ~JoltPhysicsEngine() = default;

		void Initialize();
		void Step(float inDeltaTime, int inCollisionSteps);
		void ShutDown();

		void DispatchCollisionEvent(Physics::CollisionEventType collisionEventType, void* collisionEventData);
		void DispatchTriggerEvent(Physics::CollisionEventType collisionEventType, void* triggerEventData);

		JPH::PhysicsSystem* Get() { return m_PhysicsSystem; }

		virtual void OnCollisionEnterEvent(std::function<void(Physics::CollisionEnterEvent*)>&& enterEventFunc) override;
		virtual void OnCollisionStayEvent(std::function<void(Physics::CollisionStayEvent*)>&& stayEventFunc) override;
		virtual void OnCollisionExitEvent(std::function<void(Physics::CollisionExitEvent*)>&& exitEventFunc) override;
		virtual void OnTriggerEnterEvent(std::function<void(Physics::TriggerEnterEvent*)>&& enterEventFunc) override;
		virtual void OnTriggerStayEvent(std::function<void(Physics::TriggerStayEvent*)>&& stayEventFunc) override;
		virtual void OnTriggerExitEvent(std::function<void(Physics::TriggerExitEvent*)>&& exitEventFunc) override;

		virtual void SetPosition(Component::Rigidbody& rb, const glm::vec3& position) override;
		virtual void SetRotation(Component::Rigidbody& rb, const glm::vec3& rotation) override;

		virtual void AddLinearVelocity(Component::Rigidbody& rb, const glm::vec3& linearVelocity) override;
		virtual void SetLinearVelocity(Component::Rigidbody& rb, const glm::vec3& linearVelocity) override;
		virtual glm::vec3 GetLinearVelocity(Component::Rigidbody& rb) override;

		virtual void SetAngularVelocity(Component::Rigidbody& rb, const glm::vec3& angularVelocity) override;
		virtual glm::vec3 GetAngularVelocity(Component::Rigidbody& rb) override;

		virtual void ApplyForce(Component::Rigidbody& rb, const glm::vec3& force) override;
		virtual void ApplyTorque(Component::Rigidbody& rb, const glm::vec3& torque) override;
		virtual void ApplyImpulse(Component::Rigidbody& rb, const glm::vec3& impluse) override;
		virtual void ApplyAngularImpulse(Component::Rigidbody& rb, const glm::vec3& angularImpulse) override;

		virtual void SetDebugDrawEnabled(bool enabled) override;
		virtual void OnDebugDraw() override;

	private:
		JPH::TempAllocatorImpl* m_TempAllocator = nullptr;
		JPH::JobSystemThreadPool m_JobSystem;
		JPH::PhysicsSystem* m_PhysicsSystem = nullptr;
		BPLayerInterfaceImpl m_BroadPhaseLayerInterface;
		ObjectVsBroadPhaseLayerFilterImpl m_ObjectVsBroadPhaseLayerFilter;
		ObjectLayerPairFilterImpl m_ObjectVsObjectLayerFilter;

		std::vector<std::function<void(Physics::CollisionEnterEvent*)>> m_CollisionEnterEvents;
		std::vector<std::function<void(Physics::CollisionStayEvent*)>> m_CollisionStayEvents;
		std::vector<std::function<void(Physics::CollisionExitEvent*)>> m_CollisionExitEvents;

		std::vector<std::function<void(Physics::TriggerEnterEvent*)>> m_TriggerEnterEvents;
		std::vector<std::function<void(Physics::TriggerStayEvent*)>> m_TriggerStayEvents;
		std::vector<std::function<void(Physics::TriggerExitEvent*)>> m_TriggerExitEvents;

		bool m_DebugDrawEnabled = false;
		JPH::DebugRenderer* m_DebugRenderer = nullptr;
	};

}
