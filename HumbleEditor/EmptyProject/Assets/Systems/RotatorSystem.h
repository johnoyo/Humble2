#pragma once

#include "Humble2.h"

namespace EmptyProject
{
	class RotatorSystem final : public HBL2::ISystem
	{
		virtual void OnCreate() override
		{

		}

		virtual void OnUpdate(float ts) override
		{
			m_Context->GetRegistry()
				.view<HBL2::Component::Transform>(entt::exclude<HBL2::Component::Camera>)
				.each([&](HBL2::Component::Transform& transform)
				{
					if (!transform.Static)
					{
						transform.Rotation.z += 20.0f * ts;
					}
				});
		}
	};
}