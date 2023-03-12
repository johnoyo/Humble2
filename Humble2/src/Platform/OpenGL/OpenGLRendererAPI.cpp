#include "OpenGLRendererAPI.h"

namespace HBL
{
	OpenGLRendererAPI::OpenGLRendererAPI()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);

		m_VertexArray = (OpenGLVertexArray*)VertexArray::Create();
		m_VertexBuffer = (OpenGLVertexBuffer*)VertexBuffer::Create(1000);
		m_IndexBuffer = (OpenGLIndexBuffer*)IndexBuffer::Create(1000);

		m_VertexArray->AddVertexBuffer(m_VertexBuffer);
		m_VertexArray->SetIndexBuffer(m_IndexBuffer);
		
		Texture::Load("")->Bind();
		Texture::Load("res/textures/Pixel-Art.png")->Bind();
		
#ifdef EMSCRIPTEN
		Shader::Create("Basic", "res/shaders/shaderES.vert", "res/shaders/shaderES.frag")->Bind();
#else
		Shader::Create("Basic", "res/shaders/shader.vert", "res/shaders/shader.frag")->Bind();
#endif
		
#ifdef EMSCRIPTEN
		Shader::Get("Basic")->SetInt1(0, "u_Textures0");
		Shader::Get("Basic")->SetInt1(1, "u_Textures1");
		Shader::Get("Basic")->SetInt1(2, "u_Textures2");
		Shader::Get("Basic")->SetInt1(3, "u_Textures3");
		Shader::Get("Basic")->SetInt1(4, "u_Textures4");
		Shader::Get("Basic")->SetInt1(5, "u_Textures5");
		Shader::Get("Basic")->SetInt1(6, "u_Textures6");
		Shader::Get("Basic")->SetInt1(7, "u_Textures7");
#else
		int samplers[32];
		for (uint32_t i = 0; i < 32; i++)
			samplers[i] = i;

		Shader::Get("Basic")->SetIntPtr1(samplers, 32, "u_Textures");
#endif
		glm::mat4 mvp = glm::ortho(0.f, 960.f, 0.f, 540.f, -1.f, 1.f);

		Shader::Get("Basic")->SetMat4(mvp, "u_MVP");
	}

	void OpenGLRendererAPI::DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale)
	{
		glm::vec4 color = { 1.f, 1.f, 1.f, 1.f };
		m_VertexBuffer->AppendQuad(position, rotation, scale, color, 0.f);
	}

	void OpenGLRendererAPI::DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale, glm::vec4& color)
	{
		m_VertexBuffer->AppendQuad(position, rotation, scale, color, 0.f);
	}

	void OpenGLRendererAPI::DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale, uint32_t textureID, glm::vec4 color)
	{
		m_VertexBuffer->AppendQuad(position, rotation, scale, color, textureID);
	}

	void OpenGLRendererAPI::SetViewport(GLint x, GLint y, GLsizei width, GLsizei height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::ClearScreen(glm::vec4 color)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void OpenGLRendererAPI::Flush()
	{
		Texture::ForEach([](Texture* texture)
		{
			texture->Bind();
		});

		m_VertexBuffer->Bind();
		m_VertexBuffer->SetData();

		m_IndexBuffer->Bind();
		m_IndexBuffer->SetData(m_VertexBuffer->GetBatchSize());

		glDrawElements(GL_TRIANGLES, (m_VertexBuffer->GetBatchSize() / 4) * 6, GL_UNSIGNED_INT, NULL);

		m_VertexBuffer->Reset();
	}
}


