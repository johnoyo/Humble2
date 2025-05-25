#include "OpenGLRenderPassRenderer.h"

namespace HBL2
{
	void OpenGLRenderPasRenderer::DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws)
	{
		Renderer::Instance->GetStats().DrawCalls += draws.GetCount();

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

				if (globalDraw.GlobalBufferOffset != UINT32_MAX)
				{
					glBindBuffer(GL_UNIFORM_BUFFER, buffer->RendererId);

					void* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
					memcpy(ptr, (void*)buffer->Data, buffer->ByteSize);
					glUnmapBuffer(GL_UNIFORM_BUFFER);

					glBindBufferRange(GL_UNIFORM_BUFFER, globalBindGroupLayout->BufferBindings[index++].slot, buffer->RendererId, globalDraw.GlobalBufferOffset, globalDraw.GlobalBufferSize);
				}
				else
				{
					glBindBufferBase(GL_UNIFORM_BUFFER, globalBindGroupLayout->BufferBindings[index++].slot, buffer->RendererId);
					buffer->Write();
				}
			}

			// Set global bind group
			globalBindGroup->Set();
		}

		Handle<Buffer> prevIndexBuffer;
		StaticArray<Handle<Buffer>, 3> prevVertexBuffers{};
		Handle<BindGroup> previouslyUsedBindGroup;

		for (auto&& [shaderID, drawList] : draws.GetDraws())
		{
			auto& localDraw = drawList[0];

			if (localDraw.Shader.IsValid())
			{
				Material* mat = rm->GetMaterial(localDraw.Material);
				OpenGLShader* shader = rm->GetShader(localDraw.Shader);

				// Set blend, depth state.
				shader->SetVariantProperties(mat->VariantDescriptor);

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
				if (prevIndexBuffer != meshPart.IndexBuffer)
				{
					if (meshPart.IndexBuffer.IsValid())
					{
						OpenGLBuffer* indexBuffer = rm->GetBuffer(meshPart.IndexBuffer);
						indexBuffer->Bind();

						prevIndexBuffer = meshPart.IndexBuffer;
					}
				}

				// Bind vertex buffers
				HBL2_CORE_ASSERT(meshPart.VertexBuffers.size() <= 3, "Maximum number of vertex buffers is 3.");
				HBL2_CORE_ASSERT(meshPart.VertexBuffers.size() == 1, "One packed vertex buffer is supported for now.");

				for (int i = 0; i < meshPart.VertexBuffers.size(); i++)
				{
					if (prevVertexBuffers[i] != meshPart.VertexBuffers[i])
					{
						OpenGLBuffer* vertexBuffer = rm->GetBuffer(meshPart.VertexBuffers[i]);
						vertexBuffer->Bind(draw.Material, i);

						prevVertexBuffers[i] = meshPart.VertexBuffers[i];
					}
				}

				// Set bind group
				if (draw.BindGroup.IsValid())
				{
					OpenGLBindGroup* drawBindGroup = rm->GetBindGroup(draw.BindGroup);

					if (draw.BindGroup != previouslyUsedBindGroup)
					{
						drawBindGroup->Set();
						previouslyUsedBindGroup = draw.BindGroup;
					}

					// Bind dynamic uniform buffer with the current offset and size
					if (globalDraw.UsesDynamicOffset)
					{
						OpenGLBuffer* dynamicUniformBuffer = rm->GetBuffer(drawBindGroup->Buffers[0].buffer);
						dynamicUniformBuffer->Bind(draw.Material, 0, draw.Offset, draw.Size);
					}
				}

				const auto& subMesh = meshPart.SubMeshes[draw.SubMeshIndex];

				// Draw the mesh accordingly
				if (meshPart.IndexBuffer.IsValid())
				{
					if (subMesh.IndexOffset == 0)
					{
						glDrawElements(GL_TRIANGLES, subMesh.IndexCount, GL_UNSIGNED_INT, nullptr);
					}
					else
					{
						glDrawElementsBaseVertex(GL_TRIANGLES, subMesh.IndexCount, GL_UNSIGNED_INT, (void*)(subMesh.IndexOffset * sizeof(uint32_t)), subMesh.VertexOffset);
					}
				}
				else
				{
					glDrawArrays(GL_TRIANGLES, subMesh.VertexOffset, subMesh.VertexCount);
				}
			}
		}
	}
}
