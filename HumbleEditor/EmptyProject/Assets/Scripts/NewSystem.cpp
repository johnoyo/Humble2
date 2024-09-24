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
			.view<entt::entity>()
			.each([&](entt::entity entity)
				{
					if (m_Context->HasComponent<HBL2::Component::Transform>(entity) && !m_Context->HasComponent<HBL2::Component::Camera>(entity))
					{
						auto& transform = m_Context->GetComponent<HBL2::Component::Transform>(entity);
						transform.Rotation.y += 25.0f * ts;
					}
				});
	}
};

// Factory function to create the system
extern "C" __declspec(dllexport) HBL2::ISystem* CreateSystem()
{
	return new NewSystem();
}