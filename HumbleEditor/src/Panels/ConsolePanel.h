#pragma once

#include "EditorPanel.h"

#include "ImGui/ImGuiRenderer.h"

namespace HBL2::Editor
{
	struct ConsoleViewState
	{
		bool ShowTrace = true;
		bool ShowInfo = true;
		bool ShowWarn = true;
		bool ShowError = true;
		bool ShowFatal = true;

		bool AutoScroll = true;
		bool Pause = false;

		ImGuiTextFilter Filter;
	};

	class ConsolePanel final : public EditorPanel
	{
	public:
		ConsolePanel(const std::string& name, EditorPanelSystem* owner);

		virtual void OnAttach() override;
		virtual void OnCreate() override;
		virtual void OnOpen() override;
		virtual void OnRender(float ts) override;
		virtual void OnClose() override;
		virtual void OnDestroy() override;

	private:
		ConsoleViewState m_View;

		// Persistent UI state
		uint64_t m_SelectedSeq = UINT64_MAX;
		bool m_DetailsOpen = true;
		float m_DetailsHeight = 160.0f; // only meaningful when open
	};
}
