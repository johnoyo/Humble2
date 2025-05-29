#include "VulkanRenderPassRenderer.h"

#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanResourceManager.h"

namespace HBL2
{
	void VulkanRenderPassRenderer::DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws)
	{
		Renderer::Instance->GetStats().DrawCalls += draws.GetCount();

		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		VulkanBindGroup* globalBindGroup = nullptr;

		if (globalDraw.BindGroup.IsValid())
		{
			globalBindGroup = rm->GetBindGroup(globalDraw.BindGroup);

			// Map global buffers per frame data (i.e.: Camera and lighting data)
			for (const auto& bufferEntry : globalBindGroup->Buffers)
			{
				VulkanBuffer* buffer = rm->GetBuffer(bufferEntry.buffer);

				void* data;
				vmaMapMemory(renderer->GetAllocator(), buffer->Allocation, &data);
				memcpy(data, buffer->Data, buffer->ByteSize);
				vmaUnmapMemory(renderer->GetAllocator(), buffer->Allocation);
			}
		}

		Handle<Buffer> prevIndexBuffer;
		Handle<Buffer> prevVertexBuffer;
		uint64_t prevVariantHash = 0;

		for (const auto& draw : draws.GetDraws())
		{
			VulkanShader* shader = rm->GetShader(draw.Shader);

			if (prevVariantHash != draw.VariantHash)
			{
				// Get pipeline from cache or create it. 
				VkPipeline pipeline = shader->GetOrCreateVariant(draw.VariantHash, draw.Material);

				// Bind pipeline
				vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				// Bind global descriptor set for per frame data (i.e.: Camera and lighting data).
				if (globalBindGroup != nullptr)
				{
					uint32_t offsetCount = (globalDraw.GlobalBufferOffset == UINT32_MAX ? 0 : 1);
					vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->PipelineLayout, 0, 1, &globalBindGroup->DescriptorSet, offsetCount, &globalDraw.GlobalBufferOffset);
				}

				prevVariantHash = draw.VariantHash;
			}

			// Bind the index buffer if needed.
			if (prevIndexBuffer != draw.IndexBuffer)
			{
				if (draw.IndexBuffer.IsValid())
				{
					VulkanBuffer* indexBuffer = rm->GetBuffer(draw.IndexBuffer);
					vkCmdBindIndexBuffer(m_CommandBuffer, indexBuffer->Buffer, 0, VK_INDEX_TYPE_UINT32);

					prevIndexBuffer = draw.IndexBuffer;
				}
			}

			// Bind the vertex buffer if needed.
			if (prevVertexBuffer != draw.VertexBuffer)
			{
				VulkanBuffer* vertexBuffer = rm->GetBuffer(draw.VertexBuffer);
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &vertexBuffer->Buffer, offsets);
				prevVertexBuffer = draw.VertexBuffer;
			}

			// Bind the dynamic uniform buffer in the appropriate offset.
			if (draw.BindGroup.IsValid())
			{
				uint32_t dynamicOffsetCount = (globalDraw.UsesDynamicOffset ? 1 : 0);

				VulkanBindGroup* drawBindGroup = rm->GetBindGroup(draw.BindGroup);
				vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->PipelineLayout, 1, 1, &drawBindGroup->DescriptorSet, dynamicOffsetCount, &draw.Offset);
			}

			// Draw the mesh accordingly
			if (draw.IndexBuffer.IsValid())
			{
				vkCmdDrawIndexed(m_CommandBuffer, draw.IndexCount, draw.InstanceCount, draw.IndexOffset, draw.VertexOffset, draw.InstanceOffset);
			}
			else
			{
				vkCmdDraw(m_CommandBuffer, draw.VertexCount, draw.InstanceCount, draw.VertexOffset, draw.InstanceOffset);
			}
		}
	}
}

/*
	map global bindings

	for each shader
		bind pipeline
		bind global descriptor set

		for each draw
			bind buffers
			bind dynamic uniform buffer descriptor set
			draw
*/
