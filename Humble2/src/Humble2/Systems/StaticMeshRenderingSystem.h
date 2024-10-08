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

	struct CameraData
	{
		glm::mat4 ViewProjection;
	};

	struct LightData
	{
		glm::vec4 ViewPosition;
		glm::vec4 LightPositions[16];
		glm::vec4 LightColors[16];
		glm::vec4 LightIntensities[16];
		float LightCount;
		float _padding[3];
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
		Handle<BindGroup> m_GlobalBindings;
		Handle<BindGroupLayout> m_GlobalBindGroupLayout;
		Handle<Buffer> m_CameraBuffer;
		Handle<Buffer> m_LightBuffer;
		UniformRingBuffer* m_UniformRingBuffer = nullptr;
		Scene* m_EditorScene = nullptr;
		LightData m_LightData{};
		CameraData m_CameraData{};
	};
}