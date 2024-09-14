#pragma once

#include "Humble2.h"
#include "Asset\AssetManager.h"
#include "EditorComponents.h"

namespace HBL2
{
	namespace Editor
	{
		class EditorPanelSystem final : public HBL2::ISystem
		{
		public:
			virtual void OnCreate() override;
			virtual void OnUpdate(float ts) override;
			virtual void OnGuiRender(float ts) override;
			virtual void OnDestroy() override;

		private:
			void DrawHierachyPanel();
			void DrawPropertiesPanel();
			void DrawToolBarPanel();
			void DrawConsolePanel(float ts);
			void DrawStatsPanel(float ts);
			void DrawViewportPanel();
			void DrawContentBrowserPanel();
			void DrawPlayStopPanel();

			glm::vec2 m_ViewportSize = { 0.f, 0.f };
			std::filesystem::path m_EditorScenePath;
			std::filesystem::path m_CurrentDirectory;

			bool m_OpenShaderSetupPopup = false;
			bool m_OpenMaterialSetupPopup = false;

			Scene* m_ActiveScene = nullptr;
		};
	}
}