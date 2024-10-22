#include "OpenGLRenderPassRenderer.h"

namespace HBL2
{
	void OpenGLRenderPasRenderer::DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws)
	{
		OpenGLResourceManager* rm = (OpenGLResourceManager*)ResourceManager::Instance;

		if (globalDraw.BindGroup.IsValid())
		{
			OpenGLBindGroup* globalBindGroup = rm->GetBindGroup(globalDraw.BindGroup);
			OpenGLBindGroupLayout* globalBindGroupLayout = rm->GetBindGroupLayout(globalBindGroup->BindGroupLayout);

			int index = 0;

			// Write the global uniform buffers
			for (const auto& bufferEntry : globalBindGroup->Buffers)
			{
				OpenGLBuffer* buffer = rm->GetBuffer(bufferEntry.buffer);
				glBindBufferBase(GL_UNIFORM_BUFFER, globalBindGroupLayout->BufferBindings[index++].slot, buffer->RendererId);
				buffer->Write();
			}
		}

		for (auto&& [shaderID, drawList] : draws.GetDraws())
		{
			auto& localDraw = drawList[0];

			// Write the entire dynamicUniformBuffer data 
			OpenGLBindGroup* localDrawBindGroup0 = rm->GetBindGroup(localDraw.BindGroup);
			OpenGLBuffer* dynamicUniformBuffer = rm->GetBuffer(localDrawBindGroup0->Buffers[0].buffer);
			dynamicUniformBuffer->Write();

			// Bind Vertex Array
			OpenGLShader* shader = rm->GetShader(localDraw.Shader);
			shader->BindPipeline();

			for (auto& draw : drawList)
			{
				Mesh* mesh = rm->GetMesh(draw.Mesh);
				Material* material = rm->GetMaterial(draw.Material);
				OpenGLBindGroup* drawBindGroup = rm->GetBindGroup(draw.BindGroup);

				// Bind Index buffer if applicable
				if (mesh->IndexBuffer.IsValid())
				{
					OpenGLBuffer* indexBuffer = rm->GetBuffer(mesh->IndexBuffer);
					indexBuffer->Bind();
				}

				// Bind vertex buffers
				for (int i = 0; i < mesh->VertexBuffers.size(); i++)
				{
					OpenGLBuffer* vertexBuffer = rm->GetBuffer(mesh->VertexBuffers[i]);
					vertexBuffer->Bind(draw.Material, i);
				}

				// Bind shader
				shader->Bind();

				// Set bind group
				drawBindGroup->Set();

				// Bind dynamic uniform buffer with the current offset and size
				dynamicUniformBuffer->Bind(draw.Material, 0, draw.Offset, draw.Size);

				// Draw the mesh accordingly
				if (mesh->IndexBuffer.IsValid())
				{
					Renderer::Instance->DrawIndexed(draw.Mesh);
				}
				else
				{
					Renderer::Instance->Draw(draw.Mesh);
				}
			}
		}
	}
}
