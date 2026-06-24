#pragma once

#include "EditorPanel.h"

#include "ECS/Entity.h"

namespace HBL2::Editor
{
	class HierachyPanel final : public EditorPanel
	{
	public:
		HierachyPanel(const std::string& name, EditorPanelSystem* owner);

		virtual void OnAttach() override;
		virtual void OnCreate() override;
		virtual void OnOpen() override;
		virtual void OnRender(float ts) override;
		virtual void OnClose() override;
		virtual void OnDestroy() override;

	private:
		void DrawHierachy(Entity entity, const auto& entities);
		void HandleHierachyPanelDragAndDrop();

	private:
		Entity m_EntityToBeDeleted = Entity::Null;
		Entity m_EntityToBeDuplicated = Entity::Null;
	};
}
