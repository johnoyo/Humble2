#include "RenderCommand.h"

#include "../../Platform/OpenGL/OpenGLRendererAPI.h"

namespace HBL
{
	RendererAPI* RenderCommand::s_RendererAPI = nullptr;

	void RenderCommand::Initialize(GraphicsAPI api)
	{
		switch (api)
		{
		case HBL::GraphicsAPI::OpenGL:
			s_RendererAPI = new OpenGLRendererAPI();
			break;
		case HBL::GraphicsAPI::Vulkan:
			HBL_CORE_WARN("Vulkan is not yet supported, falling back to OpenGL.");
			s_RendererAPI = new OpenGLRendererAPI();
			break;
		case HBL::GraphicsAPI::None:
			HBL_CORE_FATAL("No GraphicsAPI specified.");
			exit(-1);
			break;
		}
	}

	void RenderCommand::DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale)
	{
		s_RendererAPI->DrawQuad(position, rotation, scale);
	}

	void RenderCommand::DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale, glm::vec4& color)
	{
		s_RendererAPI->DrawQuad(position, rotation, scale, color);
	}

	void RenderCommand::DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale, uint32_t textureID, glm::vec4 color)
	{
		s_RendererAPI->DrawQuad(position, rotation, scale, textureID, color);
	}

	void RenderCommand::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
	{
		s_RendererAPI->SetViewport(x, y, width, height);
	}

	void RenderCommand::ClearScreen(glm::vec4 color)
	{
		s_RendererAPI->ClearScreen(color);
	}

	void RenderCommand::Flush()
	{
		s_RendererAPI->Flush();
	}
}