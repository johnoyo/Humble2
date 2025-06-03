#include "Physics3dSystem.h"

#include "Core/Time.h"
#include "Core/Allocators.h"
#include "Resources/ResourceManager.h"
#include <Utilities/Collections/DynamicArray.h>
#include <Utilities/Allocators/BumpAllocator.h>
#include <Utilities/MeshUtilities.h>

namespace HBL2
{
	class HumbleContactListener : public JPH::ContactListener
	{
	public:
		HumbleContactListener(JPH::PhysicsSystem* ctx) : m_Context(ctx) {}

		virtual JPH::ValidateResult	OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override
		{
			HBL2_CORE_TRACE("Contact validate callback");

			// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
			return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
		}

		virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
		{
			HBL2_CORE_TRACE("A contact was added");
			// m_Context->WereBodiesInContact(inBody1, inBody2);
		}

		virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
		{
			HBL2_CORE_TRACE("A contact was persisted");
		}

		virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
		{
			HBL2_CORE_TRACE("A contact was removed");
			/// If you want to know if this is the last contact between the two bodies, use PhysicsSystem::WereBodiesInContact.
			// m_Context->WereBodiesInContact(inBody1, inBody2);
		}

	private:
		JPH::PhysicsSystem* m_Context = nullptr;
	};

	static JPH::EMotionType BodyTypeToEMotionType(Physics::BodyType bodyType)
	{
		switch (bodyType)
		{
		case Physics::BodyType::Static:    return JPH::EMotionType::Static;
		case Physics::BodyType::Kinematic: return JPH::EMotionType::Kinematic;
		case Physics::BodyType::Dynamic:   return JPH::EMotionType::Dynamic;
		}

		HBL2_CORE_ASSERT(false, "Unknown Rigidbody type.");
		return JPH::EMotionType::Static;
	}

