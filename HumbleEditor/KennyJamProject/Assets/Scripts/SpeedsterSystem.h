#pragma once

#include "Humble2Core.h"

using namespace HBL2;

class SpeedsterSystem final : public HBL2::ISystem
{
public:
	virtual void OnCreate() override
	{
		m_Context->View<Speedster>()
			.Each([&](Speedster& speedster)
			{
				speedster.Power = 100.f;
				speedster.Score = 0.f;
				speedster.Speed = 7.f;
			});

		PhysicsEngine3D::Instance->OnCollisionEnterEvent([this](Physics::CollisionEnterEvent* collisionEnterEvent)
		{
		});

		PhysicsEngine3D::Instance->OnTriggerEnterEvent([this](Physics::TriggerEnterEvent* triggerEnterEvent)
		{
			if (m_Context->HasComponent<Speedster>(triggerEnterEvent->entityA) && m_Context->HasComponent<PowerUp>(triggerEnterEvent->entityB))
			{
				HBL2_INFO("[TRIGGER] Got power up!");

				auto& speedster = m_Context->GetComponent<Speedster>(triggerEnterEvent->entityA);
				auto& powerUp = m_Context->GetComponent<PowerUp>(triggerEnterEvent->entityB);

				speedster.Power += powerUp.Value;

				Prefab::Destroy(triggerEnterEvent->entityB);
			}

			if (m_Context->HasComponent<Speedster>(triggerEnterEvent->entityA) && m_Context->HasComponent<Coin>(triggerEnterEvent->entityB))
			{
				HBL2_INFO("[TRIGGER] Got coin!");

				auto& speedster = m_Context->GetComponent<Speedster>(triggerEnterEvent->entityA);
				auto& coin = m_Context->GetComponent<Coin>(triggerEnterEvent->entityB);

				speedster.Score += coin.Value;

				Prefab::Destroy(triggerEnterEvent->entityB);
			}
		});
	}

	virtual void OnUpdate(float ts) override
	{
		m_Context->View<Speedster, Component::Transform, Component::AudioSource>()
			.Each([&](Speedster& speedster, Component::Transform& tr, Component::AudioSource& audio)
			{
				if (!speedster.Enabled)
				{
					return;
				}

				// Lose if the car gets rotated too much.
				if (tr.Rotation.y >= 60.f || tr.Rotation.y <= -120.f)
				{
					speedster.Enabled = false;
					audio.Enabled = true;
				}
				if (tr.Rotation.x >= 170 || tr.Rotation.x <= -170)
				{
					speedster.Enabled = false;
					audio.Enabled = true;
				}

				// Lose if the car goes too far behind.
				if (tr.Translation.x <= -20.f && tr.Translation.z >= 15.f)
				{
					speedster.Enabled = false;
					audio.Enabled = true;
				}

				// Lose if the car runs out of power.
				if (speedster.Power <= 0.f)
				{
					speedster.Enabled = false;
					audio.Enabled = true;
				}

				speedster.Power -= 5.f * ts;
				speedster.Score += 100.f * ts;

				m_Score = (uint64_t)speedster.Score;
				m_Power = (int64_t)speedster.Power;
			});
	}

	virtual void OnGuiRender(float ts) override
	{
		UI::CreatePanel(
			UI::Config{
				.id = "ScoreUI",
				.mode = UI::Rectagle {.color = { 0, 0, 0, 0 }, .borderColor = { 0, 0, 0, 0 } },
				.layout = {
					.sizing =
					{
						.width = UI::Width::Grow(),
						.height = UI::Height::Grow(),
					},
					.padding = { UI::Utils::GetViewportSize().x / 32.f, UI::Utils::GetViewportSize().y / 18.f }
				}
			}, [&](UI::Panel* parent)
			{
				m_ScoreStr = std::format("Score: {}", m_Score);
				m_PowerStr = std::format("Power: {}", m_Power);

				UI::Text(parent, m_ScoreStr.c_str(), {0.f, 0.f, 0.f, 255.f});
				UI::Text(parent, "       -", { 0.f, 0.f, 0.f, 255.f });
				UI::Text(parent, m_PowerStr.c_str(), {0.f, 0.f, 0.f, 255.f});
			});
	}

	virtual void OnFixedUpdate() override
	{
		m_Context->View<Speedster, Component::Rigidbody>()
			.Each([](Speedster& speedster, Component::Rigidbody& rb)
			{
				if (!speedster.Enabled)
				{
					return;
				}

				float angleYRadians = glm::radians(-30.f);

				float deltaX = cos(angleYRadians);
				float deltaZ = -sin(angleYRadians);

				glm::vec3 movement(deltaX, 0.0f, deltaZ);

				if (Input::GetKeyPress(KeyCode::W))
				{
					PhysicsEngine3D::Instance->SetLinearVelocity(rb, { -speedster.Force * deltaX, 0.f, -speedster.Force * deltaZ });
				}
				else if (Input::GetKeyDown(KeyCode::S))
				{
					PhysicsEngine3D::Instance->SetLinearVelocity(rb, { speedster.Force * deltaX, 0.f, speedster.Force * deltaZ });
				}
			});
	}

private:
	uint64_t m_Score = 0;
	int64_t m_Power = 0;

	std::string m_ScoreStr;
	std::string m_PowerStr;
};

REGISTER_HBL2_SYSTEM(SpeedsterSystem)
