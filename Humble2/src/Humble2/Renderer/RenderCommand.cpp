#include "RenderCommand.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace HBL2
{
	RendererAPI* RenderCommand::s_RendererAPI = nullptr;
	HBL::FrameBuffer* RenderCommand::FrameBuffer = nullptr;
	GraphicsAPI RenderCommand::s_API = GraphicsAPI::None;

	void RenderCommand::Initialize(GraphicsAPI api)
	{
		s_API = api;

		switch (s_API)
		{
		case HBL2::GraphicsAPI::OpenGL:
			s_RendererAPI = new OpenGLRendererAPI();
			break;
		case HBL2::GraphicsAPI::Vulkan:
			HBL2_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			s_RendererAPI = new OpenGLRendererAPI();
			break;
		case HBL2::GraphicsAPI::None:
			HBL2_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			break;
		}
	}

	void RenderCommand::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
	{
		s_RendererAPI->SetViewport(x, y, width, height);
	}

	void RenderCommand::ClearScreen(glm::vec4 color)
	{
		s_RendererAPI->ClearScreen(color);
	}

	void RenderCommand::Draw(VertexBuffer* vertexBuffer)
	{
		s_RendererAPI->Draw(vertexBuffer);
	}

	void RenderCommand::DrawIndexed(VertexBuffer* vertexBuffer)
	{
		s_RendererAPI->DrawIndexed(vertexBuffer);
	}

	GraphicsAPI RenderCommand::GetAPI()
	{
		return s_API;
	}
}