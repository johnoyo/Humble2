#pragma once

#include "EditorPanel.h"

#include "Asset\AssetManager.h"
#include "Utilities\SlangReflection.h"
#include "Utilities\Collections\StaticArray.h"

namespace HBL2::Editor
{
	class PropertiesPanel final : public EditorPanel
	{
	public:
		PropertiesPanel(const std::string& name, EditorPanelSystem* owner);

		virtual void OnAttach() override;
		virtual void OnCreate() override;
		virtual void OnOpen() override;
		virtual void OnRender(float ts) override;
		virtual void OnClose() override;
		virtual void OnDestroy() override;

	private:
		Handle<Asset> m_PreviouslySelectedAsset;
		JobContext m_MaterialShaderReflectionCtx;
		ShaderReflectionData m_ShaderReflectionData;
		StaticArray<std::vector<uint8_t>, 8> m_ShaderUniformBufferData = {};
		uint32_t m_ShaderUniformBufferSize = 0;
		StaticArray<uint32_t, 8> m_ShaderUniformTextureData = {};
		uint32_t m_ShaderUniformTextureSize = 0;
		JobContext m_MaterialShaderResourceCtx;
		JobContext m_MaterialTextureLoadingCtx;
		JobContext m_ShaderTextureLoadingCtx;
		UUID m_CurrentShaderUUID = 0;
		bool m_MaterialShaderChanged = false;
		bool m_MaterialShaderNeedsReimport = false;
		bool m_MaterialNeedsReimport = false;
		bool m_MaterialShaderReflectionStarted = false;
		bool m_MaterialBindGroupNeedsReimport = false;
		ResourceTask<Material>* m_MaterialTask = nullptr;

		bool m_ShaderNeedsReimport = false;
		ResourceTask<Shader>* m_ShaderTask = nullptr;
	};
}