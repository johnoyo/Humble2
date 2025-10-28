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
		Handle<Buffer> prevVertexBuffer;
		Handle<BindGroup> previouslyUsedBindGroup;
		uint64_t prevVariantHash = 0;

		for (const auto& draw : draws.GetDraws())
		{
			Material* material = rm->GetMaterial(draw.Material);

			if (prevVariantHash != draw.VariantHash)
			{
				OpenGLShader* shader = rm->GetShader(draw.Shader);

				// Set blend, depth state.
				shader->SetVariantProperties(material->VariantDescriptor);

				// Bind Vertex Array
				shader->BindPipeline();

				// Bind shader
				shader->Bind();

				prevVariantHash = draw.VariantHash;
			}

			// Bind Index buffer if applicable
			if (prevIndexBuffer != draw.IndexBuffer)
			{
				if (draw.IndexBuffer.IsValid())
				{
					OpenGLBuffer* indexBuffer = rm->GetBuffer(draw.IndexBuffer);
					indexBuffer->Bind();

					prevIndexBuffer = draw.IndexBuffer;
				}
			}

			// Bind the vertex buffer if needed.
			if (prevVertexBuffer != draw.VertexBuffer)
			{
				OpenGLBuffer* vertexBuffer = rm->GetBuffer(draw.VertexBuffer);
				vertexBuffer->Bind(draw.Material, 0);

				prevVertexBuffer = draw.VertexBuffer;
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

			GLenum topology = OpenGLUtils::TopologyToGLenum(material->VariantDescriptor.topology);

			// Draw the mesh accordingly
			if (draw.IndexBuffer.IsValid())
			{
				if (draw.IndexOffset == 0)
				{
					glDrawElements(topology, draw.IndexCount, GL_UNSIGNED_INT, nullptr);
				}
				else
				{
					glDrawElementsBaseVertex(topology, draw.IndexCount, GL_UNSIGNED_INT, (void*)(draw.IndexOffset * sizeof(uint32_t)), draw.VertexOffset);
				}
			}
			else
			{
				glDrawArrays(topology, draw.VertexOffset, draw.VertexCount);
			}
		}
	}
}
