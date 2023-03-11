#pragma once

#include "RenderCommand.h"

namespace HBL
{
	class Renderer2D
	{
	public:
		Renderer2D(const Renderer2D&) = delete;

		static Renderer2D& Get()
		{
			static Renderer2D instance;
			return instance;
		}

		void Initialize(GraphicsAPI api);
		void DrawQuad();
		void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);

		inline GraphicsAPI GetAPI() { return m_API; }

	private:
		Renderer2D() {}
		GraphicsAPI m_API = GraphicsAPI::None;
	};
}