	static void TraceImpl(const char* inFMT, ...)
	{
		// Format the message
		va_list list;
		va_start(list, inFMT);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), inFMT, list);
		va_end(list);

		// Print to the TTY
		HBL2_CORE_TRACE(buffer);
	}

	static inline const JPH::BodyID& GetBodyIDFromPhysicsID(Physics::ID id)
	{
		return *((JPH::BodyID*)id);
	}

	static inline Physics::ID GetPhysicsIDFromBodyID(const JPH::BodyID& bodyId)
	{
		return (Physics::ID)&bodyId;
	}

	void Physics3dSystem::OnCreate()
	{
		JPH::RegisterDefaultAllocator();
		JPH::Trace = TraceImpl;
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		m_TempAllocator = new JPH::TempAllocatorImpl(15_MB);
		m_JobSystem.Init(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, -1);

		const uint32_t cMaxBodies = 65536;
		const uint32_t cNumBodyMutexes = 0;
		const uint32_t cMaxBodyPairs = 65536;
		const uint32_t cMaxContactConstraints = 10240;

		m_PhysicsSystem.Init(cMaxBodies,
							cNumBodyMutexes,
							cMaxBodyPairs,
							cMaxContactConstraints,
							m_BroadPhaseLayerInterface,
							m_ObjectVsBroadPhaseLayerFilter,
							m_ObjectVsObjectLayerFilter);

		// Maybe also take in a lamda to handle contacts?
		m_PhysicsSystem.SetContactListener(new HumbleContactListener(&m_PhysicsSystem));

		// The main way to interact with the bodies in the physics system is through the body interface.
		JPH::BodyInterface& bodyInterface = m_PhysicsSystem.GetBodyInterface();

		DynamicArray<JPH::BodyID, BumpAllocator> bulkAddBuffer = MakeDynamicArray<JPH::BodyID>(&Allocator::Frame);

		m_Context->GetRegistry()
			.group<Component::Rigidbody>(entt::get<Component::Transform>)
			.each([this, &bodyInterface, &bulkAddBuffer](entt::entity entity, Component::Rigidbody& rb, Component::Transform& transform)
			{
				JPH::ShapeRefC shapeRef;
				JPH::ObjectLayer bodyLayer;
				JPH::EMotionType type = BodyTypeToEMotionType(rb.Type);

				if (m_Context->HasComponent<Component::BoxCollider>(entity))
				{
					auto& bc = m_Context->GetComponent<Component::BoxCollider>(entity);

					JPH::BoxShapeSettings shapeSettings(JPH::Vec3(bc.Size.x * transform.Scale.x, bc.Size.x * transform.Scale.y, bc.Size.x * transform.Scale.z));
					shapeSettings.SetEmbedded();

					// Create the shape
					JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
					shapeRef = shapeResult.Get();
					bodyLayer = (type == JPH::EMotionType::Static ? Layers::NON_MOVING : Layers::MOVING);
				}
				else if (m_Context->HasComponent<Component::SphereCollider>(entity))
				{
					auto& sc = m_Context->GetComponent<Component::SphereCollider>(entity);

					JPH::SphereShapeSettings shapeSettings(sc.Radius * transform.Scale.x);
					shapeSettings.SetEmbedded();

					// Create the shape
					JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
					shapeRef = shapeResult.Get();
					bodyLayer = (type == JPH::EMotionType::Static ? Layers::NON_MOVING : Layers::MOVING);
				}
				else if (m_Context->HasComponent<Component::MeshCollider>(entity))
				{
					auto& mc = m_Context->GetComponent<Component::MeshCollider>(entity);

					Mesh* mesh = nullptr;

					if (mc.MeshHandle.IsValid())
					{
						mesh = ResourceManager::Instance->GetMesh(mc.MeshHandle);
					}
					else
					{
						auto& meshComponent = m_Context->GetComponent<Component::StaticMesh>(entity);
						mesh = ResourceManager::Instance->GetMesh(meshComponent.Mesh);
					}

					// TODO
					// ...

					bodyLayer = (type == JPH::EMotionType::Static ? Layers::NON_MOVING : Layers::MOVING);
				}
				else
				{
					// Create unit sphere for ghost rigidbodies
					JPH::SphereShapeSettings shapeSettings(1.0f);
					shapeSettings.SetEmbedded();

					// Create the shape
					JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
					shapeRef = shapeResult.Get();

					bodyLayer = Layers::GHOST;
				}

				JPH::RVec3 bodyPosition = JPH::RVec3(transform.Translation.x, transform.Translation.y, transform.Translation.z);
				JPH::Quat bodyRotation = JPH::Quat(transform.QRotation.x, transform.QRotation.y, transform.QRotation.z, transform.QRotation.w);

				// Create the settings for the body itself.
				JPH::BodyCreationSettings bodySettings(shapeRef, bodyPosition, bodyRotation, type, bodyLayer);
				bodySettings.mFriction = rb.Friction;
				bodySettings.mRestitution = rb.Restitution;
				bodySettings.mLinearDamping = rb.LinearDamping;
				bodySettings.mAngularDamping = rb.AngularDamping;

				// Create the actual rigid body. Note that if we run out of bodies this can return nullptr.
				JPH::Body* body = bodyInterface.CreateBody(bodySettings);

				// Maybe use the entity ID here?
				body->SetUserData((JPH::uint64)entity);
				rb.BodyID = GetPhysicsIDFromBodyID(body->GetID());

				bulkAddBuffer.Add(GetBodyIDFromPhysicsID(rb.BodyID));
			});

		JPH::BodyInterface::AddState addState =  bodyInterface.AddBodiesPrepare(bulkAddBuffer.Data(), bulkAddBuffer.Size());
		bodyInterface.AddBodiesFinalize(bulkAddBuffer.Data(), bulkAddBuffer.Size(), addState, JPH::EActivation::Activate);
	}

	void Physics3dSystem::OnUpdate(float ts)
	{
	}

	void Physics3dSystem::OnFixedUpdate()
	{
		// Handle runtime creations and properties update.
		m_Context->GetRegistry()
			.group<Component::Rigidbody>(entt::get<Component::Transform>)
			.each([this](entt::entity entity, Component::Rigidbody& rb, Component::Transform& transform)
			{
				JPH::BodyInterface& bodyInterface = m_PhysicsSystem.GetBodyInterfaceNoLock();
				if (rb.BodyID == Physics::InvalidID)
				{

					return;
				}

				JPH::BodyID bodyID = GetBodyIDFromPhysicsID(rb.BodyID);

				bodyInterface.SetFriction(bodyID, rb.Friction);
				bodyInterface.SetRestitution(bodyID, rb.Restitution);
			});

		// Update internal simulation step.
		JPH::BodyInterface& bodyInterface = m_PhysicsSystem.GetBodyInterface();
		const float cDeltaTime = Time::FixedTimeStep;
		const int cCollisionSteps = 1;

		m_PhysicsSystem.Update(cDeltaTime, cCollisionSteps, m_TempAllocator, &m_JobSystem);

		// Apply physics changes to transforms.
		m_Context->GetRegistry()
			.group<Component::Rigidbody>(entt::get<Component::Transform>)
			.each([this, &bodyInterface](entt::entity entity, Component::Rigidbody& rb, Component::Transform& transform)
			{
				glm::vec3 originalScale = transform.Scale;

				JPH::Vec3 outPosition;
				JPH::Quat outRotation;

				bodyInterface.GetPositionAndRotation(GetBodyIDFromPhysicsID(rb.BodyID), outPosition, outRotation);
				transform.Translation.x = outPosition.GetX();
				transform.Translation.y = outPosition.GetY();
				transform.Translation.z = outPosition.GetZ();

				const auto& rotation = outRotation.GetEulerAngles();
				transform.Rotation.x = glm::degrees(rotation.GetX());
				transform.Rotation.y = glm::degrees(rotation.GetY());
				transform.Rotation.z = glm::degrees(rotation.GetZ());

				transform.Scale = originalScale;
			});
	}

	void Physics3dSystem::OnDestroy()
	{
		// Unregisters all types with the factory and cleans up the default material
		JPH::UnregisterTypes();

		// Destroy temp allocator
		delete m_TempAllocator;
		m_TempAllocator = nullptr;

		// Destroy the factory
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}
}
