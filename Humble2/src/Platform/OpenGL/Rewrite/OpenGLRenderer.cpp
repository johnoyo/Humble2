#include "OpenGLRenderer.h"

namespace HBL2
{
	void OpenGLRenderer::Initialize()
	{
		m_GraphicsAPI = GraphicsAPI::OPENGL;
		m_ResourceManager = (OpenGLResourceManager*)ResourceManager::Instance;

		/*
		TODO: Get 256 from here in a API agnostic way:
				int32_t uniformBufferAlignSize;
				glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformBufferAlignSize);
		*/

		TempUniformRingBuffer = new UniformRingBuffer(4096, 256);

#ifdef DEBUG
		GLDebug::EnableGLDebugging();
#endif
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		m_MainCommandBuffer = new CommandBuffer();
		m_SecondaryCommandBuffer = new CommandBuffer();
	}

	void OpenGLRenderer::BeginFrame()
	{
		if (FrameBufferHandle.IsValid())
		{
			OpenGLFrameBuffer* openGLFrameBuffer = m_ResourceManager->GetFrameBuffer(FrameBufferHandle);

			glBindFramebuffer(GL_FRAMEBUFFER, openGLFrameBuffer->RendererId);
			glViewport(0, 0, openGLFrameBuffer->Width, openGLFrameBuffer->Height);
		}

		glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRenderer::SetPipeline(Handle<Shader> shader)
	{
		OpenGLShader* openGLShader = m_ResourceManager->GetShader(shader);
		glBindVertexArray(openGLShader->RenderPipeline);
	}

	void OpenGLRenderer::SetBuffers(Handle<Mesh> mesh, Handle<Material> material)
	{
		Mesh* openGLMesh = m_ResourceManager->GetMesh(mesh);
		Material* openGLMaterial = m_ResourceManager->GetMaterial(material);

		HBL2_CORE_ASSERT(openGLMesh != nullptr, "Mesh handle is invalid.");
		HBL2_CORE_ASSERT(openGLMaterial != nullptr, "Material handle is invalid.");

		if (openGLMesh->IndexBuffer.IsValid())
		{
			OpenGLBuffer* openGLIndexBuffer = m_ResourceManager->GetBuffer(openGLMesh->IndexBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, openGLIndexBuffer->RendererId);
		}

		for (uint32_t i = 0; i < openGLMesh->VertexBuffers.size(); i++)
		{
			if (openGLMesh->VertexBuffers[i].IsValid())
			{
				OpenGLShader* openGLShader = m_ResourceManager->GetShader(openGLMaterial->Shader);
				OpenGLBuffer* openGLBuffer = m_ResourceManager->GetBuffer(openGLMesh->VertexBuffers[i]);

				HBL2_CORE_ASSERT(openGLShader != nullptr, "Shader handle for material \"" + std::string(openGLMaterial->DebugName) + "\" is invalid.");
				HBL2_CORE_ASSERT(openGLBuffer != nullptr, "Buffer handle for material \"" + std::string(openGLMaterial->DebugName) + "\" is invalid.", openGLMaterial->DebugName);
				HBL2_CORE_ASSERT(openGLShader->VertexBufferBindings.size() >= openGLMesh->VertexBuffers.size(), "Shader: \"" + std::string(openGLShader->DebugName) + "\", Number of vertex buffers and number of buffer bindings dot not match.");

				const auto& binding = openGLShader->VertexBufferBindings[i];

				for (int j = 0; j < binding.attributes.size(); j++)
				{
					auto& attribute = binding.attributes[j];

					GLint size = 0;
					GLenum type = 0;
					GLsizei stride = binding.byteStride;
					GLsizei offset = attribute.byteOffset;

					switch (attribute.format)
					{
					case VertexFormat::FLOAT32:
						size = 1;
						type = GL_FLOAT;
						break;
					case VertexFormat::FLOAT32x2:
						size = 2;
						type = GL_FLOAT;
						break;
					case VertexFormat::FLOAT32x3:
						size = 3;
						type = GL_FLOAT;
						break;
					case VertexFormat::FLOAT32x4:
						size = 4;
						type = GL_FLOAT;
					case VertexFormat::UINT32:
						size = 1;
						type = GL_UNSIGNED_INT;
						break;
					case VertexFormat::UINT32x2:
						size = 2;
						type = GL_UNSIGNED_INT;
						break;
					case VertexFormat::UINT32x3:
						size = 3;
						type = GL_UNSIGNED_INT;
						break;
					case VertexFormat::UINT32x4:
						size = 4;
						type = GL_UNSIGNED_INT;
						break;
					case VertexFormat::INT32:
						size = 1;
						type = GL_INT;
						break;
					case VertexFormat::INT32x2:
						size = 2;
						type = GL_INT;
						break;
					case VertexFormat::INT32x3:
						size = 3;
						type = GL_INT;
						break;
					case VertexFormat::INT32x4:
						size = 4;
						type = GL_INT;
						break;
					}

					glBindBuffer(GL_ARRAY_BUFFER, openGLBuffer->RendererId);
					glEnableVertexAttribArray(i + j);
					glVertexAttribPointer(i + j, size, type, GL_FALSE, stride, (const void*)offset);
				}
			}
		}
	}

	void OpenGLRenderer::WriteBuffer(Handle<Buffer> buffer, intptr_t offset)
	{
		OpenGLBuffer* openGLBuffer = m_ResourceManager->GetBuffer(buffer);
		void* data = m_ResourceManager->GetBufferData(buffer);

		if (openGLBuffer->UsageHint == GL_STATIC_DRAW)
		{
			HBL2_CORE_WARN("Buffer with name: {0} is set for static usage, updating will have no effect", openGLBuffer->DebugName);
			return;
		}

		glBindBuffer(openGLBuffer->Usage, openGLBuffer->RendererId);
		glBufferSubData(openGLBuffer->Usage, 0, openGLBuffer->ByteSize, data);
	}

	void OpenGLRenderer::WriteBuffer(Handle<BindGroup> bindGroup, uint32_t bufferIndex)
	{
		OpenGLBindGroup* openGLBindGroup = m_ResourceManager->GetBindGroup(bindGroup);
		WriteBuffer(openGLBindGroup->Buffers[bufferIndex].buffer, openGLBindGroup->Buffers[bufferIndex].byteOffset);
	}

	void OpenGLRenderer::WriteBuffer(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset)
	{
		OpenGLBindGroup* openGLBindGroup = m_ResourceManager->GetBindGroup(bindGroup);
		WriteBuffer(openGLBindGroup->Buffers[bufferIndex].buffer, offset);
	}

	void OpenGLRenderer::SetBindGroups(Handle<Material> material)
	{
		Material* openGLMaterial = m_ResourceManager->GetMaterial(material);
		OpenGLShader* openGLShader = m_ResourceManager->GetShader(openGLMaterial->Shader);
		OpenGLBindGroup* openGLBindGroup = m_ResourceManager->GetBindGroup(openGLMaterial->BindGroup);

		glUseProgram(openGLShader->Program);

		for (int i = 0; i < openGLBindGroup->Textures.size(); i++)
		{
			OpenGLTexture* openGLTexture = m_ResourceManager->GetTexture(openGLBindGroup->Textures[i]);

			glActiveTexture(i + GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, openGLTexture->RendererId);

			int uniform_location = glGetUniformLocation(openGLShader->Program, "u_AlbedoMap");
			if (uniform_location != -1)
			{
				glUniform1i(uniform_location, i);
			}
		}
	}

	void OpenGLRenderer::SetBindGroup(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset)
	{
		OpenGLBindGroup* openGLBindGroup = m_ResourceManager->GetBindGroup(bindGroup);
		OpenGLBindGroupLayout* bindGroupLayout = m_ResourceManager->GetBindGroupLayout(openGLBindGroup->BindGroupLayout);

		if (bindGroupLayout->BufferBindings[bufferIndex].type == BufferBindingType::UNIFORM_DYNAMIC_OFFSET)
		{
			OpenGLBuffer* openGLBuffer = m_ResourceManager->GetBuffer(openGLBindGroup->Buffers[bufferIndex].buffer);
			glBindBufferRange(GL_UNIFORM_BUFFER, bindGroupLayout->BufferBindings[bufferIndex].slot, openGLBuffer->RendererId, offset, 80);
		}
	}

	void OpenGLRenderer::SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData)
	{
		OpenGLBuffer* openGLBuffer = m_ResourceManager->GetBuffer(buffer);
		openGLBuffer->Data = newData;
	}

