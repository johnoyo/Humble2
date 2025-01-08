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

			// Set global bind group
			globalBindGroup->Set();
		}

		for (auto&& [shaderID, drawList] : draws.GetDraws())
		{
			auto& localDraw = drawList[0];

			OpenGLBuffer* dynamicUniformBuffer = nullptr;

			// Write the entire dynamicUniformBuffer data
			if (localDraw.BindGroup.IsValid())
			{
				OpenGLBindGroup* localDrawBindGroup0 = rm->GetBindGroup(localDraw.BindGroup);
				dynamicUniformBuffer = rm->GetBuffer(localDrawBindGroup0->Buffers[0].buffer);
				dynamicUniformBuffer->Write();
			}

			if (localDraw.Shader.IsValid())
			{
				OpenGLShader* shader = rm->GetShader(localDraw.Shader);

				// Bind Vertex Array
				shader->BindPipeline();

				// Bind shader
				shader->Bind();
			}
			else
			{
				continue;
			}

			for (auto& draw : drawList)
			{
				Mesh* mesh = rm->GetMesh(draw.Mesh);
				Material* material = rm->GetMaterial(draw.Material);

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

				// Set bind group
				if (draw.BindGroup.IsValid())
				{
					OpenGLBindGroup* drawBindGroup = rm->GetBindGroup(draw.BindGroup);
					drawBindGroup->Set();
				}

				// Bind dynamic uniform buffer with the current offset and size
				if (dynamicUniformBuffer != nullptr)
				{
					dynamicUniformBuffer->Bind(draw.Material, 0, draw.Offset, draw.Size);
				}

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
