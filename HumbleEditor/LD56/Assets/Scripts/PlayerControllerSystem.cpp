#include "Humble2Core.h"

class PlayerControllerSystem final : public HBL2::ISystem
{
public:
	virtual void OnCreate() override
	{
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
					if (m_PlayerEntity == entity)
					{
						if (HBL2::Input::GetKeyDown(GLFW_KEY_W))
						{
							transform.Translation.z += 10.0f * ts;
						}
						if (HBL2::Input::GetKeyDown(GLFW_KEY_S))
						{
							transform.Translation.z += -10.0f * ts;
						}
						if (HBL2::Input::GetKeyDown(GLFW_KEY_D))
						{
							transform.Translation.x += 10.0f * ts;
						}
						if (HBL2::Input::GetKeyDown(GLFW_KEY_A))
						{
							transform.Translation.x += -10.0f * ts;
						}
					}
				});
	}

private:
	entt::entity m_PlayerEntity;
};

// Factory function to create the system
extern "C" __declspec(dllexport) HBL2::ISystem* CreateSystem()
{
	return new PlayerControllerSystem();
}