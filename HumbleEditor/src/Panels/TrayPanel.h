#pragma once

#include "EditorPanel.h"
#include "ECS\Entity.h"

#include <vector>
#include <unordered_map>

namespace HBL2::Editor
{
	class TrayPanel final : public EditorPanel
	{
	public:
		TrayPanel(const std::string& name, EditorPanelSystem* owner);

		virtual void OnAttach() override;
		virtual void OnCreate() override;
		virtual void OnOpen() override;
		virtual void OnRender(float ts) override;
		virtual void OnClose() override;
		virtual void OnDestroy() override;

	private:
		bool m_HotReloadedDLL = false;
		std::vector<std::string> m_UserSystemNames;
		std::vector<std::string> m_UserComponentNames;
		std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>> m_SerializedUserComponents;
	};
}