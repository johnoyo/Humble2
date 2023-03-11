#pragma once

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
		virtual void Initialize() = 0;
		virtual void DrawQuad() = 0;
		virtual void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) = 0;
	};
}