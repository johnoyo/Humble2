#pragma once

#include <string>

namespace HBL2::Editor
{
	class EditorPanelSystem;

	class EditorPanel
	{
	public:
		virtual ~EditorPanel() = default;

		virtual void OnAttach() = 0;
		virtual void OnCreate() = 0;
		virtual void OnOpen() = 0;
		virtual void OnRender(float ts) = 0;
		virtual void OnClose() = 0;
		virtual void OnDestroy() = 0;

		void SetOwner(EditorPanelSystem* owner);
		bool GotEnabled();
		bool GotDisabled();

		bool Enabled = true;
		std::string Name = "Unnamed Panel";

	protected:
		EditorPanelSystem* m_Owner = nullptr;
		bool m_CloseState = true;

	private:
		bool m_PreviousEnabledState = false;
	};
}