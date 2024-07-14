#include "OpenGLRenderer.h"

namespace HBL2
{
	void OpenGLRenderer::Initialize()
	{
		m_ResourceManager = (HBL::OpenGLResourceManager*)HBL::ResourceManager::Instance;
	}

	void OpenGLRenderer::BeginFrame()
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRenderer::SetPipeline(HBL::Handle<HBL::Material>& material)
	{
		HBL::OpenGLMaterial* openGLMaterial = m_ResourceManager->GetMaterial(material);
		HBL::OpenGLShader* openGLShader = m_ResourceManager->GetShader(openGLMaterial->Shader);

		glBindVertexArray(openGLShader->RenderPipeline);
	}

	void OpenGLRenderer::SetBuffers(HBL::Handle<HBL::Mesh>& mesh)
	{
		HBL::OpenGLMesh* openGLMesh = m_ResourceManager->GetMesh(mesh);

		for (uint32_t i = 0; i < openGLMesh->VertexBuffers.size(); i++)
		{
			HBL::OpenGLBuffer* openGLBuffer = m_ResourceManager->GetBuffer(openGLMesh->VertexBuffers[i]);

			glBindBuffer(GL_ARRAY_BUFFER, openGLBuffer->RendererId);
			glEnableVertexAttribArray(i);
			glVertexAttribPointer(i, (openGLBuffer->ByteSize / sizeof(float)) / openGLMesh->VertexCount, GL_FLOAT, GL_FALSE, openGLBuffer->ByteSize / openGLMesh->VertexCount, nullptr);
		}
	}

	void OpenGLRenderer::SetBufferData(HBL::Handle<HBL::Mesh>& mesh)
	{
		HBL::OpenGLMesh* openGLMesh = m_ResourceManager->GetMesh(mesh);

		for (uint32_t i = 0; i < openGLMesh->VertexBuffers.size(); i++)
		{
			HBL::OpenGLBuffer* openGLBuffer = m_ResourceManager->GetBuffer(openGLMesh->VertexBuffers[i]);
			void* data = m_ResourceManager->GetBufferData(openGLMesh->VertexBuffers[i]);

			glBindBuffer(GL_ARRAY_BUFFER, openGLBuffer->RendererId);
			glBufferSubData(GL_ARRAY_BUFFER, 0, openGLBuffer->ByteSize * openGLBuffer->RendererId, data);
		}
	}

	void OpenGLRenderer::SetBindGroups(HBL::Handle<HBL::Material>& material)
	{
		HBL::OpenGLMaterial* openGLMaterial = m_ResourceManager->GetMaterial(material);
		HBL::OpenGLShader* openGLShader = m_ResourceManager->GetShader(openGLMaterial->Shader);

		glUseProgram(openGLShader->Program);
	}

	void OpenGLRenderer::Draw(HBL::Handle<HBL::Mesh>& mesh, HBL::Handle<HBL::Material>& material)
	{
		HBL::OpenGLMesh* openGLMesh = m_ResourceManager->GetMesh(mesh);

		glDrawArrays(GL_TRIANGLES, 0, openGLMesh->VertexCount);

		glBindVertexArray(0);
		glUseProgram(0);
	}

	void OpenGLRenderer::DrawIndexed(HBL::Handle<HBL::Mesh>& mesh, HBL::Handle<HBL::Material>& material)
	{
		HBL::OpenGLMesh* openGLMesh = m_ResourceManager->GetMesh(mesh);

		glDrawElements(GL_TRIANGLES, (openGLMesh->VertexCount / 4) * 6, GL_UNSIGNED_INT, nullptr);

		glBindVertexArray(0);
		glUseProgram(0);
	}

	void OpenGLRenderer::EndFrame()
	{
	}

	void OpenGLRenderer::Clean()
	{
	}
}
