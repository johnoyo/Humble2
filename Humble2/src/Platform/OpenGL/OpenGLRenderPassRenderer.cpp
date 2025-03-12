#include "OpenGLRenderPassRenderer.h"

namespace HBL2
{
	void OpenGLRenderPasRenderer::DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws)
	{
		Renderer::Instance->GetRendererStats().DrawCalls += draws.GetCount();

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

		if (globalDraw.DynamicUniformBufferSize != 0)
		{
			OpenGLBuffer* dynamicUniformBuffer = rm->GetBuffer(Renderer::Instance->TempUniformRingBuffer->GetBuffer());

			glBindBuffer(GL_UNIFORM_BUFFER, dynamicUniformBuffer->RendererId);

			void* ptr = glMapBufferRange(GL_UNIFORM_BUFFER, globalDraw.DynamicUniformBufferOffset, globalDraw.DynamicUniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(ptr, (void*)((char*)dynamicUniformBuffer->Data + globalDraw.DynamicUniformBufferOffset), globalDraw.DynamicUniformBufferSize);
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}

		for (auto&& [shaderID, drawList] : draws.GetDraws())
		{
			auto& localDraw = drawList[0];

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

				const auto& meshPart = mesh->Meshes[draw.MeshIndex];

				// Bind Index buffer if applicable
				if (meshPart.IndexBuffer.IsValid())
				{
					OpenGLBuffer* indexBuffer = rm->GetBuffer(meshPart.IndexBuffer);
					indexBuffer->Bind();
				}

				// Bind vertex buffers
				for (int i = 0; i < meshPart.VertexBuffers.size(); i++)
				{
					OpenGLBuffer* vertexBuffer = rm->GetBuffer(meshPart.VertexBuffers[i]);
					vertexBuffer->Bind(draw.Material, i);
				}

				// Set bind group
				if (draw.BindGroup.IsValid())
				{
					OpenGLBindGroup* drawBindGroup = rm->GetBindGroup(draw.BindGroup);
					drawBindGroup->Set();

					// Bind dynamic uniform buffer with the current offset and size
					OpenGLBuffer* dynamicUniformBuffer = rm->GetBuffer(drawBindGroup->Buffers[0].buffer);
					dynamicUniformBuffer->Bind(draw.Material, 0, draw.Offset, draw.Size);
				}

				const auto& subMesh = meshPart.SubMeshes[draw.SubMeshIndex];

				// Draw the mesh accordingly
				if (meshPart.IndexBuffer.IsValid())
				{
					glDrawElements(GL_TRIANGLES, (subMesh.IndexCount - subMesh.IndexOffset), GL_UNSIGNED_INT, nullptr);
				}
				else
				{
					glDrawArrays(GL_TRIANGLES, subMesh.VertexOffset, subMesh.VertexCount);
				}
			}
		}
	}
}
