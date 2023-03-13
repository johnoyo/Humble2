#pragma once

#include "RendererAPI.h"
#include "../Utilities/Log.h"

namespace HBL
{
	class RenderCommand
	{
	public:
		static void Initialize(GraphicsAPI api);

		static void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
		static void ClearScreen(glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });
		static void Submit(VertexBuffer* vertexBuffer);
	private:
		static RendererAPI* s_RendererAPI;
	};
}