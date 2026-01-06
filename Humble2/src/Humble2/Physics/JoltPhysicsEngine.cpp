#include "JoltPhysicsEngine.h"

#include "JoltDebugRenderer.h"

#include "Core\Time.h"
#include "Core\Allocators.h"
#include "Resources\ResourceManager.h"
#include "Systems\AnimationCurveSystem.h"
#include "Utilities\Collections\Collections.h"

namespace HBL2
{
	static inline const JPH::BodyID& GetBodyIDFromPhysicsID(Physics::ID id)
	{
		return *((JPH::BodyID*)id);
	}

	static inline Physics::ID GetPhysicsIDFromBodyID(const JPH::BodyID& bodyId)
	{
		return (Physics::ID)&bodyId;
	}

	class HumbleContactListener : public JPH::ContactListener
	{
	public:
		HumbleContactListener(JoltPhysicsEngine* engine, JPH::PhysicsSystem* ctx) : m_Context(ctx), m_Engine(engine) {}

		virtual JPH::ValidateResult	OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override
		{
			// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
			return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
		}

		virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
		{
			if (m_Context->WereBodiesInContact(inBody1.GetID(), inBody2.GetID()))
			{
				return; // not the first contact between bodies
			}

			if (ioSettings.mIsSensor)
			{
				Physics::TriggerEnterEvent triggerEnterEvent = { (Entity)inBody1.GetUserData(), (Entity)inBody2.GetUserData() };
				m_Engine->DispatchTriggerEvent(Physics::CollisionEventType::Enter, &triggerEnterEvent);
			}
			else
			{
				Physics::CollisionEnterEvent collisionEnterEvent = { (Entity)inBody1.GetUserData(), (Entity)inBody2.GetUserData() };
				m_Engine->DispatchCollisionEvent(Physics::CollisionEventType::Enter, &collisionEnterEvent);
			}
		}

		virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
		{
			if (ioSettings.mIsSensor)
			{
				Physics::TriggerStayEvent triggerStayEvent = { (Entity)inBody1.GetUserData(), (Entity)inBody2.GetUserData() };
				m_Engine->DispatchTriggerEvent(Physics::CollisionEventType::Stay, &triggerStayEvent);
			}
			else
			{
				Physics::CollisionEnterEvent collisionEnterEvent = { (Entity)inBody1.GetUserData(), (Entity)inBody2.GetUserData() };
				m_Engine->DispatchCollisionEvent(Physics::CollisionEventType::Stay, &collisionEnterEvent);
			}
		}

		virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
		{
			const auto body1ID = inSubShapePair.GetBody1ID();
			const auto body2ID = inSubShapePair.GetBody2ID();

			if (m_Context->WereBodiesInContact(body1ID, body2ID))
			{
				return; // not the last contact between bodies
			}

			auto& bodyIterface = m_Context->GetBodyInterfaceNoLock();

			if (bodyIterface.GetObjectLayer(body1ID) == Layers::TRIGGER || bodyIterface.GetObjectLayer(body2ID) == Layers::TRIGGER)
			{
				Physics::TriggerExitEvent triggerExitEvent = { (Entity)bodyIterface.GetUserData(body1ID), (Entity)bodyIterface.GetUserData(body2ID) };
				m_Engine->DispatchTriggerEvent(Physics::CollisionEventType::Exit, &triggerExitEvent);
			}
			else
			{
				Physics::CollisionExitEvent collisionExitEvent = { (Entity)bodyIterface.GetUserData(body1ID), (Entity)bodyIterface.GetUserData(body2ID) };
				m_Engine->DispatchCollisionEvent(Physics::CollisionEventType::Exit, &collisionExitEvent);
			}
		}

	private:
		JoltPhysicsEngine* m_Engine = nullptr;
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

