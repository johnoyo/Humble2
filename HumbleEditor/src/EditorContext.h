#pragma once

#include "Humble2.h"

#include "Asset\EditorAssetManager.h"

#include "Humble2\Systems\CameraSystem.h"

#include "Systems/TransformSystem.h"
#include "Systems/EditorPanelSystem.h"
#include "Systems/EditorCameraSystem.h"

namespace HBL2
{
	namespace Editor
	{
		class EditorContext final : public HBL2::Context
		{
		public:
			virtual void OnAttach() override;
			virtual void OnCreate() override;
			virtual void OnUpdate(float ts) override;
			virtual void OnFixedUpdate() override;
			virtual void OnGuiRender(float ts) override;
			virtual void OnGizmoRender(float ts) override;
			virtual void OnDestroy() override;
			virtual void OnDetach() override;

		private:
			bool OpenProject();
			void LoadProject();
			bool IsActiveSceneValid();

		private:
			Scene* m_EditorScene = nullptr;
			Scene* m_ActiveScene = nullptr;
			Scene* m_EmptyScene = nullptr;
			float m_AccumulatedTime = 0.0f;
		};
	}
}