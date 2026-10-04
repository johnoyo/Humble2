#pragma once

#include "SceneRenderer.h"

#include "DrawList.h"
#include "UniformRingBuffer.h"

#include "Renderer/Renderer.h"
#include "Resources/ResourceManager.h"

namespace HBL2
{
	struct ForwardPlusSceneRenderData
	{
		CameraSettings m_CameraSettings{};

		Handle<BindGroup> ShadowBindings;
		Handle<BindGroup> GlobalBindings2D;
		Handle<BindGroup> GlobalBindings3D;
	};

	class HBL2_API ForwardPlusSceneRenderer final : public SceneRenderer
	{
	public:
		virtual ~ForwardPlusSceneRenderer() = default;

		virtual void Initialize(Scene* scene, uint32_t maxLights) override;
		virtual void Gather(Entity mainCamera) override;
		virtual void Render(void* renderData, void* debugRenderData) override;
		virtual void CleanUp() override;

		virtual void* GetRenderData() override;

	private:
		Handle<BindGroup> GetShadowBindings() const { return m_RenderData[Renderer::Instance->GetFrameNumber() % Renderer::FrameCount].ShadowBindings; }
		Handle<BindGroup> GetGlobalBindings2D() const { return m_RenderData[Renderer::Instance->GetFrameNumber() % Renderer::FrameCount].GlobalBindings2D; }
		Handle<BindGroup> GetGlobalBindings3D() const { return m_RenderData[Renderer::Instance->GetFrameNumber() % Renderer::FrameCount].GlobalBindings3D; }

		void RenderPassSetup();
		void BindingsSetup();

		void PostProcessPassSetup();
		void PresentPassSetup();

		void PostProcessPass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData);
		void PresentPass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData);

		void GetViewProjection(ForwardPlusSceneRenderData* sceneRenderData, Entity mainCamera);

	private:
		PoolReservation* m_Reservation = nullptr;
		Arena m_Arena;

		ResourceManager* m_ResourceManager = nullptr;
		UniformRingBuffer* m_UniformRingBuffer = nullptr;

		Scene* m_EditorScene = nullptr;
		ForwardPlusSceneRenderData m_RenderData[Renderer::FrameCount]{};

		uint32_t m_MaxLights = 0;

		RefHandle<BindGroupLayout> m_ShadowBindingsLayout;
		RefHandle<BindGroupLayout> m_GlobalBindingsLayout2D;
		RefHandle<BindGroupLayout> m_GlobalBindingsLayout3D;

		Handle<RenderPassLayout> m_RenderPassLayout;

		// Post process pass resources.
		Handle<RenderPass> m_PostProcessRenderPass;
		Handle<Buffer> m_PostProcessBuffer;
		Handle<BindGroup> m_PostProcessBindGroup;
		Handle<BindGroupLayout> m_PostProcessBindGroupLayout;
		Handle<Shader> m_PostProcessShader;
		ShaderDescriptor::RenderPipeline::PackedVariant m_PostProcessShaderVariantHash;
		Handle<Buffer> m_PostProcessQuadVertexBuffer;

		// Present pass resources.
		Handle<Buffer> m_QuadVertexBuffer;
		Handle<Material> m_QuadMaterial;
		Handle<Shader> m_PresentShader;
	};
}