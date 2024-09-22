#pragma once

#include "Humble2.h"
#include "EditorComponents.h"

namespace HBL2
{
	namespace Editor
	{
		class EditorCameraSystem final : public ISystem
		{
		public:
			virtual void OnCreate() override;
			virtual void OnUpdate(float ts) override;
			virtual void OnDestroy() override;

		private:
			Component::EditorCamera* m_EditorCamera = nullptr;
			HBL2::Component::Transform* m_Transform = nullptr;
			float m_Timestep;
		};
	}
}