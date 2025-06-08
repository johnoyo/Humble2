#pragma once

#include "Humble2Core.h"

using namespace HBL2;

class NewSystem final : public HBL2::ISystem
{
public:
	virtual void OnCreate() override
	{
		Physics2D::OnBeginTouchEvent([this](Physics2D::ContactBeginTouchEvent* beginTouchEvent)
		{
			auto& tagA = m_Context->GetComponent<Component::Tag>(beginTouchEvent->entityA).Name;
			auto& tagB = m_Context->GetComponent<Component::Tag>(beginTouchEvent->entityB).Name;

			HBL2_INFO("{} -> {}\n", tagA, tagB);

			m_Grounded = true;
		});

		Physics3D::OnCollisionEnterEvent([this](Physics3D::CollisionEnterEvent* collisionEnterEvent)
		{
			auto& tagA = m_Context->GetComponent<Component::Tag>(collisionEnterEvent->entityA).Name;
			auto& tagB = m_Context->GetComponent<Component::Tag>(collisionEnterEvent->entityB).Name;

			HBL2_INFO("[COLLISION] {} -> {}\n", tagA, tagB);
		});

		Physics3D::OnTriggerEnterEvent([this](Physics3D::TriggerEnterEvent* triggerEnterEvent)
		{
			auto& tagA = m_Context->GetComponent<Component::Tag>(triggerEnterEvent->entityA).Name;
			auto& tagB = m_Context->GetComponent<Component::Tag>(triggerEnterEvent->entityB).Name;

			HBL2_INFO("[TRIGGER] {} -> {}\n", tagA, tagB);
		});
	}

	virtual void OnUpdate(float ts) override
	{
		m_Context->GetRegistry()
			.view<NewComponent>()
			.each([&](NewComponent& newComponent)
			{
				if (HBL2::Input::GetKeyPress(KeyCode::C))
				{
					if (newComponent.Mario == entt::null)
					{
						HBL2_INFO("Hello!");
					}
					else
					{
						HBL2_INFO("Hello {}!", m_Context->GetComponent<Component::Tag>(newComponent.Mario).Name);
					}

					HBL2::SceneManager::Get().LoadScene(newComponent.SceneHandle);
				}
			});		
	}

	virtual void OnFixedUpdate() override
	{
		m_Context->GetRegistry()
			.view<Component::Rigidbody2D>()
			.each([this](entt::entity entity, Component::Rigidbody2D& rb2d)
			{
				if (rb2d.Type == Physics::BodyType::Dynamic)
				{
					if (Input::GetKeyPress(KeyCode::W) && m_Grounded)
					{
						m_Grounded = false;
						Physics2D::ApplyLinearImpulse(rb2d, { 0.0f, 10.0f }, true);
					}
					if (Input::GetKeyDown(KeyCode::D))
					{
						Physics2D::ApplyForce(rb2d, { 10.0f, 0.0f }, true);
					}
					if (Input::GetKeyDown(KeyCode::A))
					{
						Physics2D::ApplyForce(rb2d, { -10.0f, 0.0f }, true);
					}
				}
			});
	}

private:
	bool m_Grounded = false;
};

REGISTER_HBL2_SYSTEM(NewSystem)
