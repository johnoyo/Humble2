#pragma once

#include <glm/glm.hpp>
#include <stdint.h>

namespace HBL
{
	enum class GraphicsAPI
	{
		OpenGL,
		Vulkan,
		None
	};

	class RendererAPI
	{
	public:
		virtual ~RendererAPI() = default;

		virtual void DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale) = 0;
		virtual void DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale, glm::vec4& color) = 0;
		virtual void DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale, uint32_t textureID, glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f}) = 0;
		virtual void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) = 0;
		virtual void ClearScreen(glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }) = 0;
		virtual void Flush() = 0;
	};
}