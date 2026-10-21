#pragma once

#include "Base.h"
#include "Scene/ISystem.h"
#include "Core/Context.h"
#include "Resources/ResourceManager.h"
#include "Renderer/SceneRenderer.h"

namespace HBL2
{
	class RenderingSystem final : public ISystem
	{
	public:
		RenderingSystem() { Name = "RenderingSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnRender(const RendererFrameData* renderData) override;
		virtual void OnDestroy() override;

	private:
		void SkinningPassSetup();
		void PresentPassSetup();

		Entity GetMainCamera();

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
		Handle<Buffer> m_QuadVertexBuffer;
		Handle<Material> m_QuadMaterial;
		Handle<Shader> m_PresentShader;
	};
}