	void JoltPhysicsEngine::Initialize(Scene* ctx)
	{
		m_Context = ctx;

		JPH::RegisterDefaultAllocator();
		JPH::Trace = TraceImpl;
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		m_TempAllocator = new JPH::TempAllocatorImpl(50_MB);
		m_JobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, -1);

		const uint32_t cMaxBodies = 65536;
		const uint32_t cNumBodyMutexes = 0;
		const uint32_t cMaxBodyPairs = 65536;
		const uint32_t cMaxContactConstraints = 10240;

		m_PhysicsSystem = new JPH::PhysicsSystem;

		m_PhysicsSystem->Init(cMaxBodies,
			cNumBodyMutexes,
			cMaxBodyPairs,
			cMaxContactConstraints,
			m_BroadPhaseLayerInterface,
			m_ObjectVsBroadPhaseLayerFilter,
			m_ObjectVsObjectLayerFilter);

		m_DebugRenderer = new JoltDebugRenderer;

		// Register collision listener to dispatch events.
		m_PhysicsSystem->SetContactListener(new HumbleContactListener(this, m_PhysicsSystem));

		// The main way to interact with the bodies in the physics system is through the body interface.
		JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();

		DArray<JPH::BodyID> bulkAddBuffer = MakeDArray<JPH::BodyID>(Allocator::FrameArena, 32);

		m_Context->Group<Component::Rigidbody>(Get<Component::Transform>)
			.Each([this, &bodyInterface, &bulkAddBuffer](Entity entity, Component::Rigidbody& rb, Component::Transform& transform)
			{
				AddRigidBody(entity, rb, transform, bodyInterface);
				bulkAddBuffer.push_back(GetBodyIDFromPhysicsID(rb.BodyID));
			});

		JPH::BodyInterface::AddState addState = bodyInterface.AddBodiesPrepare(bulkAddBuffer.data(), bulkAddBuffer.size());
		bodyInterface.AddBodiesFinalize(bulkAddBuffer.data(), bulkAddBuffer.size(), addState, JPH::EActivation::Activate);
	}

