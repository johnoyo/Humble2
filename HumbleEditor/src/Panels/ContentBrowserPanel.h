#pragma once

#include "EditorPanel.h"

#include "Asset\AssetManager.h"
#include "Utilities\SlangReflection.h"
#include "Utilities\Collections\StaticArray.h"

#include <filesystem>

namespace HBL2::Editor
{
	class ContentBrowserPanel final : public EditorPanel
	{
	public:
		ContentBrowserPanel(const std::string& name, EditorPanelSystem* owner);

		virtual void OnAttach() override;
		virtual void OnCreate() override;
		virtual void OnOpen() override;
		virtual void OnRender(float ts) override;
		virtual void OnClose() override;
		virtual void OnDestroy() override;

	private:
		void HandleContentBrowserDragAndDrop();
		void DrawDirectoryRecursive(const std::filesystem::path& path);
		void DrawContentBrowserContextMenu();

		void ClearMaterialContextMenuData();

	private:
		bool m_OpenNewFolderSetupPopup = false;
		bool m_OpenSceneSetupPopup = false;
		bool m_OpenShaderSetupPopup = false;
		uint32_t m_SelectedShaderType = 0;

		bool m_OpenMaterialSetupPopup = false;
		ShaderReflectionData m_ShaderReflectionData{};
		StaticArray<std::vector<uint8_t>, 8> m_ShaderUniformBufferData = {};
		uint32_t m_ShaderUniformBufferSize = 0;
		StaticArray<uint32_t, 8> m_ShaderUniformTextureData = {};
		uint32_t m_ShaderUniformTextureSize = 0;
		uint32_t m_SelectedMaterialType = 0;

		std::string m_ShaderNameBuffer = "New-Shader";
		std::string m_ScriptNameBuffer = "NewHelperScript";
		std::string m_ComponentNameBuffer = "NewComponent";
		std::string m_SystemNameBuffer = "NewSystem";
		std::string m_SceneNameBuffer = "NewScene";
		std::string m_FolderNameBuffer = "NewFolder";

		uint32_t m_ShaderAssetHandlePacked = 0;
		std::string m_MaterialNameBuffer = "New-Material";

		bool m_OpenScriptSetupPopup = false;
		bool m_OpenComponentSetupPopup = false;
		bool m_OpenHelperScriptSetupPopup = false;

		bool m_OpenDeleteConfirmationWindow = false;
		Handle<Asset> m_AssetToBeDeleted;

		std::string m_SearchQuery;

		int m_Topology = 3;
		int m_PolygonMode = 0;
		int m_CullMode = 2;
		int m_FrontFace = 1;
		bool m_BlendEnabled = false;
		bool m_ColorOutput = true;
		bool m_DepthEnabled = true;
		bool m_DepthWriteEnabled = false;
		int m_DepthTest = 1;
		bool m_StencilEnabled = true;

		JobContext m_MaterialTextureLoadingCtx;

		ResourceTask<Texture>* m_AlbedoMapTask = nullptr;
		ResourceTask<Texture>* m_NormalMapTask = nullptr;
		ResourceTask<Texture>* m_MetallicMapTask = nullptr;
		ResourceTask<Texture>* m_RoughnessMapTask = nullptr;
	};
}