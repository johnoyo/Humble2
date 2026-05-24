#pragma once

#include "Humble2.h"
#include "Scene\ISystem.h"

#include "EditorComponents.h"

#include "Asset\AssetManager.h"
#include "Scene\SceneManager.h"
#include "Utilities\EntityPresets.h"
#include "ImGui\ImGuiRenderer.h"

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
			void DrawHierachy(Entity entity, const auto& entities);
			void DrawHierachyPanel();
			void HandleHierachyPanelDragAndDrop();
			void DrawPropertiesPanel();
			void DrawToolBarPanel();
			void DrawConsolePanel(float ts);
			void DrawStatsPanel(float ts);
			void DrawViewportPanel();
			void DrawContentBrowserPanel();
			void HandleContentBrowserDragAndDrop();
			void DrawDirectoryRecursive(const std::filesystem::path& path);
			void DrawContentBrowserContextMenu();
			void DrawPlayStopPanel();
			void DrawSystemsPanel();
			void DrawTrayPanel();

			Entity m_EntityToBeDeleted = Entity::Null;
			Entity m_EntityToBeDuplicated = Entity::Null;

			glm::vec2 m_ViewportSize = { 0.f, 0.f };
			std::filesystem::path m_CurrentDirectory;
			std::filesystem::path m_EditorScenePath;

			bool m_ShowProjectSettingsWindow = false;
			bool m_ShowEditorSettingsWindow = false;

			bool m_OpenNewFolderSetupPopup = false;
			bool m_OpenSceneSetupPopup = false;
			bool m_OpenShaderSetupPopup = false;
			uint32_t m_SelectedShaderType = 0;

			bool m_OpenMaterialSetupPopup = false;
			ShaderReflectionData m_ShaderReflectionData;
			StaticArray<std::vector<uint8_t>, 8> m_ShaderUniformBufferData;
			uint32_t m_ShaderUniformBufferSize;
			StaticArray<uint32_t, 8> m_ShaderUniformTextureData;
			uint32_t m_ShaderUniformTextureSize;
			uint32_t m_SelectedMaterialType = 0;

			bool m_OpenScriptSetupPopup = false;
			bool m_OpenComponentSetupPopup = false;
			bool m_OpenHelperScriptSetupPopup = false;

			bool m_OpenDeleteConfirmationWindow = false;
			Handle<Asset> m_AssetToBeDeleted;
			Handle<Asset> m_SelectedAsset;
			Handle<Asset> m_PreviouslySelectedAsset;
			JobContext m_MaterialShaderReflectionCtx;
			ShaderReflectionData m_ShaderReflectionData2;
			StaticArray<std::vector<uint8_t>, 8> m_ShaderUniformBufferData2;
			uint32_t m_ShaderUniformBufferSize2;
			StaticArray<uint32_t, 8> m_ShaderUniformTextureData2;
			uint32_t m_ShaderUniformTextureSize2;

			Scene* m_ActiveScene = nullptr;
			Handle<Scene> m_ActiveSceneTemp;
			bool m_ProjectChanged = false;

			bool m_HotReloadedDLL = false;
			std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>> m_SerializedUserComponents;

			ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::OPERATION::BOUNDS;
			ImGuizmo::MODE m_GizmoMode = ImGuizmo::MODE::LOCAL;
			float m_CameraPivotDistance = 5.0f;

			std::string m_SearchQuery;
		};
	}
}