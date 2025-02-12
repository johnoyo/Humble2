#pragma once

#include "Humble2Core.h"

class NewSystem final : public HBL2::ISystem
{
public:
	virtual void OnCreate() override
	{
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
};

REGISTER_HBL2_SYSTEM(NewSystem)
