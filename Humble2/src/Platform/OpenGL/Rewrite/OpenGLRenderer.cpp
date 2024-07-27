#include "OpenGLRenderer.h"

namespace HBL2
{
	void OpenGLRenderer::Initialize()
	{
		m_GraphicsAPI = GraphicsAPI::OPENGL;
		m_ResourceManager = (OpenGLResourceManager*)ResourceManager::Instance;

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

	void OpenGLRenderer::SetPipeline(Handle<Material> material)
	{
		Material* openGLMaterial = m_ResourceManager->GetMaterial(material);
		OpenGLShader* openGLShader = m_ResourceManager->GetShader(openGLMaterial->Shader);

		glBindVertexArray(openGLShader->RenderPipeline);
	}

	void OpenGLRenderer::SetBuffers(Handle<Mesh> mesh)
	{
		Mesh* openGLMesh = m_ResourceManager->GetMesh(mesh);

		if (openGLMesh->IndexBuffer.IsValid())
		{
			OpenGLBuffer* openGLIndexBuffer = m_ResourceManager->GetBuffer(openGLMesh->IndexBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, openGLIndexBuffer->RendererId);
		}

		for (uint32_t i = 0; i < openGLMesh->VertexBuffers.size(); i++)
		{
			if (openGLMesh->VertexBuffers[i].IsValid())
			{
				OpenGLBuffer* openGLBuffer = m_ResourceManager->GetBuffer(openGLMesh->VertexBuffers[i]);

				glBindBuffer(GL_ARRAY_BUFFER, openGLBuffer->RendererId);
				glEnableVertexAttribArray(i);
				glVertexAttribPointer(i, (openGLBuffer->ByteSize / sizeof(float)) / openGLMesh->VertexCount, GL_FLOAT, GL_FALSE, openGLBuffer->ByteSize / openGLMesh->VertexCount, nullptr);
			}
		}
	}

	void OpenGLRenderer::WriteBuffer(Handle<Buffer> buffer, void* newData)
	{
		OpenGLBuffer* openGLBuffer = m_ResourceManager->GetBuffer(buffer);
		void* data = m_ResourceManager->GetBufferData(buffer);
		data = newData;

		if (openGLBuffer->UsageHint == GL_STATIC_DRAW)
		{
			HBL2_CORE_WARN("Buffer with name: {0} is set for static usage, updating will have no effect", openGLBuffer->DebugName);
			return;
		}

		glBindBuffer(openGLBuffer->Usage, openGLBuffer->RendererId);
		glBufferSubData(openGLBuffer->Usage, 0, openGLBuffer->ByteSize, data);
	}

	void OpenGLRenderer::WriteBuffer(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData)
	{
		OpenGLBindGroup* openGLBindGroup = m_ResourceManager->GetBindGroup(bindGroup);
		WriteBuffer(openGLBindGroup->Buffers[bufferIndex].buffer, newData);
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

	void OpenGLRenderer::Draw(Handle<Mesh> mesh, Handle<Material> material)
	{
		Mesh* openGLMesh = m_ResourceManager->GetMesh(mesh);

		glDrawArrays(GL_TRIANGLES, 0, openGLMesh->VertexCount);

		glBindVertexArray(0);
		glUseProgram(0);
	}

	void OpenGLRenderer::DrawIndexed(Handle<Mesh> mesh, Handle<Material> material)
	{
		Mesh* openGLMesh = m_ResourceManager->GetMesh(mesh);

		glDrawElements(GL_TRIANGLES, (openGLMesh->VertexCount / 4) * 6, GL_UNSIGNED_INT, nullptr);

		glBindVertexArray(0);
		glUseProgram(0);
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
