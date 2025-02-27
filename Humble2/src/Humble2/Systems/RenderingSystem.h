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
		void MeshRenderingSetup();
		void SpriteRenderingSetup();
		void FullScreenQuadSetup();

		bool IsInFrustum(const Component::Transform& transform);
		bool IsInFrustum(Handle<Mesh> meshHandle, const Component::Transform& transform);
		void StaticMeshPass();
		void SpritePass();
		void PostProcessPass();
		void CompositePass();

		void GetViewProjection();

	private:
		ResourceManager* m_ResourceManager = nullptr;
		UniformRingBuffer* m_UniformRingBuffer = nullptr;
		Scene* m_EditorScene = nullptr;
		LightData m_LightData{};
		CameraData m_CameraData{};
		Component::Camera::CameraFrustum m_CameraFrustum;

		Handle<RenderPassLayout> m_RenderPassLayout;

		Handle<RenderPass> m_MeshRenderPass;
		Handle<FrameBuffer> m_MeshFrameBuffer;

		Handle<Mesh> m_SpriteMesh;
		Handle<Buffer> m_VertexBuffer;
		Handle<RenderPass> m_SpriteRenderPass;
		Handle<FrameBuffer> m_SpriteFrameBuffer;

		Handle<Buffer> m_QuadVertexBuffer;
		Handle<Mesh> m_QuadMesh;
		Handle<Material> m_QuadMaterial;
	};
}