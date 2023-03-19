#pragma once

#include "Shader.h"
#include "Texture.h"
#include "VertexArray.h"
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

		void BeginFrame();
		void Submit();
		void EndFrame();

		uint32_t AddBatch(const std::string& shaderName, uint32_t vertexBufferSize, glm::mat4& mvp);

		void DrawQuad(uint32_t batchIndex, glm::vec3& position, glm::vec3& scale, float textureID = 0.f, glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });
		void DrawQuad(uint32_t batchIndex, glm::vec3& position, float rotation, glm::vec3& scale, float textureID = 0.f, glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });

		void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);

		inline GraphicsAPI GetAPI() { return m_API; }
	private:
		Renderer2D() {}
		GraphicsAPI m_API = GraphicsAPI::None;
		VertexArray* m_VertexArray = nullptr;
		glm::vec4 m_QuadVertexPosition[4] = {};
		std::vector<std::string> m_Shaders;
	};
}