#pragma once

#include "SceneRenderer.h"

#include "DrawList.h"
#include "UniformRingBuffer.h"

#include "Renderer/Renderer.h"
#include "Resources/ResourceManager.h"

namespace HBL2
{
	struct InvalidSceneRenderData
	{
	};

	class HBL2_API InvalidSceneRenderer final : public SceneRenderer
	{
	public:
		virtual ~InvalidSceneRenderer() = default;

		virtual void Initialize(Scene* scene, uint32_t maxLights, bool useEditorCamera) override;
		virtual void Gather(Entity mainCamera) override;
		virtual void Render(CommandBuffer* commandBuffer, uint32_t renderDataIndex) override;
		virtual void CleanUp() override;

	private:
		void TextureSetup();
		void RenderPassSetup();
		void PinkQuadPassSetup();
		void PinkQuadPass(CommandBuffer* commandBuffer, InvalidSceneRenderData* sceneRenderData);

	private:
		PoolReservation* m_Reservation = nullptr;
		Arena m_Arena;

		ResourceManager* m_ResourceManager = nullptr;
		UniformRingBuffer* m_UniformRingBuffer = nullptr;

		Scene* m_EditorScene = nullptr;
		InvalidSceneRenderData m_RenderData[Renderer::FrameCount]{};

		uint32_t m_MaxLights = 0;

		Handle<RenderPassLayout> m_RenderPassLayout;
		UUID m_RendererResizeEventHandle = 0;

		// Pink quad pass resources.
		Handle<RenderPass> m_PinkQuadRenderPass;
		Handle<Buffer> m_PinkQuadBuffer;
		Handle<BindGroup> m_PinkQuadBindGroup;
		Handle<BindGroupLayout> m_PinkQuadBindGroupLayout;
		Handle<Shader> m_PinkQuadShader;
		ShaderDescriptor::RenderPipeline::PackedVariant m_PinkQuadShaderVariantHash;
		Handle<Buffer> m_PinkQuadVertexBuffer;
		UUID m_PinkQuadPassResizeEventHandle = 0;
	};
}