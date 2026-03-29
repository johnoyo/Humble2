#pragma once

#include "Humble2Core.h"

using namespace HBL2;

class NewSystem final : public HBL2::ISystem
{
public:
	virtual ~NewSystem()
	{
		// HBL2_FATAL("NewSystem dtor\n");
	}

	virtual void OnCreate() override
	{
		PhysicsEngine2D::Instance->OnCollisionEnterEvent([this](Physics::CollisionEnterEvent* beginTouchEvent)
		{
			auto tagA = m_Context->GetComponent<Component::Tag>(beginTouchEvent->entityA).Name.view();
			auto tagB = m_Context->GetComponent<Component::Tag>(beginTouchEvent->entityB).Name.view();

			HBL2_INFO("{} -> {}\n", tagA, tagB);

			m_Grounded = true;
		});

		PhysicsEngine3D::Instance->OnCollisionEnterEvent([this](Physics::CollisionEnterEvent* collisionEnterEvent)
		{
			auto tagA = m_Context->GetComponent<Component::Tag>(collisionEnterEvent->entityA).Name.view();
			auto tagB = m_Context->GetComponent<Component::Tag>(collisionEnterEvent->entityB).Name.view();

			HBL2_INFO("[COLLISION] Entered {} -> {}\n", tagA, tagB);

			auto& mesh = m_Context->GetComponent<Component::StaticMesh>(collisionEnterEvent->entityB);
			Material* mat = ResourceManager::Instance->GetMaterial(mesh.Material);
			mat->AlbedoColor = { 1.0f, 0.55f, 0.95f, 1.0f };
		});

		PhysicsEngine3D::Instance->OnTriggerEnterEvent([this](Physics::TriggerEnterEvent* triggerEnterEvent)
		{
			auto tagA = m_Context->GetComponent<Component::Tag>(triggerEnterEvent->entityA).Name.view();
			auto tagB = m_Context->GetComponent<Component::Tag>(triggerEnterEvent->entityB).Name.view();

			HBL2_INFO("[TRIGGER] Entered {} -> {}\n", tagA, tagB);
		});

		PhysicsEngine3D::Instance->OnCollisionExitEvent([this](Physics::CollisionExitEvent* collisionExitEvent)
		{
			auto tagA = m_Context->GetComponent<Component::Tag>(collisionExitEvent->entityA).Name.view();
			auto tagB = m_Context->GetComponent<Component::Tag>(collisionExitEvent->entityB).Name.view();

			HBL2_INFO("[COLLISION] Exited {} -> {}\n", tagA, tagB);
		});

		PhysicsEngine3D::Instance->OnTriggerExitEvent([this](Physics::TriggerExitEvent* triggerExitEvent)
		{
			auto tagA = m_Context->GetComponent<Component::Tag>(triggerExitEvent->entityA).Name.view();
			auto tagB = m_Context->GetComponent<Component::Tag>(triggerExitEvent->entityB).Name.view();

			HBL2_INFO("[TRIGGER] Exited {} -> {}\n", tagA, tagB);
		});
	}

	virtual void OnUpdate(float ts) override
	{
		m_Context->Filter<Component::AudioSource>()
			.ForEach([&](Component::AudioSource& as)
			{
				if (HBL2::Input::GetKeyPress(KeyCode::P))
				{
					SoundEngine::Instance->Play(as);
				}

				if (HBL2::Input::GetKeyPress(KeyCode::L))
				{
					SoundEngine::Instance->Pause(as);
				}

				if (HBL2::Input::GetKeyPress(KeyCode::R))
				{
					SoundEngine::Instance->Resume(as);
				}

				if (HBL2::Input::GetKeyPress(KeyCode::S))
				{
					SoundEngine::Instance->Stop(as);
				}
			});

		m_Context->Filter<NewComponent>()
			.ForEach([&](NewComponent& newComponent)
			{
				if (HBL2::Input::GetKeyPress(KeyCode::C))
				{
					if (newComponent.Mario == Entity::Null)
					{
						HBL2_INFO("Hello!");
					}
					else
					{
						HBL2_INFO("Hello - {}!", m_Context->GetComponent<Component::Tag>(newComponent.Mario).Name.view());
					}

					// HBL2::SceneManager::Get().LoadScene(newComponent.SceneHandle);
				}
			});		
	}

	virtual void OnFixedUpdate() override
	{
		m_Context->Filter<Component::Rigidbody2D>()
			.ForEach([this](Entity entity, Component::Rigidbody2D& rb2d)
			{
				if (rb2d.Type == Physics::BodyType::Dynamic)
				{
					if (Input::GetKeyPress(KeyCode::W) && m_Grounded)
					{
						m_Grounded = false;
						PhysicsEngine2D::Instance->ApplyLinearImpulse(rb2d, { 0.0f, 10.0f }, true);
					}
					if (Input::GetKeyDown(KeyCode::D))
					{
						PhysicsEngine2D::Instance->ApplyForce(rb2d, { 12.0f, 0.0f }, true);
					}
					if (Input::GetKeyDown(KeyCode::A))
					{
						PhysicsEngine2D::Instance->ApplyForce(rb2d, { -12.0f, 0.0f }, true);
					}
				}
			});
	}

private:
	bool m_Grounded = false;
};

REGISTER_HBL2_SYSTEM(NewSystem)
