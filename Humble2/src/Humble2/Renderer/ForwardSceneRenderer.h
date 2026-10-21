#pragma once

#include "SceneRenderer.h"

#include "DrawList.h"
#include "UniformRingBuffer.h"

#include "Scene/Scene.h"
#include "Renderer/Renderer.h"
#include "Resources/ResourceManager.h"

#include "Utilities/Collections/FixedArray.h"

namespace HBL2
{
	struct ForwardSceneRenderData
	{
		FixedArray<Light> m_LightData;
		FrameData m_FrameData{};
		CameraSettings m_CameraSettings{};
		Component::Camera::CameraFrustum m_CameraFrustum{};
		glm::mat4 m_OnlyRotationInViewProjection = glm::mat4(1.0f);
		glm::mat4 m_CameraProjection = glm::mat4(1.0f);

		Handle<Buffer> LightSpaceBuffer;

		Handle<BindGroup> ShadowBindings;
		Handle<BindGroup> GlobalBindings2D;
		Handle<BindGroup> GlobalBindings3D;

		uint32_t m_UBOStartingOffset = 0;
		uint32_t m_UBOEndingOffset = 0;
		DrawList m_MeshOpaqueDraws;
		DrawList m_MeshTransparentDraws;
		DrawList m_PrePassMeshDraws;
		DrawList m_ShadowPassMeshDraws;

		DrawList m_SpriteOpaqueDraws;
		DrawList m_SpriteTransparentDraws;
		DrawList m_PrePassSpriteDraws;
	};

	class HBL2_API ForwardSceneRenderer final : public SceneRenderer
	{
	public:
		virtual ~ForwardSceneRenderer() = default;

		virtual void Initialize(Scene* scene, uint32_t maxLights) override;
		virtual void Gather(Entity mainCamera) override;
		virtual void Render(CommandBuffer* commandBuffer, void* renderData, void* debugRenderData) override;
		virtual void CleanUp() override;

		virtual void* GetRenderData() override;

	protected:
		void TextureSetup();
		void RenderPassSetup();
		void BindingsSetup();

		Handle<BindGroup> GetShadowBindings() const { return m_RenderData[Renderer::Instance->GetFrameNumber() % Renderer::FrameCount].ShadowBindings; }
		Handle<BindGroup> GetGlobalBindings2D() const { return m_RenderData[Renderer::Instance->GetFrameNumber() % Renderer::FrameCount].GlobalBindings2D; }
		Handle<BindGroup> GetGlobalBindings3D() const { return m_RenderData[Renderer::Instance->GetFrameNumber() % Renderer::FrameCount].GlobalBindings3D; }

		void ShadowPassSetup();
		void DepthPrePassSetup();
		void GeometryPassSetup();
		void SpriteRenderingSetup();
		void SkyboxPassSetup();
		void PostProcessPassSetup();

		void GatherDraws(ForwardSceneRenderData* sceneRenderData);
		void GatherLights(ForwardSceneRenderData* sceneRenderData);

		void ShadowPass(CommandBuffer* commandBuffer, ForwardSceneRenderData* sceneRenderData);
		void DepthPrePass(CommandBuffer* commandBuffer, ForwardSceneRenderData* sceneRenderData);
        void GeometryPass(CommandBuffer* commandBuffer, ForwardSceneRenderData* sceneRenderData, RenderPassPool& renderPassPool);
		void OpaquePass(RenderPassRenderer* passRenderer, ForwardSceneRenderData* sceneRenderData);
		void TransparentPass(RenderPassRenderer* passRenderer, ForwardSceneRenderData* sceneRenderData);
        void SkyboxComputePass(CommandBuffer* commandBuffer, DrawList* skyboxDraws);
		void SkyboxPass(DrawList& skyboxDraws, RenderPassRenderer* passRenderer, ForwardSceneRenderData* sceneRenderData);
		void PostProcessPass(CommandBuffer* commandBuffer, ForwardSceneRenderData* sceneRenderData);
		void DebugPass(CommandBuffer* commandBuffer, void* debugRenderData);

		void GetViewProjection(ForwardSceneRenderData* sceneRenderData, Entity mainCamera);

	private:
		PoolReservation* m_Reservation = nullptr;
		Arena m_Arena;

		ResourceManager* m_ResourceManager = nullptr;
		UniformRingBuffer* m_UniformRingBuffer = nullptr;

		Scene* m_EditorScene = nullptr;
		ForwardSceneRenderData m_RenderData[Renderer::FrameCount]{};

		uint32_t m_MaxLights = 0;

		RefHandle<BindGroupLayout> m_ShadowBindingsLayout;
		RefHandle<BindGroupLayout> m_GlobalBindingsLayout2D;
		RefHandle<BindGroupLayout> m_GlobalBindingsLayout3D;
		
		Handle<RenderPassLayout> m_RenderPassLayout;
		UUID m_TargetTextureResizeEventHandle = 0;

		// Shadow pass resources.
		Handle<Texture> m_ShadowDepthTexture;
        Handle<RenderPass> m_ShadowRenderPassClear;
		Handle<RenderPass> m_ShadowRenderPassLoad;
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
		UUID m_DepthPassResizeEventHandle = 0;

		// Geometry pass resources.
		Handle<RenderPass> m_GeometryRenderPass;
		Handle<Mesh> m_SpriteMesh;
		Handle<Buffer> m_VertexBuffer;
		UUID m_GeometryPassResizeEventHandle = 0;

		// Skybox pass resources.
		RefHandle<BindGroupLayout> m_EquirectToSkyboxBindGroupLayout;
		Handle<Buffer> m_CaptureMatricesBuffer;
		Handle<Shader> m_EquirectToSkyboxShader;
		RefHandle<BindGroupLayout> m_SkyboxGlobalBindGroupLayout;
		Handle<BindGroup> m_SkyboxGlobalBindGroup;
		Handle<Shader> m_SkyboxShader;
		RefHandle<BindGroupLayout> m_SkyboxBindGroupLayout;
		Handle<BindGroup> m_ComputeBindGroup;
		ShaderDescriptor::RenderPipeline::PackedVariant m_SkyboxVariant{};
		ShaderDescriptor::RenderPipeline::PackedVariant m_ComputeVariant{};
		Handle<Buffer> m_CubeMeshBuffer;
		Handle<Mesh> m_CubeMesh;

		// Post process pass resources.
		Handle<RenderPass> m_PostProcessRenderPass;
		Handle<Buffer> m_PostProcessBuffer;
		Handle<BindGroup> m_PostProcessBindGroup;
		RefHandle<BindGroupLayout> m_PostProcessBindGroupLayout;
		Handle<Shader> m_PostProcessShader;
		ShaderDescriptor::RenderPipeline::PackedVariant m_PostProcessShaderVariantHash;
		Handle<Buffer> m_PostProcessQuadVertexBuffer;
		UUID m_PostProcessPassResizeEventHandle = 0;
	};
}
