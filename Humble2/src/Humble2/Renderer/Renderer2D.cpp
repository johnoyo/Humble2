#include "Renderer2D.h"

namespace HBL
{
	void Renderer2D::Initialize(GraphicsAPI api)
	{
		m_API = api;
		RenderCommand::Initialize(api);
	}

	void Renderer2D::DrawQuad()
	{
		RenderCommand::DrawQuad();
	}

	void Renderer2D::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
	{
		RenderCommand::SetViewport(x, y, width, height);
	}
}