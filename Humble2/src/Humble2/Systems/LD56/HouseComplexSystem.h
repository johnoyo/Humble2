#pragma once

#include "Base.h"

#include "Scene\Scene.h"
#include "Scene\ISystem.h"

namespace LD56
{
	class HouseComplexSystem final : public HBL2::ISystem
	{
	public:
		HouseComplexSystem() { Name = "HouseComplexSystem"; }

		virtual void OnCreate() override
		{
			auto houseEntity0 = m_Context->FindEntityByUUID(5458692348207903744);
			auto houseCollidersEntity0 = m_Context->FindEntityByUUID(4073195747501176320);
			auto houseEntity1 = m_Context->FindEntityByUUID(12705467087370573824);
			auto houseCollidersEntity1 = m_Context->FindEntityByUUID(1589691311269435136);
			auto houseEntity2 = m_Context->FindEntityByUUID(572588895030259968);
			auto houseCollidersEntity2 = m_Context->FindEntityByUUID(16787833864284327936);

			if (houseEntity0 != entt::null)
			{
				auto& houseComplex = m_Context->AddComponent<Component::HouseComplex>(houseEntity0);
				houseComplex.ResetPosition = m_Context->GetComponent<HBL2::Component::Transform>(houseEntity2).Translation;
				houseComplex.Colliders = houseCollidersEntity0;
			}

			if (houseEntity1 != entt::null)
			{
				auto& houseComplex = m_Context->AddComponent<Component::HouseComplex>(houseEntity1);
				houseComplex.ResetPosition = m_Context->GetComponent<HBL2::Component::Transform>(houseEntity2).Translation;
				houseComplex.Colliders = houseCollidersEntity1;
			}

			if (houseEntity2 != entt::null)
			{
				auto& houseComplex = m_Context->AddComponent<Component::HouseComplex>(houseEntity2);
				houseComplex.ResetPosition = m_Context->GetComponent<HBL2::Component::Transform>(houseEntity2).Translation;
				houseComplex.Colliders = houseCollidersEntity2;
			}

			m_Player = m_Context->FindEntityByUUID(11084216877952962560);

			if (m_Player == entt::null)
			{
				HBL2_CORE_INFO("Player entity not found!");
			}
		}

		virtual void OnUpdate(float ts) override
		{
			Component::PlayerController& playerController = m_Context->GetComponent<Component::PlayerController>(m_Player);

			m_Context->GetRegistry()
				.group<Component::HouseComplex>(entt::get<HBL2::Component::Transform>)
				.each([&](auto entity, Component::HouseComplex& houseComplex, HBL2::Component::Transform& transform)
				{
					if (!playerController.Alive)
					{
						houseComplex.Enabled = false;
					}

					if (houseComplex.Enabled)
					{
						transform.Translation.z += m_Speed * ts;

						if (transform.Translation.z >= 80.0f)
						{
							m_Speed += 1.0f;
							playerController.MovementSpeed += 0.1f;
							transform.Translation = houseComplex.ResetPosition;
						}
					}
				});
		}

	private:
		entt::entity m_Player;
		float m_Speed = 10.0f;
	};
}