	void JoltPhysicsEngine::Update()
	{
		// Handle runtime creations and properties update.
		m_Context->Group<Component::Rigidbody>(Get<Component::Transform>)
			.Each([this](Entity entity, Component::Rigidbody& rb, Component::Transform& transform)
			{
				JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
				if (rb.BodyID == Physics::InvalidID)
				{
					AddRigidBody(entity, rb, transform, bodyInterface);
					bodyInterface.AddBody(GetBodyIDFromPhysicsID(rb.BodyID), JPH::EActivation::Activate);
					return;
				}

				bool hasCollider = HasAnyCollider(entity);

				JPH::BodyID bodyID = GetBodyIDFromPhysicsID(rb.BodyID);
				JPH::ObjectLayer objectLayer = bodyInterface.GetObjectLayer(bodyID);
				JPH::EMotionType newType = BodyTypeToEMotionType(rb.Type);

				if (rb.Dirty)
				{
					if (rb.Trigger)
					{
						const JPH::BodyLockInterface& lockingBodyInterface = m_PhysicsSystem->GetBodyLockInterface();

						JPH::BodyLockWrite lock(lockingBodyInterface, bodyID);
						if (lock.Succeeded())
						{
							JPH::Body& body = lock.GetBody();
							body.SetIsSensor(true);
						}

						bodyInterface.SetObjectLayer(bodyID, Layers::TRIGGER);
					}
					else
					{
						// NOTE: There is a bug when adding collider after rigidbody at runtime, the collider settings wont apply.
						bodyInterface.RemoveBody(bodyID);
						bodyInterface.DestroyBody(bodyID);
						AddRigidBody(entity, rb, transform, bodyInterface);
						bodyInterface.AddBody(GetBodyIDFromPhysicsID(rb.BodyID), JPH::EActivation::Activate);
					}

					rb.Dirty = false;
				}

				if (bodyInterface.GetFriction(bodyID) != rb.Friction)
				{
					bodyInterface.SetFriction(bodyID, rb.Friction);
				}

				if (bodyInterface.GetGravityFactor(bodyID) != rb.GravityFactor)
				{
					bodyInterface.SetGravityFactor(bodyID, rb.GravityFactor);
				}

				if (bodyInterface.GetRestitution(bodyID) != rb.Restitution)
				{
					bodyInterface.SetRestitution(bodyID, rb.Restitution);
				}

				if (bodyInterface.GetMotionType(bodyID) != newType)
				{
					bodyInterface.SetMotionType(bodyID, newType, JPH::EActivation::Activate);

					if (!rb.Trigger)
					{
						bodyInterface.SetObjectLayer(bodyID, newType == JPH::EMotionType::Static ? Layers::NON_MOVING : Layers::MOVING);
					}
				}
			});

		// Update internal simulation step.
		JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();
		const float cDeltaTime = Time::FixedTimeStep;
		const int cCollisionSteps = 4;

		m_PhysicsSystem->Update(cDeltaTime, cCollisionSteps, m_TempAllocator, m_JobSystem);

		// Apply physics changes to transforms.
		m_Context->Group<Component::Rigidbody>(Get<Component::Transform>)
			.Each([this, &bodyInterface](Entity entity, Component::Rigidbody& rb, Component::Transform& transform)
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

	void JoltPhysicsEngine::Shutdown()
	{
		// Clear collision events.
		m_CollisionEnterEvents.clear();
		m_CollisionStayEvents.clear();
		m_CollisionExitEvents.clear();

		// Clear trigger events.
		m_TriggerEnterEvents.clear();
		m_TriggerStayEvents.clear();
		m_TriggerExitEvents.clear();

		// Unregisters all types with the factory and cleans up the default material.
		JPH::UnregisterTypes();

		// Destroy temp allocator.
		delete m_TempAllocator;
		m_TempAllocator = nullptr;

		// Destroy the factory.
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;

		// Delete job system.
		delete m_JobSystem;
		m_JobSystem = nullptr;

		// Delete physics system.
		delete m_PhysicsSystem;
		m_PhysicsSystem = nullptr;

		// Delete jolt debug renderer.
		delete m_DebugRenderer;
		m_DebugRenderer = nullptr;
	}

	void JoltPhysicsEngine::DispatchCollisionEvent(Physics::CollisionEventType collisionEventType, void* collisionEventData)
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
		case Physics::CollisionEventType::Stay:
			if (!m_CollisionStayEvents.empty())
			{
				Physics::CollisionStayEvent* collisionStayEvent = (Physics::CollisionStayEvent*)collisionEventData;

				for (const auto& callback : m_CollisionStayEvents)
				{
					callback(collisionStayEvent);
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
		}
	}

	void JoltPhysicsEngine::DispatchTriggerEvent(Physics::CollisionEventType collisionEventType, void* triggerEventData)
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
		case Physics::CollisionEventType::Stay:
			if (!m_TriggerStayEvents.empty())
			{
				Physics::TriggerStayEvent* triggerStayEvent = (Physics::TriggerStayEvent*)triggerEventData;

				for (const auto& callback : m_TriggerStayEvents)
				{
					callback(triggerStayEvent);
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

	void JoltPhysicsEngine::OnCollisionEnterEvent(std::function<void(Physics::CollisionEnterEvent*)>&& enterEventFunc)
	{
		m_CollisionEnterEvents.emplace_back(enterEventFunc);
	}

	void JoltPhysicsEngine::OnCollisionStayEvent(std::function<void(Physics::CollisionStayEvent*)>&& stayEventFunc)
	{
		m_CollisionStayEvents.emplace_back(stayEventFunc);
	}

	void JoltPhysicsEngine::OnCollisionExitEvent(std::function<void(Physics::CollisionExitEvent*)>&& exitEventFunc)
	{
		m_CollisionExitEvents.emplace_back(exitEventFunc);
	}

	void JoltPhysicsEngine::OnTriggerEnterEvent(std::function<void(Physics::TriggerEnterEvent*)>&& enterEventFunc)
	{
		m_TriggerEnterEvents.emplace_back(enterEventFunc);
	}

	void JoltPhysicsEngine::OnTriggerStayEvent(std::function<void(Physics::TriggerStayEvent*)>&& stayEventFunc)
	{
		m_TriggerStayEvents.emplace_back(stayEventFunc);
	}

	void JoltPhysicsEngine::OnTriggerExitEvent(std::function<void(Physics::TriggerExitEvent*)>&& exitEventFunc)
	{
		m_TriggerExitEvents.emplace_back(exitEventFunc);
	}

	void JoltPhysicsEngine::SetPosition(Component::Rigidbody& rb, const glm::vec3& position)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.SetPosition(GetBodyIDFromPhysicsID(rb.BodyID), { position.x, position.y, position.z }, JPH::EActivation::Activate);
	}

	void JoltPhysicsEngine::SetRotation(Component::Rigidbody& rb, const glm::vec3& rotation)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		glm::quat qRot = glm::quat({ glm::radians(rotation.x), glm::radians(rotation.y), glm::radians(rotation.z) });
		bodyInterface.SetRotation(GetBodyIDFromPhysicsID(rb.BodyID), { qRot.x, qRot.y, qRot.z, qRot.w }, JPH::EActivation::Activate);
	}
	
	void JoltPhysicsEngine::AddLinearVelocity(Component::Rigidbody& rb, const glm::vec3& linearVelocity)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.AddLinearVelocity(GetBodyIDFromPhysicsID(rb.BodyID), { linearVelocity.x, linearVelocity.y, linearVelocity.z });
	}

	void JoltPhysicsEngine::SetLinearVelocity(Component::Rigidbody& rb, const glm::vec3& linearVelocity)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.SetLinearVelocity(GetBodyIDFromPhysicsID(rb.BodyID), { linearVelocity.x, linearVelocity.y, linearVelocity.z });
	}

