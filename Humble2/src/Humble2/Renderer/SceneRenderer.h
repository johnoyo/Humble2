#pragma once

#include "Base.h"
#include "Scene/Scene.h"
#include "Renderer/CommandBuffer.h"

namespace HBL2
{
	struct PerDrawData
	{
		glm::mat4 Model = glm::mat4(1.0f);
		glm::mat4 InverseModel = glm::mat4(1.0f);
		glm::vec4 Color = { 0.0f, 0.0f, 0.0f, 0.0f };
	};

	struct PerDrawDataSprite
	{
		glm::mat4 Model = glm::mat4(1.0f);
		glm::vec4 Color = { 0.0f, 0.0f, 0.0f, 0.0f };
	};

	class HBL2_API SceneRenderer
	{
	public:
		virtual ~SceneRenderer() = default;

		virtual void Initialize(Scene* scene, uint32_t maxLights, bool useEditorCamera) = 0;
		virtual void Gather(Entity mainCamera) = 0;
		virtual void Render(CommandBuffer* commandBuffer, uint32_t renderDataIndex) = 0;
		virtual void CleanUp() = 0;

		virtual void* GetRenderData() = 0;

		Handle<Texture> GetTexture() const { return m_TargetTexture; }
		Handle<Texture> GetDepthTexture() const { return m_DepthTexture; }
		bool IsInitialized() const { return m_Initialized; }

	protected:
		Handle<Texture> m_TargetTexture;
		Handle<Texture> m_IntermediateTexture;
		Handle<Texture> m_DepthTexture;
		Scene* m_Scene = nullptr;
		bool m_Initialized = false;
	};
}