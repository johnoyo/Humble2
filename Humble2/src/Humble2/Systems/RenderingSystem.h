#pragma once

#include "Base.h"
#include "Scene/ISystem.h"
#include "Core/Context.h"
#include "Resources/RefHandle.h"
#include "Resources/ResourceManager.h"
#include "Renderer/Renderer.h"
#include "Renderer/SceneRenderer.h"

namespace HBL2
{
	class RenderingSystem final : public ISystem
	{
	public:
		RenderingSystem() { Name = "RenderingSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnRender(CommandBuffer* commandBuffer, const RendererFrameData* renderData) override;
		virtual void OnDestroy() override;

	private:
		void SkinningPassSetup();
		void PresentPassSetup();

		Handle<BindGroup> GetPresentBindings() const;

		void SkinningPass(CommandBuffer* commandBuffer);
		void PresentPass(CommandBuffer* commandBuffer) const;

	private:
		ResourceManager* m_ResourceManager = nullptr;
		Scene* m_EditorScene = nullptr;
		SceneRenderer* m_SceneRenderer = nullptr;

		// Skinning compute pass resources.
		Handle<Shader> m_SkinningComputeShader;
		ShaderDescriptor::RenderPipeline::PackedVariant m_SkinningComputeVariant{};
		RefHandle<BindGroupLayout> m_SkinningBindGroupLayout;

		// Present pass resources.
		RefHandle<BindGroupLayout> m_PresentBindGroupLayout;
		RefHandle<BindGroup> m_PresentBindGroups[Renderer::FrameCount];
		UUID m_RendererResizeEventHandle = 0;

		Handle<Buffer> m_QuadVertexBuffer;
		Handle<Material> m_QuadMaterial;
		Handle<Shader> m_PresentShader;
	};
}
