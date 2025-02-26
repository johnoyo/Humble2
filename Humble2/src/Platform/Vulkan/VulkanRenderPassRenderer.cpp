#include "VulkanRenderPassRenderer.h"

#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanResourceManager.h"

namespace HBL2
{
	void VulkanRenderPasRenderer::DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws)
	{
		Renderer::Instance->GetRendererStats().DrawCalls += draws.GetCount();

		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		VulkanBindGroup* globalBindGroup = nullptr;

		if (globalDraw.BindGroup.IsValid())
		{
			globalBindGroup = rm->GetBindGroup(globalDraw.BindGroup);

			for (const auto& bufferEntry : globalBindGroup->Buffers)
			{
				VulkanBuffer* buffer = rm->GetBuffer(bufferEntry.buffer);

				void* data;
				vmaMapMemory(renderer->GetAllocator(), buffer->Allocation, &data);
				memcpy(data, buffer->Data, buffer->ByteSize);
				vmaUnmapMemory(renderer->GetAllocator(), buffer->Allocation);
			}
		}

		if (globalDraw.DynamicUniformBufferSize != 0)
		{
			VulkanBuffer* dynamicUniformBuffer = rm->GetBuffer(Renderer::Instance->TempUniformRingBuffer->GetBuffer());

			void* data;
			vmaMapMemory(renderer->GetAllocator(), dynamicUniformBuffer->Allocation, &data);
			memcpy((void*)((char*)data + globalDraw.DynamicUniformBufferOffset), (void*)((char*)dynamicUniformBuffer->Data + globalDraw.DynamicUniformBufferOffset), globalDraw.DynamicUniformBufferSize);
			vmaUnmapMemory(renderer->GetAllocator(), dynamicUniformBuffer->Allocation);
		}

		for (auto&& [shaderID, drawList] : draws.GetDraws())
		{
			auto& localDraw = drawList[0];

			Material* localMaterial0 = rm->GetMaterial(localDraw.Material);
			VulkanShader* localShader0 = rm->GetShader(localMaterial0->Shader);

			// Bind pipeline
			vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, localShader0->Pipeline);

			// Bind global descriptor set
			if (globalBindGroup != nullptr)
			{
				vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, localShader0->PipelineLayout, 0, 1, &globalBindGroup->DescriptorSet, 0, nullptr);
			}

			for (auto& draw : drawList)
			{
				Mesh* mesh = rm->GetMesh(draw.Mesh);
				Material* material = rm->GetMaterial(draw.Material);
				VulkanShader* shader = rm->GetShader(material->Shader);

				// vkCmdBindIndexBuffer
				if (mesh->IndexBuffer.IsValid())
				{
					VulkanBuffer* indexBuffer = rm->GetBuffer(mesh->IndexBuffer);
					vkCmdBindIndexBuffer(m_CommandBuffer, indexBuffer->Buffer, 0, VK_INDEX_TYPE_UINT32);
				}

				// vkCmdBindVertexBuffers
				uint32_t bufferCounter = 0;
				std::vector<VkBuffer> buffers(mesh->VertexBuffers.size());
				std::vector<VkDeviceSize> offsets(mesh->VertexBuffers.size());
				for (const auto vertexBufferHandle : mesh->VertexBuffers)
				{
					VulkanBuffer* vertexBuffer = rm->GetBuffer(vertexBufferHandle);
					buffers[bufferCounter] = vertexBuffer->Buffer;
					offsets[bufferCounter] = 0;
					bufferCounter++;
				}
				vkCmdBindVertexBuffers(m_CommandBuffer, 0, bufferCounter, buffers.data(), offsets.data());

				// vkCmdBindDescriptorSets
				if (draw.BindGroup.IsValid())
				{
					VulkanBindGroup* drawBindGroup = rm->GetBindGroup(draw.BindGroup);
					vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->PipelineLayout, 1, 1, &drawBindGroup->DescriptorSet, 1, &draw.Offset);
				}

				// Draw the mesh accordingly
				if (mesh->IndexBuffer.IsValid())
				{
					// vkCmdDrawIndexed
					vkCmdDrawIndexed(m_CommandBuffer, mesh->IndexCount, mesh->InstanceCount, mesh->IndexOffset, mesh->VertexOffset, mesh->InstanceOffset);
				}
				else
				{
					// vkCmdDraw
					vkCmdDraw(m_CommandBuffer, mesh->VertexCount, mesh->InstanceCount, mesh->VertexOffset, mesh->InstanceOffset);
				}
			}
		}
	}
}
