#include "Renderer2D.h"

namespace HBL
{
	void Renderer2D::Initialize(GraphicsAPI api)
	{
		m_API = api;
		RenderCommand::Initialize(api);
	}

	void Renderer2D::BeginFrame()
	{
		RenderCommand::ClearScreen({ 1.f, 0.7f, 0.3f, 1.0f });
	}

	void Renderer2D::Render()
	{
	}

	void Renderer2D::EndFrame()
	{
		RenderCommand::Flush();
	}

	void Renderer2D::DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale)
	{
		RenderCommand::DrawQuad(position, rotation, scale);
	}

	void Renderer2D::DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale, glm::vec4& color)
	{
		RenderCommand::DrawQuad(position, rotation, scale, color);
	}

	void Renderer2D::DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale, uint32_t textureID, glm::vec4 color)
	{
		RenderCommand::DrawQuad(position, rotation, scale, textureID, color);
	}

	void Renderer2D::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
	{
		RenderCommand::SetViewport(x, y, width, height);
	}
}