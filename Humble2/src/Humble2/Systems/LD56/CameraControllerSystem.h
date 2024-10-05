#pragma once

#include "Scene\Scene.h"
#include "Scene\ISystem.h"

namespace LD56
{
	class CameraControllerSystem final : public HBL2::ISystem
	{
	public:
		CameraControllerSystem() { Name = "CameraControllerSystem"; }

		virtual void OnCreate() override
		{
			auto cameraEntity = m_Context->FindEntityByUUID(12276041074768058368);

			if (cameraEntity != entt::null)
			{
				m_Context->AddComponent<Component::CameraController>(cameraEntity);
			}
			else
			{
				HBL2_ERROR("Camera entity not found!");
			}

			auto playerEntity = m_Context->FindEntityByUUID(11084216877952962560);

			if (playerEntity != entt::null)
			{
				m_PlayerTransform = &m_Context->GetComponent<HBL2::Component::Transform>(playerEntity);
			}
			else
			{
				HBL2_ERROR("Player entity not found!");
			}
		}

		virtual void OnUpdate(float ts) override
		{
			m_Context->GetRegistry()
				.group<Component::CameraController>(entt::get<HBL2::Component::Transform, HBL2::Component::Camera>)
				.each([&](auto entity, Component::CameraController& controller, HBL2::Component::Transform& transform, HBL2::Component::Camera& camera)
				{
					glm::vec3 targetPosition = m_PlayerTransform->Translation - glm::vec3(0.0f, -2.0f, -4.0f);
					transform.Translation = glm::mix(transform.Translation, targetPosition, controller.SmoothFactor * ts);
				});
		}

	private:
		HBL2::Component::Transform* m_PlayerTransform;
	};
}