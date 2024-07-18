#pragma once

#include "RendererAPI.h"
#include "Utilities/Log.h"
#include "FrameBuffer.h"

namespace HBL2
{
	class RenderCommand
	{
	public:
		static void Initialize(GraphicsAPI api);

		static void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
		static void ClearScreen(glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });
		static void Draw(VertexBuffer* vertexBuffer);
		static void DrawIndexed(VertexBuffer* vertexBuffer);

		static HBL::FrameBuffer* FrameBuffer;

		static GraphicsAPI GetAPI();

	private:
		static RendererAPI* s_RendererAPI;
		static GraphicsAPI s_API;
	};
}