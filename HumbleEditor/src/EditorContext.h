#pragma once

#include "Humble2.h"

#include "Asset\EditorAssetManager.h"

#ifndef EMSCRIPTEN
	#include "Systems/EditorPanelSystem.h"
	#include "Systems/EditorCameraSystem.h"
#endif

namespace HBL2
{
	namespace Editor
	{
		class EditorContext final : public HBL2::Context
		{
		public:
			virtual void OnCreate() override;
			virtual void OnUpdate(float ts) override;
			virtual void OnGuiRender(float ts) override;
			virtual void OnDestroy() override;

		private:
			bool OpenEmptyProject();
			void LoadBuiltInAssets();

		private:
			Scene* m_EditorScene = nullptr;
			Scene* m_ActiveScene = nullptr;
			Scene* m_EmptyScene = nullptr;
		};
	}
}