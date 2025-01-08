#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"
#include "Renderer\Renderer.h"
#include "Resources\ResourceManager.h"

#include "Renderer\UniformRingBuffer.h"

#include "Utilities\ShaderUtilities.h"
#include "Utilities\TextureUtilities.h"

#include <glm\gtx\quaternion.hpp>

namespace HBL2
{
	struct PerDrawData
	{
		glm::mat4 Model;
		glm::mat4 InverseModel;
		glm::vec4 Color;
		float Glossiness;
	};

	class StaticMeshRenderingSystem final : public ISystem
	{
	public:
		StaticMeshRenderingSystem() { Name = "StaticMeshRenderingSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnDestroy() override;

	private:
		void GetViewProjection();

	private:
		UniformRingBuffer* m_UniformRingBuffer = nullptr;
		Scene* m_EditorScene = nullptr;
		LightData m_LightData{};
		CameraData m_CameraData{};

		Handle<RenderPass> m_RenderPass;
		Handle<FrameBuffer> m_FrameBuffer;
	};
}