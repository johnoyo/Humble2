#pragma once

#include "SceneRenderer.h"

#include "DrawList.h"
#include "UniformRingBuffer.h"

#include <Scene/Scene.h>
#include "Renderer/Renderer.h"
#include <Resources/ResourceManager.h>

namespace HBL2
{
	struct PerDrawData
	{
		glm::mat4 Model = glm::mat4(1.0f);
		glm::mat4 InverseModel = glm::mat4(1.0f);
		glm::vec4 Color = { 0.0f, 0.0f, 0.0f, 0.0f };
		float Glossiness = 0.0f;
	};

	struct PerDrawDataSprite
	{
		glm::mat4 Model = glm::mat4(1.0f);
		glm::vec4 Color = { 0.0f, 0.0f, 0.0f, 0.0f };
	};

	class HBL2_API ForwardSceneRenderer final : public SceneRenderer
	{
	public:
		virtual void Initialize(Scene* scene) override;
		virtual void Render(Entity mainCamera) override;
		virtual void CleanUp() override;

	private:
		void ShadowPassSetup();
		void DepthPrePassSetup();
		void OpaquePassSetup();
		void TransparentPassSetup();
		void SpriteRenderingSetup();
		void SkyboxPassSetup();
		void PostProcessPassSetup();
		void PresentPassSetup();

		void GatherDraws();
		void GatherLights();

		void ShadowPass(CommandBuffer* commandBuffer);
		void DepthPrePass(CommandBuffer* commandBuffer);
		void OpaquePass(CommandBuffer* commandBuffer);
		void TransparentPass(CommandBuffer* commandBuffer);
		void SkyboxPass(CommandBuffer* commandBuffer);
		void PostProcessPass(CommandBuffer* commandBuffer);
		void PresentPass(CommandBuffer* commandBuffer);

		void GetViewProjection(Entity mainCamera);

		void CreateAlignedMatrixArray(const glm::mat4* matrices, size_t count, uint32_t alignedSize);

	private:
		ResourceManager* m_ResourceManager = nullptr;
		UniformRingBuffer* m_UniformRingBuffer = nullptr;

		Scene* m_EditorScene = nullptr;
		LightData m_LightData{};
		CameraData m_CameraData{};
		CameraSettings m_CameraSettings{};
		Component::Camera::CameraFrustum m_CameraFrustum{};
		glm::mat4 m_OnlyRotationInViewProjection = glm::mat4(1.0f);
		glm::mat4 m_CameraProjection = glm::mat4(1.0f);

		Handle<RenderPassLayout> m_RenderPassLayout;

		Handle<Texture> m_ShadowDepthTexture;
		Handle<FrameBuffer> m_ShadowFrameBuffer;
		Handle<RenderPass> m_ShadowRenderPass;
		Handle<Shader> m_ShadowPrePassShader;
		Handle<Material> m_ShadowPrePassMaterial;
		uint64_t m_ShadowPrePassMaterialHash;

		Handle<RenderPassLayout> m_DepthOnlyRenderPassLayout;
		Handle<RenderPass> m_DepthOnlyRenderPass;
		Handle<FrameBuffer> m_DepthOnlyFrameBuffer;
		Handle<Material> m_DepthOnlyMaterial;
		uint64_t m_DepthOnlyMaterialHash;
		Handle<Material> m_DepthOnlySpriteMaterial;
		uint64_t m_DepthOnlySpriteMaterialHash;
		Handle<BindGroup> m_DepthOnlyMeshBindGroup;
		Handle<BindGroup> m_DepthOnlySpriteBindGroup;
		Handle<BindGroupLayout> m_DepthOnlyBindGroupLayout;
		Handle<Shader> m_DepthOnlyShader;
		Handle<Shader> m_DepthOnlySpriteShader;

		Handle<RenderPass> m_OpaqueRenderPass;
		Handle<RenderPass> m_TransparentRenderPass;
		Handle<FrameBuffer> m_OpaqueFrameBuffer;
		Handle<FrameBuffer> m_TransparentFrameBuffer;

		Handle<BindGroupLayout> m_EquirectToSkyboxBindGroupLayout;
		Handle<Buffer> m_CaptureMatricesBuffer;
		Handle<Shader> m_EquirectToSkyboxShader;
		Handle<BindGroupLayout> m_SkyboxGlobalBindGroupLayout;
		Handle<BindGroup> m_SkyboxGlobalBindGroup;
		Handle<Shader> m_SkyboxShader;
		Handle<BindGroupLayout> m_SkyboxBindGroupLayout;
		Handle<BindGroup> m_ComputeBindGroup;
		ShaderDescriptor::RenderPipeline::Variant m_SkyboxVariant{};
		ShaderDescriptor::RenderPipeline::Variant m_ComputeVariant{};

		Handle<RenderPass> m_PostProcessRenderPass;
		Handle<FrameBuffer> m_PostProcessFrameBuffer;
		Handle<Buffer> m_PostProcessBuffer;
		Handle<BindGroup> m_PostProcessBindGroup;
		Handle<BindGroupLayout> m_PostProcessBindGroupLayout;
		Handle<Shader> m_PostProcessShader;
		Handle<Material> m_PostProcessMaterial;
		Handle<Buffer> m_CubeMeshBuffer;
		Handle<Mesh> m_CubeMesh;

		Handle<Mesh> m_SpriteMesh;
		Handle<Buffer> m_VertexBuffer;

		Handle<Buffer> m_PostProcessQuadVertexBuffer;
		Handle<Mesh> m_PostProcessQuadMesh;

		Handle<Buffer> m_QuadVertexBuffer;
		Handle<Mesh> m_QuadMesh;
		Handle<Material> m_QuadMaterial;
		Handle<Shader> m_PresentShader;

		uint32_t m_UBOStartingOffset = 0.0f;
		DrawList m_StaticMeshOpaqueDraws;
		DrawList m_StaticMeshTransparentDraws;
		DrawList m_PrePassStaticMeshDraws;
		DrawList m_ShadowPassStaticMeshDraws;

		DrawList m_SpriteOpaqueDraws;
		DrawList m_SpriteTransparentDraws;
		DrawList m_PrePassSpriteDraws;
		DrawList m_ShadowPassSpriteDraws;

		std::vector<uint8_t> m_LightSpaceMatricesData;
	};
}