	void OpenGLRenderer::SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData)
	{
		OpenGLBindGroup* openGLBindGroup = m_ResourceManager->GetBindGroup(bindGroup);
		SetBufferData(openGLBindGroup->Buffers[bufferIndex].buffer, openGLBindGroup->Buffers[bufferIndex].byteOffset, newData);
	}

	void OpenGLRenderer::Draw(Handle<Mesh> mesh)
	{
		Mesh* openGLMesh = m_ResourceManager->GetMesh(mesh);
		glDrawArrays(GL_TRIANGLES, openGLMesh->VertexOffset, openGLMesh->VertexCount);
	}

	void OpenGLRenderer::DrawIndexed(Handle<Mesh> mesh)
	{
		Mesh* openGLMesh = m_ResourceManager->GetMesh(mesh);
		glDrawElements(GL_TRIANGLES, (openGLMesh->IndexCount - openGLMesh->IndexOffset), GL_UNSIGNED_INT, nullptr);
	}

	CommandBuffer* OpenGLRenderer::BeginCommandRecording(CommandBufferType type)
	{
		switch (type)
		{
		case HBL2::CommandBufferType::MAIN:
			return m_MainCommandBuffer;
		case HBL2::CommandBufferType::OFFSCREEN:
			return m_SecondaryCommandBuffer;
		default:
			assert(false);
			return nullptr;
		}
	}

	void OpenGLRenderer::EndFrame()
	{
		if (FrameBufferHandle.IsValid())
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	void OpenGLRenderer::Clean()
	{
	}

	void OpenGLRenderer::ResizeFrameBuffer(uint32_t width, uint32_t height)
	{
		if (FrameBufferHandle.IsValid())
		{
			OpenGLFrameBuffer* openGLFrameBuffer = m_ResourceManager->GetFrameBuffer(FrameBufferHandle);
			openGLFrameBuffer->Resize(width, height);
		}
	}

	void* OpenGLRenderer::GetDepthAttachment()
	{
		if (FrameBufferHandle.IsValid())
		{
			OpenGLFrameBuffer* openGLFrameBuffer = m_ResourceManager->GetFrameBuffer(FrameBufferHandle);
			return (void*)(intptr_t)openGLFrameBuffer->DepthAttachmentId;
		}

		return nullptr;
	}

	void* OpenGLRenderer::GetColorAttachment()
	{
		if (FrameBufferHandle.IsValid())
		{
			OpenGLFrameBuffer* openGLFrameBuffer = m_ResourceManager->GetFrameBuffer(FrameBufferHandle);
			return (void*)(intptr_t)openGLFrameBuffer->ColorAttachmentId;
		}

		return nullptr;
	}
}
