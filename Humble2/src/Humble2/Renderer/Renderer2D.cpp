#include "Renderer2D.h"

namespace HBL
{
	void Renderer2D::Initialize(GraphicsAPI api)
	{
		m_API = api;

		RenderCommand::Initialize(api);

		m_VertexArray = VertexArray::Create();
		m_VertexArray->SetIndexBuffer(IndexBuffer::Create(30000));

		Texture::Load("")->Bind();

		// TODO: Remove from here. ------------------------------------------------------------------
		Texture::Load("res/textures/Pixel-Art.png")->Bind();

#ifdef EMSCRIPTEN
		Shader::Create("Basic", "res/shaders/shaderES.vert", "res/shaders/shaderES.frag");
#else
		Shader::Create("Basic", "res/shaders/shader.vert", "res/shaders/shader.frag");
#endif
		glm::mat4 mvp = glm::ortho(0.f, 960.f, 0.f, 540.f, -1.f, 1.f);

		AddBatch("Basic", 10000, mvp);
		AddBatch("Basic", 10000, mvp);
		AddBatch("Basic", 30000, mvp);
		// -------------------------------------------------------------------------------------------

		m_QuadVertexPosition[0] = { -0.5f, 0.5f, 0.0f, 1.0f };
		m_QuadVertexPosition[1] = { 0.5f,  0.5f, 0.0f, 1.0f };
		m_QuadVertexPosition[2] = { 0.5f, -0.5f, 0.0f, 1.0f };
		m_QuadVertexPosition[3] = { -0.5f,-0.5f, 0.0f, 1.0f };
	}

	void Renderer2D::BeginFrame()
	{
		RenderCommand::ClearScreen({ 1.f, 0.7f, 0.3f, 1.0f });
	}

	void Renderer2D::Submit()
	{
		Texture::ForEach([](Texture* texture)
		{
			texture->Bind();
		});

		m_VertexArray->Bind();

		for (auto& buffer : m_VertexArray->GetVertexBuffers())
		{
			buffer->Bind();
			buffer->SetData();

			m_VertexArray->GetIndexBuffer()->Bind();
			m_VertexArray->GetIndexBuffer()->SetData(buffer->BatchSize);

			Shader::Get(m_Shaders[buffer->BatchIndex])->Bind();

			RenderCommand::Submit(buffer);

			buffer->UnBind();
			m_VertexArray->GetIndexBuffer()->UnBind();
		}

		m_VertexArray->UnBind();
	}

	void Renderer2D::EndFrame()
	{
		for (auto& buffer : m_VertexArray->GetVertexBuffers())
		{
			buffer->BatchSize = 0;
		}
	}

	uint32_t Renderer2D::AddBatch(const std::string& shaderName, uint32_t vertexBufferSize, glm::mat4& mvp)
	{
		VertexBufferLayout vertexBufferLayout(
		{
			{ 0, 2, Type::FLOAT, false },
			{ 1, 4, Type::FLOAT, false },
			{ 2, 2, Type::FLOAT, false },
			{ 3, 1, Type::FLOAT, false },
		});

		VertexBuffer* vertexBuffer = VertexBuffer::Create(vertexBufferSize * 4, vertexBufferLayout);

		m_VertexArray->AddVertexBuffer(vertexBuffer);

		Shader::Get(shaderName)->Bind();

#ifdef EMSCRIPTEN
		Shader::Get(shaderName)->SetInt1(0, "u_Textures0");
		Shader::Get(shaderName)->SetInt1(1, "u_Textures1");
		Shader::Get(shaderName)->SetInt1(2, "u_Textures2");
		Shader::Get(shaderName)->SetInt1(3, "u_Textures3");
		Shader::Get(shaderName)->SetInt1(4, "u_Textures4");
		Shader::Get(shaderName)->SetInt1(5, "u_Textures5");
		Shader::Get(shaderName)->SetInt1(6, "u_Textures6");
		Shader::Get(shaderName)->SetInt1(7, "u_Textures7");
#else
		int samplers[32];
		for (uint32_t i = 0; i < 32; i++)
			samplers[i] = i;

		Shader::Get(shaderName)->SetIntPtr1(samplers, 32, "u_Textures");
#endif
		Shader::Get(shaderName)->SetMat4(mvp, "u_MVP");

		m_Shaders.push_back(shaderName);

		return m_VertexArray->GetVertexBuffers().size() - 1;
	}

	void Renderer2D::DrawQuad(uint32_t batchIndex, glm::vec3& position, float rotation, glm::vec3& scale, float textureID, glm::vec4 color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(scale.x, scale.y, 1.0f));

		Buffer* buffer = m_VertexArray->GetVertexBuffers()[batchIndex]->GetHandle();
		uint32_t& batchSize = m_VertexArray->GetVertexBuffers()[batchIndex]->BatchSize;

		buffer[batchSize].Position = transform * m_QuadVertexPosition[0];
		buffer[batchSize].Color = color;
		buffer[batchSize].TextureCoord = { 0.0f, 1.0f };
		buffer[batchSize].TextureID = textureID;
		batchSize++;

		buffer[batchSize].Position = transform * m_QuadVertexPosition[1];
		buffer[batchSize].Color = color;
		buffer[batchSize].TextureCoord = { 1.0f, 1.0f };
		buffer[batchSize].TextureID = textureID;
		batchSize++;

		buffer[batchSize].Position = transform * m_QuadVertexPosition[2];
		buffer[batchSize].Color = color;
		buffer[batchSize].TextureCoord = { 1.0f, 0.0f };
		buffer[batchSize].TextureID = textureID;
		batchSize++;

		buffer[batchSize].Position = transform * m_QuadVertexPosition[3];
		buffer[batchSize].Color = color;
		buffer[batchSize].TextureCoord = { 0.0f, 0.0f };
		buffer[batchSize].TextureID = textureID;
		batchSize++;
	}

	void Renderer2D::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
	{
		RenderCommand::SetViewport(x, y, width, height);
	}
}