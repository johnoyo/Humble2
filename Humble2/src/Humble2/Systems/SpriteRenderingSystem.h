#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

#include "Renderer\Renderer.h"
#include "Resources\ResourceManager.h"

#include "Renderer\UniformRingBuffer.h"

#include "Utilities\ShaderUtilities.h"
#include "Utilities\TextureUtilities.h"

#include "Platform\OpenGL\OpenGLBindGroup.h"
#include "Platform\OpenGL\OpenGLResourceManager.h"

#include <glm\gtx\quaternion.hpp>

namespace HBL2
{
	struct PerDrawDataSprite
	{
		glm::mat4 Model;
		glm::vec4 Color;
	};

	class SpriteRenderingSystem final : public ISystem
	{
	public:
		SpriteRenderingSystem() { Name = "SpriteRenderingSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnDestroy() override;

	private:
		const glm::mat4& GetViewProjection();

	private:
		Handle<BindGroup> m_GlobalBindings;
		Handle<BindGroupLayout> m_GlobalBindGroupLayout;
		Handle<Buffer> m_CameraBuffer;
		Handle<Buffer> m_VertexBuffer;
		Handle<Mesh> m_SpriteMesh;
		UniformRingBuffer* m_UniformRingBuffer = nullptr;
		Scene* m_EditorScene = nullptr;
	};
}