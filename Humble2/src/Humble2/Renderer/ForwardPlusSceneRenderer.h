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
		FixedArray<Light> m_LightData;
		FrameData m_FrameData{};
		CameraSettings m_CameraSettings{};
		Component::Camera::CameraFrustum m_CameraFrustum{};
		glm::mat4 m_OnlyRotationInViewProjection = glm::mat4(1.0f);
		glm::mat4 m_CameraProjection = glm::mat4(1.0f);

		Handle<Buffer> LightsSSBO;
		Handle<Buffer> LightSpaceBuffer;

		Handle<BindGroup> ShadowBindings;
		Handle<BindGroup> GlobalBindings2D;
		Handle<BindGroup> GlobalBindingsOpaque3D;
		Handle<BindGroup> GlobalBindingsTransparent3D;
		Handle<BindGroup> LightCullingBindings;

		uint32_t m_UBOStartingOffset = 0;
		uint32_t m_UBOEndingOffset = 0;
		DrawList m_StaticMeshOpaqueDraws;
		DrawList m_StaticMeshTransparentDraws;
		DrawList m_PrePassStaticMeshDraws;
		DrawList m_ShadowPassStaticMeshDraws;

		DrawList m_SpriteOpaqueDraws;
		DrawList m_SpriteTransparentDraws;
		DrawList m_PrePassSpriteDraws;
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
		Handle<BindGroup> GetGlobalBindingsOpaque3D() const { return m_RenderData[Renderer::Instance->GetFrameNumber() % Renderer::FrameCount].GlobalBindingsOpaque3D; }
		Handle<BindGroup> GetGlobalBindingsTransparent3D() const { return m_RenderData[Renderer::Instance->GetFrameNumber() % Renderer::FrameCount].GlobalBindingsTransparent3D; }
		Handle<BindGroup> GetLightCullingBindings() const { return m_RenderData[Renderer::Instance->GetFrameNumber() % Renderer::FrameCount].LightCullingBindings; }

		void RenderPassSetup();
		void BindingsSetup();

		void ShadowPassSetup();
		void DepthPrePassSetup();
		void GridFrustumsComputePassSetup();
		void LightCullingComputePassSetup();
		void GeometryPassSetup();
		void SkyboxPassSetup();
		void PostProcessPassSetup();
		void PresentPassSetup();

		void GatherDraws(ForwardPlusSceneRenderData* sceneRenderData);
		void GatherLights(ForwardPlusSceneRenderData* sceneRenderData);

		void ShadowPass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData);
		void DepthPrePass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData);
		void GridFrustumsComputePass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData);
		void LightCullingComputePass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData);
		void GeometryPass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData);
		void OpaquePass(RenderPassRenderer* passRenderer, ForwardPlusSceneRenderData* sceneRenderData);
		void TransparentPass(RenderPassRenderer* passRenderer, ForwardPlusSceneRenderData* sceneRenderData);
		void SkyboxComputePass(CommandBuffer* commandBuffer, DrawList* skyboxDraws);
		void SkyboxPass(DrawList& skyboxDraws, RenderPassRenderer* passRenderer, ForwardPlusSceneRenderData* sceneRenderData);
		void PostProcessPass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData);
		void DebugPass(CommandBuffer* commandBuffer, void* debugRenderData);
		void PresentPass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData);

		void GetViewProjection(ForwardPlusSceneRenderData* sceneRenderData, Entity mainCamera);
		void CreateGridFrustumsComputeBindGroup(uint32_t blockSize, uint32_t width, uint32_t height, const glm::mat4& cameraProjection);
		void CreateLightCullingComputeBindGroup(uint32_t blockSize, uint32_t width, uint32_t height, ForwardPlusSceneRenderData* sceneRenderData);

	private:
		PoolReservation* m_Reservation = nullptr;
		Arena m_Arena;

		ResourceManager* m_ResourceManager = nullptr;
		UniformRingBuffer* m_UniformRingBuffer = nullptr;

		Scene* m_EditorScene = nullptr;
		ForwardPlusSceneRenderData m_RenderData[Renderer::FrameCount]{};

		uint32_t m_MaxLights = 0;
		Entity m_MainCamera = Entity::Null;

		RefHandle<BindGroupLayout> m_ShadowBindingsLayout;
		RefHandle<BindGroupLayout> m_GlobalBindingsLayout2D;
		RefHandle<BindGroupLayout> m_GlobalBindingsLayout3D;

		Handle<RenderPassLayout> m_RenderPassLayout;

		// Shadow pass resources.
		Handle<Texture> m_ShadowDepthTexture;
		Handle<RenderPass> m_ShadowRenderPass;
		Handle<Shader> m_ShadowPrePassShader;
		Handle<Material> m_ShadowPrePassMaterial;
		ShaderDescriptor::RenderPipeline::PackedVariant m_ShadowPrePassMaterialHash = g_NullVariant;

		// Depth pre pass resources.
		Handle<RenderPassLayout> m_DepthOnlyRenderPassLayout;
		Handle<RenderPass> m_DepthOnlyRenderPass;
		Handle<Material> m_DepthOnlyMaterial;
		ShaderDescriptor::RenderPipeline::PackedVariant m_DepthOnlyMaterialHash = g_NullVariant;
		Handle<Material> m_DepthOnlySpriteMaterial;
		ShaderDescriptor::RenderPipeline::PackedVariant m_DepthOnlySpriteMaterialHash = g_NullVariant;
		Handle<BindGroup> m_DepthOnlyMeshBindGroup;
		Handle<BindGroup> m_DepthOnlySpriteBindGroup;
		Handle<Shader> m_DepthOnlyShader;
		Handle<Shader> m_DepthOnlySpriteShader;

		// Grid frustums compute pass resources.
		Handle<Shader> m_GridFrustumsComputeShader;
		ShaderDescriptor::RenderPipeline::PackedVariant m_GridFrustumsComputeVariant{};
		Handle<BindGroupLayout> m_GridFrustumsBindGroupLayout;
		Handle<Buffer> m_FrustumsBuffer;
		Handle<BindGroup> m_GridFrustumsBindGroup;

		// Light culling compute pass resources.
		Handle<Shader> m_LightCullingComputeShader;
		ShaderDescriptor::RenderPipeline::PackedVariant m_LightCullingComputeVariant{};
		Handle<BindGroupLayout> m_LightCullingBindGroupLayout;

		Handle<Buffer> o_LightIndexCounter;
		Handle<Buffer> t_LightIndexCounter;
		Handle<Buffer> o_LightIndexList;
		Handle<Buffer> t_LightIndexList;
		Handle<Texture> o_LightGrid;
		Handle<Texture> t_LightGrid;

		// Geometry pass resources.
		Handle<RenderPass> m_GeometryRenderPass;
		Handle<Mesh> m_SpriteMesh;
		Handle<Buffer> m_VertexBuffer;

		// Skybox pass resources.
		Handle<BindGroupLayout> m_EquirectToSkyboxBindGroupLayout;
		Handle<Buffer> m_CaptureMatricesBuffer;
		Handle<Shader> m_EquirectToSkyboxShader;
		Handle<BindGroupLayout> m_SkyboxGlobalBindGroupLayout;
		Handle<BindGroup> m_SkyboxGlobalBindGroup;
		Handle<Shader> m_SkyboxShader;
		Handle<BindGroupLayout> m_SkyboxBindGroupLayout;
		Handle<BindGroup> m_ComputeBindGroup;
		ShaderDescriptor::RenderPipeline::PackedVariant m_SkyboxVariant{};
		ShaderDescriptor::RenderPipeline::PackedVariant m_ComputeVariant{};
		Handle<Buffer> m_CubeMeshBuffer;
		Handle<Mesh> m_CubeMesh;

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