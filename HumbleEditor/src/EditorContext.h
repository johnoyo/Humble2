#pragma once

#include "Humble2.h"

#include "Asset\EditorAssetManager.h"
#include "Renderer\RenderCommand.h"

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

		private:
			bool OpenEmptyProject();
			void RegisterAssets();

		private:
			Scene* m_EditorScene = nullptr;
			Scene* m_ActiveScene = nullptr;
			Scene* m_EmptyScene = nullptr;
		};
	}
}