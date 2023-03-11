#pragma once

#include "RendererAPI.h"
#include "../Utilities/Log.h"
#include "../../Platform/OpenGL/OpenGLRendererAPI.h"

namespace HBL
{
	class RenderCommand
	{
	public:
		static void Initialize(GraphicsAPI api);
		static void DrawQuad();
		static void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
	private:
		static RendererAPI* s_RendererAPI;
	};
}