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
	}

	virtual void OnUpdate(float ts) override
	{
		m_Context->GetRegistry()
			.view<NewComponent>()
			.each([&](NewComponent& newComponent)
			{
				if (HBL2::Input::GetKeyPress(GLFW_KEY_C))
				{
					HBL2_INFO("Hello!");
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
				if (rb2d.Type == Component::Rigidbody2D::BodyType::Dynamic)
				{
					if (Input::GetKeyPress(GLFW_KEY_W) && m_Grounded)
					{
						m_Grounded = false;
						Physics2D::ApplyLinearImpulse(rb2d, { 0.0f, 10.0f }, true);
					}
					if (Input::GetKeyDown(GLFW_KEY_D))
					{
						Physics2D::ApplyForce(rb2d, { 10.0f, 0.0f }, true);
					}
					if (Input::GetKeyDown(GLFW_KEY_A))
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
