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

	class RenderingSystem final : public ISystem
	{
	public:
		RenderingSystem() { Name = "RenderingSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnDestroy() override;

	private:
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

		Handle<RenderPassLayout> m_RenderPassLayout;

		Handle<RenderPassLayout> m_DepthOnlyRenderPassLayout;
		Handle<RenderPass> m_DepthOnlyRenderPass;
		Handle<FrameBuffer> m_DepthOnlyFrameBuffer;
		Handle<Material> m_DepthOnlyMaterial;
		Handle<Material> m_DepthOnlySpriteMaterial;
		Handle<BindGroup> m_DepthOnlyBindGroup;
		Handle<BindGroup> m_DepthOnlySpriteBindGroup;
		Handle<BindGroupLayout> m_DepthOnlyBindGroupLayout;
		Handle<Shader> m_DepthOnlyShader;
		Handle<Shader> m_DepthOnlySpriteShader;

		Handle<RenderPass> m_OpaqueRenderPass;
		Handle<RenderPass> m_TransparentRenderPass;
		Handle<RenderPass> m_PostProcessRenderPass;
		Handle<FrameBuffer> m_OpaqueFrameBuffer;
		Handle<FrameBuffer> m_TransparentFrameBuffer;
		Handle<FrameBuffer> m_PostProcessFrameBuffer;

		Handle<Buffer> m_PostProcessBuffer;
		Handle<BindGroup> m_PostProcessBindGroup;
		Handle<BindGroupLayout> m_PostProcessBindGroupLayout;
		Handle<Shader> m_PostProcessShader;
		Handle<Material> m_PostProcessMaterial;

		Handle<Mesh> m_SpriteMesh;
		Handle<Buffer> m_VertexBuffer;

		Handle<Buffer> m_QuadVertexBuffer;
		Handle<Mesh> m_QuadMesh;
		Handle<Material> m_QuadMaterial;

		uint32_t m_UBOStaticMeshOffset = 0.0f;
		uint32_t m_UBOStaticMeshSize = 0.0f;
		DrawList m_StaticMeshOpaqueDraws;
		DrawList m_StaticMeshTransparentDraws;
		DrawList m_PrePassStaticMeshDraws;

		uint32_t m_UBOSpriteOffset = 0.0f;
		uint32_t m_UBOSpriteSize = 0.0f;
		DrawList m_SpriteOpaqueDraws;
		DrawList m_SpriteTransparentDraws;
		DrawList m_PrePassSpriteDraws;
	};
}