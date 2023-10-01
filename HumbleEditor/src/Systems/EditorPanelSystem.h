#pragma once

#include "Humble2.h"

#include "EditorComponents.h"

namespace HBL2Editor
{
	class EditorPanelSystem final : public HBL2::ISystem
	{
	public:
		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnGuiRender(float ts) override;
		virtual void OnDestroy() override;

	private:
		void DrawHierachyPanel(HBL2::Scene* context);
		void DrawPropertiesPanel(HBL2::Scene* context);
		void DrawToolBarPanel(HBL2::Scene* context);
		void DrawConsolePanel(HBL2::Scene* context, float ts);
		void DrawStatsPanel(HBL2::Scene* context, float ts);
		void DrawViewportPanel(HBL2::Scene* context);
		void DrawContentBrowserPanel(HBL2::Scene* context);

		glm::vec2 m_ViewportSize = { 0.f, 0.f };
		std::filesystem::path m_EditorScenePath;
		std::filesystem::path m_CurrentDirectory;
	};
}