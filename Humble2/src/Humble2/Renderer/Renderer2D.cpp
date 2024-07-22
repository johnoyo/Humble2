#include "Renderer2D.h"

namespace HBL2
{
	void Renderer2D::Initialize()
	{
		m_VertexArray = VertexArray::Create();
		m_VertexArray->SetIndexBuffer(IndexBuffer::Create(MAX_BATCH_SIZE));

		HBL::Texture::Load("")->Bind();

		m_QuadVertexPosition[0] = { -0.5f, 0.5f, 0.0f, 1.0f };
		m_QuadVertexPosition[1] = { 0.5f,  0.5f, 0.0f, 1.0f };
		m_QuadVertexPosition[2] = { 0.5f, -0.5f, 0.0f, 1.0f };
		m_QuadVertexPosition[3] = { -0.5f,-0.5f, 0.0f, 1.0f };

		m_QuadTextureCoordinates[0] = { 0.0f, 1.0f };
		m_QuadTextureCoordinates[1] = { 1.0f, 1.0f };
		m_QuadTextureCoordinates[2] = { 1.0f, 0.0f };
		m_QuadTextureCoordinates[3] = { 0.0f, 0.0f };

		m_FrameBuffer = RenderCommand::FrameBuffer;
	}

	void Renderer2D::BeginFrame()
	{
		if (m_FrameBuffer != nullptr)
		{
			m_FrameBuffer->Bind();
		}
	}

	void Renderer2D::Submit()
	{
		glm::mat4 mvp = glm::mat4(0.f);

		if (Context::Mode == Mode::Runtime)
		{
			if (Context::ActiveScene->MainCamera != entt::null)
			{
				mvp = Context::ActiveScene->GetComponent<Component::Camera>(Context::ActiveScene->MainCamera).ViewProjectionMatrix;
			}
			else
			{
				HBL2_CORE_WARN("No main camera set for runtime context.");
			}
		}
		else if (Context::Mode == Mode::Editor)
		{
			if (Context::Core->MainCamera != entt::null)
			{
				mvp = Context::Core->GetComponent<Component::Camera>(Context::Core->MainCamera).ViewProjectionMatrix;
			}
			else
			{
				HBL2_CORE_WARN("No main camera set for editor context.");
			}
		}
		else
		{
			HBL2_CORE_WARN("No mode set for current context.");
		}

		HBL::Texture::ForEach([](HBL::Texture* texture)
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

			HBL::Shader::Get(m_Shaders[buffer->BatchIndex])->Bind();

			HBL::Shader::Get(m_Shaders[buffer->BatchIndex])->SetMat4(mvp, "u_MVP");

			RenderCommand::DrawIndexed(buffer);

			HBL::Shader::Get(m_Shaders[buffer->BatchIndex])->UnBind();

			m_VertexArray->GetIndexBuffer()->UnBind();

			buffer->UnBind();
		}

		m_VertexArray->UnBind();
	}

	void Renderer2D::EndFrame()
	{
		for (auto& buffer : m_VertexArray->GetVertexBuffers())
		{
			buffer->BatchSize = 0;
		}

		if (m_FrameBuffer != nullptr)
		{
			m_FrameBuffer->UnBind();
		}
	}

	uint32_t Renderer2D::AddBatch(const std::string& shaderName, uint32_t vertexBufferSize, glm::mat4& mvp)
	{
		assert(vertexBufferSize <= MAX_BATCH_SIZE);

		VertexBufferLayout vertexBufferLayout(
		{
			{ 0, 3, Type::FLOAT, false },
			{ 1, 4, Type::FLOAT, false },
			{ 2, 2, Type::FLOAT, false },
			{ 3, 1, Type::FLOAT, false },
			{ 4, 3, Type::FLOAT, false },
		});

		VertexBuffer* vertexBuffer = VertexBuffer::Create(vertexBufferSize * 4, vertexBufferLayout);

		m_VertexArray->AddVertexBuffer(vertexBuffer);

		HBL::Shader::Get(shaderName)->Bind();

#ifdef EMSCRIPTEN
		HBL::Shader::Get(shaderName)->SetInt1(0, "u_Textures0");
		HBL::Shader::Get(shaderName)->SetInt1(1, "u_Textures1");
		HBL::Shader::Get(shaderName)->SetInt1(2, "u_Textures2");
		HBL::Shader::Get(shaderName)->SetInt1(3, "u_Textures3");
		HBL::Shader::Get(shaderName)->SetInt1(4, "u_Textures4");
		HBL::Shader::Get(shaderName)->SetInt1(5, "u_Textures5");
		HBL::Shader::Get(shaderName)->SetInt1(6, "u_Textures6");
		HBL::Shader::Get(shaderName)->SetInt1(7, "u_Textures7");
		HBL::Shader::Get(shaderName)->SetInt1(8, "u_Textures8");
		HBL::Shader::Get(shaderName)->SetInt1(9, "u_Textures9");
		HBL::Shader::Get(shaderName)->SetInt1(10, "u_Textures10");
		HBL::Shader::Get(shaderName)->SetInt1(11, "u_Textures11");
		HBL::Shader::Get(shaderName)->SetInt1(12, "u_Textures12");
		HBL::Shader::Get(shaderName)->SetInt1(13, "u_Textures13");
		HBL::Shader::Get(shaderName)->SetInt1(14, "u_Textures14");
		HBL::Shader::Get(shaderName)->SetInt1(15, "u_Textures15");
#else
		int samplers[32];
		for (uint32_t i = 0; i < 32; i++)
			samplers[i] = i;

		HBL::Shader::Get(shaderName)->SetIntPtr1(samplers, 32, "u_Textures");
#endif
		HBL::Shader::Get(shaderName)->SetMat4(mvp, "u_MVP");

		m_Shaders.push_back(shaderName);

		m_VertexArray->GetIndexBuffer()->Invalidate(MAX_BATCH_SIZE * 4 * m_VertexArray->GetVertexBuffers().size());

		return m_VertexArray->GetVertexBuffers().size() - 1;
	}

