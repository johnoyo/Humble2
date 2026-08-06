#include "VulkanRenderPassRenderer.h"

#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanResourceManager.h"

/*
* Overview of draw loop:
* 
*	map global bindings
* 
*	for each draw
* 		bind pipeline if changed
* 		bind global descriptor set if it and the shader changed
* 		bind global shader descriptor set if it and the shader changed
* 		bind material descriptor set if material changed
* 		bind vertex and index buffers if changed
* 		bind dynamic uniform buffer descriptor set
* 		draw
*/

namespace HBL2
{
	void VulkanRenderPassRenderer::DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws)
	{
		Renderer::Instance->GetStats().DrawCalls += draws.GetCount();

		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		// Global descriptor set.
		VulkanBindGroupHot* globalBindGroupHot = nullptr;
		VulkanBindGroupCold* globalBindGroupCold = nullptr;

		if (globalDraw.BindGroup.IsValid())
		{
			globalBindGroupHot = rm->GetBindGroupHot(globalDraw.BindGroup);
			globalBindGroupCold = rm->GetBindGroupCold(globalDraw.BindGroup);

			// Map global buffers per frame data (i.e.: Camera and lighting data)
			for (const auto& bufferEntry : globalBindGroupCold->Buffers)
			{
				VulkanBufferHot* buffer = rm->GetBufferHot(bufferEntry.buffer);

				void* data;
				vmaMapMemory(renderer->GetAllocator(), buffer->Allocation, &data);
				memcpy(data, buffer->Data, buffer->ByteSize);
				vmaUnmapMemory(renderer->GetAllocator(), buffer->Allocation);
			}
		}

		Handle<Buffer> prevIndexBuffer;
		Handle<Buffer> prevVertexBuffer;
		Handle<Shader> prevShader;
		Handle<BindGroup> prevShaderBindGroup;
		Handle<BindGroup> prevMaterialBindGroup;

		uint64_t prevVariantHash = 0;
		uint64_t prevBindGroupLayoutHash = 0;

		for (const auto& draw : draws.GetDraws())
		{
			VulkanShaderHot* shader = rm->GetShaderHot(draw.Shader);

			if (prevVariantHash != draw.VariantHandle || prevShader != draw.Shader)
			{
				// Bind pipeline
				vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)draw.VariantHandle);

				prevVariantHash = draw.VariantHandle;
				prevShader = draw.Shader;

				// Bind global descriptor set for per frame data if needed (i.e.: Camera and lighting data).
				if (prevBindGroupLayoutHash != shader->GlobalBindGroupLayoutHash)
				{
					if (globalBindGroupHot != nullptr)
					{
						uint32_t offsetCount = (globalDraw.GlobalBufferOffset == UINT32_MAX ? 0 : 1);
						const uint32_t* offset = (globalDraw.GlobalBufferOffset == UINT32_MAX ? nullptr : &globalDraw.GlobalBufferOffset);
						vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->PipelineLayout, 0, 1, &globalBindGroupHot->DescriptorSet, offsetCount, offset);
					}

					prevBindGroupLayoutHash = shader->GlobalBindGroupLayoutHash;
				}

				// Bind global shader descriptor set for custom per frame data if needed.
				if (shader->ShaderBindGroup.IsValid() && prevShaderBindGroup != shader->ShaderBindGroup)
				{
					VulkanBindGroupHot* shaderBindGroupHot = rm->GetBindGroupHot(shader->ShaderBindGroup);

					if (shaderBindGroupHot != nullptr)
					{
						vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->PipelineLayout, 1, 1, &shaderBindGroupHot->DescriptorSet, 0, nullptr);
					}

					prevShaderBindGroup = shader->ShaderBindGroup;
				}
			}

			// Bind the per material bind group if needed.
			if (draw.MaterialBindGroup.IsValid() && prevMaterialBindGroup != draw.MaterialBindGroup)
			{
				VulkanBindGroupHot* materialBindGroupHot = rm->GetBindGroupHot(draw.MaterialBindGroup);
				vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->PipelineLayout, 2, 1, &materialBindGroupHot->DescriptorSet, 0, nullptr);
				prevMaterialBindGroup = draw.MaterialBindGroup;
			}

			// Bind the index buffer if needed.
			if (prevIndexBuffer != draw.IndexBuffer)
			{
				if (draw.IndexBuffer.IsValid())
				{
					VulkanBufferHot* indexBuffer = rm->GetBufferHot(draw.IndexBuffer);
					vkCmdBindIndexBuffer(m_CommandBuffer, indexBuffer->Buffer, 0, VK_INDEX_TYPE_UINT32);

					prevIndexBuffer = draw.IndexBuffer;
				}
			}

			// Bind the vertex buffer if needed.
			if (prevVertexBuffer != draw.VertexBuffer)
			{
				VulkanBufferHot* vertexBuffer = rm->GetBufferHot(draw.VertexBuffer);
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &vertexBuffer->Buffer, offsets);
				prevVertexBuffer = draw.VertexBuffer;
			}

			// Bind the dynamic uniform buffer in the appropriate offset.
			if (draw.BindGroup.IsValid())
			{
				uint32_t dynamicOffsetCount = (globalDraw.UsesDynamicOffset ? 1 : 0);

				VulkanBindGroupHot* drawBindGroupHot = rm->GetBindGroupHot(draw.BindGroup);
				vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->PipelineLayout, 3, 1, &drawBindGroupHot->DescriptorSet, dynamicOffsetCount, &draw.Offset);
			}

			// Draw the mesh accordingly.
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
