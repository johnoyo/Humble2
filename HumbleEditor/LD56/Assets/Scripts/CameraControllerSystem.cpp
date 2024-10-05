#include "Humble2Core.h"

class CameraControllerSystem final : public HBL2::ISystem
{
public:
	virtual void OnCreate() override
	{
		m_CameraEntity = m_Context->FindEntityByUUID(12276041074768058368);

		if (m_CameraEntity == entt::null)
		{
			std::cout << "Camera entity not found!\n";
		}

		m_PlayerEntity = m_Context->FindEntityByUUID(2360173364745782784);

		if (m_PlayerEntity == entt::null)
		{
			std::cout << "Player entity not found!\n";
		}
	}

	virtual void OnUpdate(float ts) override
	{
		m_Context->GetRegistry()
			.view<HBL2::Component::Transform, HBL2::Component::Camera>()
			.each([&](auto entity, HBL2::Component::Transform& transform, HBL2::Component::Camera& camera)
			{
				if (m_CameraEntity == entity)
				{
					HBL2::Component::Transform& playerTransform = m_Context->GetComponent<HBL2::Component::Transform>(m_PlayerEntity);

					transform.Translation = playerTransform.Translation - glm::vec3(-5, -2, 0);
					transform.Rotation = playerTransform.Rotation - glm::vec3(20, -90, 0);
				}
			});
	}

private:
	entt::entity m_CameraEntity;
	entt::entity m_PlayerEntity;
};

// Factory function to create the system
extern "C" __declspec(dllexport) HBL2::ISystem * CreateSystem()
{
	return new CameraControllerSystem();
}