	void Renderer2D::DrawQuad(uint32_t batchIndex, Component::Transform& transform, float textureID, glm::vec4 color)
	{
		HBL::Buffer * buffer = m_VertexArray->GetVertexBuffers()[batchIndex]->GetHandle();
		uint32_t& batchSize = m_VertexArray->GetVertexBuffers()[batchIndex]->BatchSize;

		buffer[batchSize].Position = transform.Matrix * m_QuadVertexPosition[0];
		buffer[batchSize].Color = color;
		buffer[batchSize].TextureCoord = m_QuadTextureCoordinates[0];
		buffer[batchSize].TextureID = textureID;
		batchSize++;

		buffer[batchSize].Position = transform.Matrix * m_QuadVertexPosition[1];
		buffer[batchSize].Color = color;
		buffer[batchSize].TextureCoord = m_QuadTextureCoordinates[1];
		buffer[batchSize].TextureID = textureID;
		batchSize++;

		buffer[batchSize].Position = transform.Matrix * m_QuadVertexPosition[2];
		buffer[batchSize].Color = color;
		buffer[batchSize].TextureCoord = m_QuadTextureCoordinates[2];
		buffer[batchSize].TextureID = textureID;
		batchSize++;

		buffer[batchSize].Position = transform.Matrix * m_QuadVertexPosition[3];
		buffer[batchSize].Color = color;
		buffer[batchSize].TextureCoord = m_QuadTextureCoordinates[3];
		buffer[batchSize].TextureID = textureID;
		batchSize++;
	}

	void Renderer2D::DrawQuad(uint32_t batchIndex, glm::vec3& position, glm::vec3& scale, float textureID, glm::vec4 color)
	{
		HBL::Buffer* buffer = m_VertexArray->GetVertexBuffers()[batchIndex]->GetHandle();
		uint32_t& batchSize = m_VertexArray->GetVertexBuffers()[batchIndex]->BatchSize;

		buffer[batchSize].Position = { position.x - scale.x / 2.f, position.y + scale.y / 2.f, 0.f };
		buffer[batchSize].Color = color;
		buffer[batchSize].TextureCoord = m_QuadTextureCoordinates[0];
		buffer[batchSize].TextureID = textureID;
		batchSize++;

		buffer[batchSize].Position = { position.x + scale.x / 2.f, position.y + scale.y / 2.f, 0.f };
		buffer[batchSize].Color = color;
		buffer[batchSize].TextureCoord = m_QuadTextureCoordinates[1];
		buffer[batchSize].TextureID = textureID;
		batchSize++;

		buffer[batchSize].Position = { position.x + scale.x / 2.f, position.y - scale.y / 2.f, 0.f };
		buffer[batchSize].Color = color;
		buffer[batchSize].TextureCoord = m_QuadTextureCoordinates[2];
		buffer[batchSize].TextureID = textureID;
		batchSize++;

		buffer[batchSize].Position = { position.x - scale.x / 2.f, position.y - scale.y / 2.f, 0.f };
		buffer[batchSize].Color = color;
		buffer[batchSize].TextureCoord = m_QuadTextureCoordinates[3];
		buffer[batchSize].TextureID = textureID;
		batchSize++;
	}

	void Renderer2D::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
	{
		RenderCommand::SetViewport(x, y, width, height);
	}

	void Renderer2D::Clean()
	{
		for (VertexBuffer* vb : m_VertexArray->GetVertexBuffers())
		{
			delete vb;
		}

		IndexBuffer* ib = m_VertexArray->GetIndexBuffer();
		delete ib;

		delete m_VertexArray;
	}
}