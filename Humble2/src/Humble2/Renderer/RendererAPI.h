#pragma once

#include "VertexBuffer.h"

#include <glm/glm.hpp>
#include <stdint.h>

namespace HBL2
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

		virtual void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) = 0;
		virtual void ClearScreen(glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }) = 0;
		virtual void Submit(VertexBuffer* vertexBuffer) = 0;
	};
}