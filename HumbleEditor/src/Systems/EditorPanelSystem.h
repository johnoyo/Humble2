#pragma once

#include "Humble2.h"
#include "Scene\ISystem.h"

#include "EditorPanel.h"
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

			template<typename C>
			bool HasCustomEditor()
			{
				return m_CustomEditors.find(typeid(C).hash_code()) != m_CustomEditors.end();
			}

			template<typename C, typename E>
			bool DrawCustomEditor(const C& component)
			{
				E* customEditor = (E*)m_CustomEditors[typeid(C).hash_code()];
				customEditor->OnUpdate(component);
				return customEditor->GetRenderBaseEditor();
			}

			template<typename C, typename E>
			void InitCustomEditor()
			{
				E* customEditor = (E*)m_CustomEditors[typeid(C).hash_code()];
				customEditor->OnCreate();
			}

			template<typename C, typename E>
			void RegisterCustomEditor()
			{
				m_CustomEditors[typeid(C).hash_code()] = new E;
			}

		private:
			std::vector<EditorPanel*> m_EditorPanels;
			std::unordered_map<size_t, void*> m_CustomEditors;

			Handle<Asset> m_PreviouslySelectedAsset;
			JobContext m_MaterialShaderReflectionCtx;
			ShaderReflectionData m_ShaderReflectionData2;
			StaticArray<std::vector<uint8_t>, 8> m_ShaderUniformBufferData2;
			uint32_t m_ShaderUniformBufferSize2;
			StaticArray<uint32_t, 8> m_ShaderUniformTextureData2;
			uint32_t m_ShaderUniformTextureSize2;
			JobContext m_MaterialShaderResourceCtx;
			UUID m_CurrentShaderUUID = 0;
			bool m_MaterialShaderChanged = false;
			bool m_MaterialShaderNeedsReimport = false;
			bool m_MaterialNeedsReimport = false;
			bool m_MaterialShaderReflectionStarted = false;
			bool m_MaterialBindGroupNeedsReimport = false;
			ResourceTask<Material>* m_MaterialTask = nullptr;

			bool m_ShaderNeedsReimport = false;
			ResourceTask<Shader>* m_ShaderTask = nullptr;

			Scene* m_ActiveScene = nullptr;
			Handle<Scene> m_ActiveSceneTemp;
			glm::vec2 m_ViewportSize = { 0.f, 0.f };
			ImGuizmo::MODE m_GizmoMode = ImGuizmo::MODE::LOCAL;
			bool m_ProjectChanged = false;
			std::filesystem::path m_EditorScenePath;
			std::filesystem::path m_CurrentDirectory;
			Handle<Asset> m_SelectedAsset;

			friend class TrayPanel;
			friend class PlayStopPanel;
			friend class SystemsPanel;
			friend class StatsPanel;
			friend class ViewportPanel;
			friend class TopBarPanel;
			friend class EditorSettingsPanel;
			friend class ProjectSettingsPanel;
			friend class HierachyPanel;
			friend class ContentBrowserPanel;
			friend class PropertiesPanel;
		};
	}
}