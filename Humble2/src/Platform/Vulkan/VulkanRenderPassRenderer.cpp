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

		Handle<Buffer> prevIndexBuffer;
		StaticArray<Handle<Buffer>, 3> prevVertexBuffers;

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

				const auto& meshPart = mesh->Meshes[draw.MeshIndex];

				// vkCmdBindIndexBuffer
				if (prevIndexBuffer != meshPart.IndexBuffer)
				{
					if (meshPart.IndexBuffer.IsValid())
					{
						VulkanBuffer* indexBuffer = rm->GetBuffer(meshPart.IndexBuffer);
						vkCmdBindIndexBuffer(m_CommandBuffer, indexBuffer->Buffer, 0, VK_INDEX_TYPE_UINT32);

						prevIndexBuffer = meshPart.IndexBuffer;
					}
				}

				// vkCmdBindVertexBuffers
				bool rebindVertexBuffers = false;
				uint32_t bufferCounter = 0;
				std::vector<VkBuffer> buffers(meshPart.VertexBuffers.size());
				std::vector<VkDeviceSize> offsets(meshPart.VertexBuffers.size());
				HBL2_CORE_ASSERT(meshPart.VertexBuffers.size() <= 3, "Maximum number of vertex buffers is 3.");
				for (const auto vertexBufferHandle : meshPart.VertexBuffers)
				{
					if (prevVertexBuffers[bufferCounter] != vertexBufferHandle)
					{
						rebindVertexBuffers = true;
						prevVertexBuffers[bufferCounter] = vertexBufferHandle;
					}

					VulkanBuffer* vertexBuffer = rm->GetBuffer(vertexBufferHandle);
					buffers[bufferCounter] = vertexBuffer->Buffer;
					offsets[bufferCounter] = 0;
					bufferCounter++;
				}

				if (rebindVertexBuffers)
				{
					vkCmdBindVertexBuffers(m_CommandBuffer, 0, bufferCounter, buffers.data(), offsets.data());
				}

				// vkCmdBindDescriptorSets
				if (draw.BindGroup.IsValid())
				{
					VulkanBindGroup* drawBindGroup = rm->GetBindGroup(draw.BindGroup);
					vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->PipelineLayout, 1, 1, &drawBindGroup->DescriptorSet, 1, &draw.Offset);
				}

				const auto& subMesh = meshPart.SubMeshes[draw.SubMeshIndex];

				// Draw the mesh accordingly
				if (meshPart.IndexBuffer.IsValid())
				{
					// vkCmdDrawIndexed
					vkCmdDrawIndexed(m_CommandBuffer, subMesh.IndexCount, subMesh.InstanceCount, subMesh.IndexOffset, subMesh.VertexOffset, subMesh.InstanceOffset);
				}
				else
				{
					// vkCmdDraw
					vkCmdDraw(m_CommandBuffer, subMesh.VertexCount, subMesh.InstanceCount, subMesh.VertexOffset, subMesh.InstanceOffset);
				}
			}
		}
	}
}
