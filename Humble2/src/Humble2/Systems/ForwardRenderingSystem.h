#pragma once

#include "Core\Context.h"

#include "Scene\ISystem.h"
#include "Scene\Components.h"

#include "Resources\ResourceManager.h"

#include "Renderer\Renderer.h"
#include "Renderer\UniformRingBuffer.h"

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

	class ForwardRenderingSystem final : public ISystem
	{
	public:
		ForwardRenderingSystem() { Name = "ForwardRenderingSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnDestroy() override;

	private:
		void ShadowPassSetup();
		void DepthPrePassSetup();
		void OpaquePassSetup();
		void TransparentPassSetup();
		void SpriteRenderingSetup();
		void SkyboxPassSetup();
		void PostProcessPassSetup();
		void PresentPassSetup();

		bool IsInFrustum(const Component::Transform& transform);
		bool IsInFrustum(Handle<Mesh> meshHandle, const Component::Transform& transform);

		void GatherDraws();
		void GatherLights();
		void ShadowPass(CommandBuffer* commandBuffer);
		void DepthPrePass(CommandBuffer* commandBuffer);
		void OpaquePass(CommandBuffer* commandBuffer);
		void TransparentPass(CommandBuffer* commandBuffer);
		void SkyboxPass(CommandBuffer* commandBuffer);
		void PostProcessPass(CommandBuffer* commandBuffer);
		void PresentPass(CommandBuffer* commandBuffer);

		void GetViewProjection();

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
		Handle<FrameBuffer> m_ShadowFrameBufferClear;
		Handle<FrameBuffer> m_ShadowFrameBufferLoad;
		Handle<RenderPass> m_DepthOnlyRenderPassLoad;
		Handle<Shader> m_ShadowPrePassShader;
		Handle<Material> m_ShadowPrePassMaterial;

		Handle<RenderPassLayout> m_DepthOnlyRenderPassLayout;
		Handle<RenderPass> m_DepthOnlyRenderPass;
		Handle<FrameBuffer> m_DepthOnlyFrameBuffer;
		Handle<Material> m_DepthOnlyMaterial;
		Handle<Material> m_DepthOnlySpriteMaterial;
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

		Handle<Buffer> m_QuadVertexBuffer;
		Handle<Mesh> m_QuadMesh;
		Handle<Material> m_QuadMaterial;
		Handle<Shader> m_PresentShader;

		uint32_t m_UBOStaticMeshOffset = 0.0f;
		uint32_t m_UBOStaticMeshSize = 0.0f;
		DrawList m_StaticMeshOpaqueDraws;
		DrawList m_StaticMeshTransparentDraws;
		DrawList m_PrePassStaticMeshDraws;
		DrawList m_ShadowPassStaticMeshDraws;

		uint32_t m_UBOSpriteOffset = 0.0f;
		uint32_t m_UBOSpriteSize = 0.0f;
		DrawList m_SpriteOpaqueDraws;
		DrawList m_SpriteTransparentDraws;
		DrawList m_PrePassSpriteDraws;
		DrawList m_ShadowPassSpriteDraws;
	};
}