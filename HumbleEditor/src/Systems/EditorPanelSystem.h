#pragma once

#include "Humble2.h"
#include "Asset\AssetManager.h"
#include "Scene\SceneManager.h"

#include "Utilities\EntityPresets.h"

#include "ImGui\ImGuiRenderer.h"

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
			void DrawHierachy(entt::entity entity, const auto& entities);
			void DrawHierachyPanel();
			void DrawPropertiesPanel();
			void DrawToolBarPanel();
			void DrawConsolePanel(float ts);
			void DrawStatsPanel(float ts);
			void DrawViewportPanel();
			void DrawContentBrowserPanel();
			void DrawDirectoryRecursive(const std::filesystem::path& path);
			void DrawPlayStopPanel();
			void DrawSystemsPanel();
			void DrawTrayPanel();

			entt::entity m_EntityToBeDeleted = entt::null;

			glm::vec2 m_ViewportSize = { 0.f, 0.f };
			std::filesystem::path m_CurrentDirectory;
			std::filesystem::path m_EditorScenePath;

			bool m_OpenNewFolderSetupPopup = false;
			bool m_OpenSceneSetupPopup = false;
			bool m_OpenShaderSetupPopup = false;
			uint32_t m_SelectedShaderType = 0;
			bool m_OpenMaterialSetupPopup = false;
			uint32_t m_SelectedMaterialType = 0;
			bool m_OpenScriptSetupPopup = false;
			bool m_OpenComponentSetupPopup = false;
			bool m_OpenHelperScriptSetupPopup = false;

			bool m_OpenDeleteConfirmationWindow = false;
			Handle<Asset> m_AssetToBeDeleted;
			Handle<Asset> m_SelectedAsset;

			Scene* m_ActiveScene = nullptr;
			Handle<Scene> m_ActiveSceneTemp;
			bool m_ProjectChanged = false;

			ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::OPERATION::BOUNDS;
			float m_CameraPivotDistance = 5.0f;

			std::string m_SearchQuery;
		};
	}
}