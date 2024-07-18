#pragma once

#include "Shader.h"
#include "Texture.h"
#include "VertexArray.h"
#include "FrameBuffer.h"
#include "RenderCommand.h"

#include "Core/Context.h"

#define MAX_BATCH_SIZE 50000

namespace HBL2
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

		void Initialize();

		void BeginFrame();
		void Submit();
		void EndFrame();

		void Clean();

		uint32_t AddBatch(const std::string& shaderName, uint32_t vertexBufferSize, glm::mat4& mvp);

		void DrawQuad(uint32_t batchIndex, glm::vec3& position, glm::vec3& scale, float textureID = 0.f, glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });
		void DrawQuad(uint32_t batchIndex, Component::Transform& transform, float textureID = 0.f, glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f });

		void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);

	private:
		Renderer2D() {}

		HBL::FrameBuffer* m_FrameBuffer = nullptr;
		VertexArray* m_VertexArray = nullptr;
		glm::vec4 m_QuadVertexPosition[4] = {};
		glm::vec2 m_QuadTextureCoordinates[4] = {};
		std::vector<std::string> m_Shaders;
	};
}