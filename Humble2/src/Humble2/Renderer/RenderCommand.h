#pragma once

#include "RendererAPI.h"
#include "../Utilities/Log.h"

namespace HBL
{
	class RenderCommand
	{
	public:
		static void Initialize(GraphicsAPI api);

		static void DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale);
		static void DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale, glm::vec4& color);
		static void DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale, uint32_t textureID, glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });

		static void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
		static void ClearScreen(glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });

		static void Flush();
	private:
		static RendererAPI* s_RendererAPI;
	};
}