	glm::vec3 JoltPhysicsEngine::GetLinearVelocity(Component::Rigidbody& rb)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		const auto& v = bodyInterface.GetLinearVelocity(GetBodyIDFromPhysicsID(rb.BodyID));
		return { v.GetX(), v.GetY(), v.GetZ() };
	}

	void JoltPhysicsEngine::SetAngularVelocity(Component::Rigidbody& rb, const glm::vec3& angularVelocity)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.SetAngularVelocity(GetBodyIDFromPhysicsID(rb.BodyID), { angularVelocity.x, angularVelocity.y, angularVelocity.z });
	}

	glm::vec3 JoltPhysicsEngine::GetAngularVelocity(Component::Rigidbody& rb)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		const auto& v = bodyInterface.GetAngularVelocity(GetBodyIDFromPhysicsID(rb.BodyID));
		return { v.GetX(), v.GetY(), v.GetZ() };
	}

	void JoltPhysicsEngine::ApplyForce(Component::Rigidbody& rb, const glm::vec3& force)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.AddForce(GetBodyIDFromPhysicsID(rb.BodyID), { force.x, force.y, force.z });
	}

	void JoltPhysicsEngine::ApplyTorque(Component::Rigidbody& rb, const glm::vec3& torque)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.AddTorque(GetBodyIDFromPhysicsID(rb.BodyID), { torque.x, torque.y, torque.z });
	}

	void JoltPhysicsEngine::ApplyImpulse(Component::Rigidbody& rb, const glm::vec3& impluse)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.AddImpulse(GetBodyIDFromPhysicsID(rb.BodyID), { impluse.x, impluse.y, impluse.z });
	}

	void JoltPhysicsEngine::ApplyAngularImpulse(Component::Rigidbody& rb, const glm::vec3& angularImpulse)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.AddAngularImpulse(GetBodyIDFromPhysicsID(rb.BodyID), { angularImpulse.x, angularImpulse.y, angularImpulse.z });
	}

	void JoltPhysicsEngine::SetDebugDrawEnabled(bool enabled)
	{
		m_DebugDrawEnabled = enabled;
	}

	void JoltPhysicsEngine::ShowColliders(bool show)
	{
		m_ShowColliders = show;
	}

	void JoltPhysicsEngine::ShowBoundingBoxes(bool show)
	{
		m_ShowBoundingBoxes = show;
	}

	void JoltPhysicsEngine::OnDebugDraw()
	{
		if (!m_DebugDrawEnabled || m_PhysicsSystem == nullptr)
		{
			return;
		}

		JPH::BodyManager::DrawSettings settings;
		settings.mDrawShape = m_ShowColliders;
		settings.mDrawBoundingBox = m_ShowBoundingBoxes;
		settings.mDrawShapeWireframe = false;

		m_PhysicsSystem->DrawBodies(settings, m_DebugRenderer, nullptr);
	}

	void JoltPhysicsEngine::AddRigidBody(Entity entity, Component::Rigidbody& rb, Component::Transform& transform, JPH::BodyInterface& bodyInterface)
	{
		JPH::ShapeRefC shapeRef;
		JPH::EMotionType type = BodyTypeToEMotionType(rb.Type);

		/*glm::vec3 worldTransform = glm::vec3(transform.WorldMatrix * glm::vec4(transform.Translation, 1.0));

		glm::mat3 rotationMatrix = glm::mat3(transform.WorldMatrix);
		rotationMatrix[0] = glm::normalize(rotationMatrix[0]);
		rotationMatrix[1] = glm::normalize(rotationMatrix[1]);
		rotationMatrix[2] = glm::normalize(rotationMatrix[2]);
		glm::quat worldQRotation = glm::quat_cast(rotationMatrix);*/

		JPH::RVec3 bodyPosition = JPH::RVec3(transform.Translation.x, transform.Translation.y, transform.Translation.z);
		JPH::Quat bodyRotation = JPH::Quat(transform.QRotation.x, transform.QRotation.y, transform.QRotation.z, transform.QRotation.w);

		if (m_Context->HasComponent<Component::BoxCollider>(entity))
		{
			auto& bc = m_Context->GetComponent<Component::BoxCollider>(entity);

			JPH::BoxShapeSettings shapeSettings(JPH::Vec3(bc.Size.x * transform.Scale.x, bc.Size.y * transform.Scale.y, bc.Size.z * transform.Scale.z));
			shapeSettings.SetEmbedded();

			// Create the shape
			JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
			shapeRef = shapeResult.Get();
		}
		else if (m_Context->HasComponent<Component::SphereCollider>(entity))
		{
			auto& sc = m_Context->GetComponent<Component::SphereCollider>(entity);

			JPH::SphereShapeSettings shapeSettings(sc.Radius * transform.Scale.x);
			shapeSettings.SetEmbedded();

			// Create the shape
			JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
			shapeRef = shapeResult.Get();
		}
		else if (m_Context->HasComponent<Component::CapsuleCollider>(entity))
		{
			auto& cc = m_Context->GetComponent<Component::CapsuleCollider>(entity);

			JPH::CapsuleShapeSettings shapeSettings(cc.Height / 2.0f, cc.Radius);
			shapeSettings.SetEmbedded();

			// Create the shape
			JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
			shapeRef = shapeResult.Get();
		}
		else if (m_Context->HasComponent<Component::TerrainCollider>(entity))
		{
			auto& tc = m_Context->GetComponent<Component::TerrainCollider>(entity);
			auto& chunk = m_Context->GetComponent<Component::TerrainChunk>(entity);
			auto& tr = m_Context->GetComponent<Component::Transform>(entity);

			auto& link = m_Context->GetComponent<Component::Link>(entity);
			Entity terrainEntity = m_Context->FindEntityByUUID(link.Parent);
			auto& terrain = m_Context->GetComponent<Component::Terrain>(terrainEntity);
			auto& curve = m_Context->GetComponent<Component::AnimationCurve>(terrainEntity);

			const int32_t N = terrain.ChunkSize;
			const float S = terrain.Scale;
			const float H = terrain.HeightMultiplier;
			const float halfSize = 0.5f * (N - 1) * S;

			// The height-field that Jolt expects and the visual mesh we create are	laid out mirrored in the Z axis.
			std::vector<float> samples(chunk.NoiseMap.size());
			for (int32_t y = 0; y < N; ++y)
			{
				for (int32_t x = 0; x < N; ++x)
				{
					// source row is (N-1-y) instead of y
					float h = chunk.NoiseMap[(N - 1 - y) * N + x];
					samples[y * N + x] = AnimationCurveSystem::Evaluate(curve, h);
				}
			}

			JPH::Vec3 scale(S, H, S);

			// This is the centre of the render chunk.
			glm::vec3 chunkOrigin = { 0.0f, 0.0f, 0.0f };

			// Send the NW corner (sample 0,0) to Jolt
			JPH::Vec3 offset(chunkOrigin.x - halfSize, 0.0f, chunkOrigin.z - halfSize);

			JPH::HeightFieldShapeSettings shapeSettings(samples.data(), offset, scale, (uint32_t)N);
			shapeSettings.SetEmbedded();

			// Create the shape
			JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
			shapeRef = shapeResult.Get();
		}

		JPH::ObjectLayer bodyLayer;

		if (rb.Trigger)
		{
			bodyLayer = Layers::TRIGGER;
		}
		else
		{
			bodyLayer = (type == JPH::EMotionType::Static ? Layers::NON_MOVING : Layers::MOVING);
		}

		// Create the settings for the body itself.
		JPH::BodyCreationSettings bodySettings(shapeRef, bodyPosition, bodyRotation, type, bodyLayer);
		bodySettings.mGravityFactor = rb.GravityFactor;
		bodySettings.mFriction = rb.Friction;
		bodySettings.mRestitution = rb.Restitution;
		bodySettings.mLinearDamping = rb.LinearDamping;
		bodySettings.mAngularDamping = rb.AngularDamping;
		bodySettings.mAllowDynamicOrKinematic = true;
		bodySettings.mIsSensor = rb.Trigger;
		bodySettings.mMaxLinearVelocity = 1000.f;
		bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		bodySettings.mMassPropertiesOverride = { .mMass = rb.Mass };

		bodySettings.mCollideKinematicVsNonDynamic = rb.Trigger;
		bodySettings.mMotionQuality = (rb.MotionQuality == Component::Rigidbody::EMotionQuality::Discrete ? JPH::EMotionQuality::Discrete : JPH::EMotionQuality::LinearCast);

		// Create the actual rigid body. Note that if we run out of bodies this can return nullptr.
		JPH::Body* body = bodyInterface.CreateBody(bodySettings);

		// FIXME: This^ will crash if no collider is attached!

		if (body == nullptr)
		{
			HBL2_CORE_ERROR("Exceeded maximum number of supported rigudbodies");
			return;
		}

		// Maybe use the entity ID here?
		body->SetUserData((JPH::uint64)entity);
		rb.BodyID = GetPhysicsIDFromBodyID(body->GetID());
	}

	bool JoltPhysicsEngine::HasAnyCollider(Entity entity)
	{
		return m_Context->HasComponent<Component::BoxCollider>(entity) ||
			m_Context->HasComponent<Component::SphereCollider>(entity) ||
			m_Context->HasComponent<Component::CapsuleCollider>(entity) ||
			m_Context->HasComponent<Component::TerrainCollider>(entity);
	}
}

