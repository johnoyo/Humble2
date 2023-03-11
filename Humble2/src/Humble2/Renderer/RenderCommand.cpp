#include "RenderCommand.h"

namespace HBL
{
	RendererAPI* RenderCommand::s_RendererAPI = nullptr;

	void RenderCommand::Initialize(GraphicsAPI api)
	{
		switch (api)
		{
		case HBL::GraphicsAPI::OpenGL:
			s_RendererAPI = new OpenGLRendererAPI;
			break;
		case HBL::GraphicsAPI::Vulkan:
			HBL_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			s_RendererAPI = new OpenGLRendererAPI;
			break;
		case HBL::GraphicsAPI::None:
			HBL_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			break;
		}

		s_RendererAPI->Initialize();
	}

	void RenderCommand::DrawQuad()
	{
		s_RendererAPI->DrawQuad();
	}

	void RenderCommand::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
	{
		s_RendererAPI->SetViewport(x, y, width, height);
	}
}