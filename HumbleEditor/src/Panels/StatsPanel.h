#pragma once

#include "EditorPanel.h"

namespace HBL2::Editor
{
	class StatsPanel final : public EditorPanel
	{
	public:
		StatsPanel(const std::string& name, EditorPanelSystem* owner);

		virtual void OnAttach() override;
		virtual void OnCreate() override;
		virtual void OnOpen() override;
		virtual void OnRender(float ts) override;
		virtual void OnClose() override;
		virtual void OnDestroy() override;

	private:
	};
}