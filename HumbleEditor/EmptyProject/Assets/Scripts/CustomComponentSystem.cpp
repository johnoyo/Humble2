#include "Humble2Core.h"

struct CustomComponent
{
	float value = 10.f;
};

class CustomComponentSystem final : public HBL2::ISystem
{
public:
	virtual void OnCreate() override
	{
		entt::entity cerberus = m_Context->FindEntityByUUID(5613358100414213120);

		if (cerberus != entt::null)
		{
			m_Context->AddComponent<CustomComponent>(cerberus).value = 20.f;
		}
	}

	virtual void OnUpdate(float ts) override
	{
		m_Context->GetRegistry()
			.group<CustomComponent>(entt::get<HBL2::Component::Transform>)
			.each([&](CustomComponent& custom, HBL2::Component::Transform& transform)
			{
				transform.Rotation.x += custom.value * ts;
			});
	}
};

// Factory function to create the system
extern "C" __declspec(dllexport) HBL2::ISystem* CreateSystem()
{
	return new CustomComponentSystem();
}