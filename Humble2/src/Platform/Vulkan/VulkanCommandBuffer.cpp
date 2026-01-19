#include "VulkanCommandBuffer.h"

#include "Core\Window.h"
#include "VulkanRenderer.h"
#include "VulkanResourceManager.h"

namespace HBL2
{
    RenderPassRenderer* VulkanCommandBuffer::BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer, Viewport&& drawArea)
    {
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

        m_CurrentRenderPassRenderer.m_CommandBuffer = CommandBuffer;

		if (!renderPass.IsValid())
		{
			return &m_CurrentRenderPassRenderer;
		}

		VulkanRenderPass* vkRenderPass = rm->GetRenderPass(renderPass);

		if (!frameBuffer.IsValid())
		{
			return &m_CurrentRenderPassRenderer;
		}

		VulkanFrameBuffer* vkFrameBuffer = rm->GetFrameBuffer(frameBuffer);

		if (!drawArea.IsValid())
		{
			drawArea =
			{
				0, 0, vkFrameBuffer->Width, vkFrameBuffer->Height
			};
		}

		std::vector<VkClearValue> clearValues;

		for (auto clearValue : vkRenderPass->ColorClearValues)
		{
			if (clearValue)
			{
				VkClearValue clearValue;
				clearValue.color = { { vkRenderPass->ClearColor.r, vkRenderPass->ClearColor.g, vkRenderPass->ClearColor.b, vkRenderPass->ClearColor.a } };
				clearValues.push_back(clearValue);
			}
		}

		if (vkRenderPass->DepthClearValue)
		{
			VkClearValue depthClear;
			depthClear.depthStencil.depth = vkRenderPass->ClearDepth;
			clearValues.push_back(depthClear);
		}

		VkRenderPassBeginInfo rpInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = vkRenderPass->RenderPass,
			.framebuffer = vkFrameBuffer->FrameBuffer,
			.renderArea = 
			{
				.offset = { (int32_t)drawArea.x, (int32_t)drawArea.y },
				.extent = { drawArea.width, drawArea.height },
			},
			.clearValueCount = (uint32_t)clearValues.size(),
			.pClearValues = clearValues.data(),
		};

		vkCmdBeginRenderPass(CommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Set viewport
		VkViewport viewport =
		{
			.x = (float)drawArea.x,
			.y = (float)drawArea.y,
			.width = (float)drawArea.width,
			.height = (float)drawArea.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};

		vkCmdSetViewport(CommandBuffer, 0, 1, &viewport);

		// Set scissor
		VkRect2D scissor =
		{
			.offset = {(int32_t)drawArea.x, (int32_t)drawArea.y },
			.extent = { drawArea.width, drawArea.height },
		};

		vkCmdSetScissor(CommandBuffer, 0, 1, &scissor);

        return &m_CurrentRenderPassRenderer;
    }

    void VulkanCommandBuffer::EndRenderPass(const RenderPassRenderer& renderPassRenderer)
    {
		vkCmdEndRenderPass(CommandBuffer);
    }

	ComputePassRenderer* VulkanCommandBuffer::BeginComputePass(const Span<const Handle<Texture>>& texturesWrite, const Span<const Handle<Buffer>>& buffersWrite)
	{
		m_CurrentComputePassRenderer.m_CommandBuffer = CommandBuffer;
		m_TexturesWrite = texturesWrite;
		m_BuffersWrite = buffersWrite;
		return &m_CurrentComputePassRenderer;
	}

	void VulkanCommandBuffer::EndComputePass(const ComputePassRenderer& computePassRenderer)
	{
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		std::vector<VkImageMemoryBarrier> imageBarriers(m_TexturesWrite.Size());
		std::vector<VkBufferMemoryBarrier> bufferBarriers(m_BuffersWrite.Size());

		uint32_t index = 0;

		for (auto& texture : m_TexturesWrite)
		{
			VulkanTexture* vkTexture = rm->GetTexture(texture);

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = vkTexture->ImageLayout;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Fix
			barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			barrier.image = vkTexture->Image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = (vkTexture->ImageType == TextureType::CUBE ? 6 : 1);

			imageBarriers[index++] = barrier;

			vkTexture->ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		index = 0;

		for (auto& buffer : m_BuffersWrite)
		{
			VulkanBuffer* vkBuffer = rm->GetBuffer(buffer);

			VkBufferMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; // Fix
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT; // Fix
			barrier.buffer = vkBuffer->Buffer;
			barrier.offset = vkBuffer->ByteOffset;
			barrier.size = vkBuffer->ByteSize;

			bufferBarriers[index++] = barrier;
		}

		if (!imageBarriers.empty() || !bufferBarriers.empty())
		{
			vkCmdPipelineBarrier(
				CommandBuffer,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
				static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data());
		}

		return;
	}

	void VulkanCommandBuffer::EndCommandRecording()
	{
		VK_VALIDATE(vkEndCommandBuffer(CommandBuffer), "vkEndCommandBuffer");
	}

	void VulkanCommandBuffer::Submit()
    {
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &m_WaitSemaphore,
			.pWaitDstStageMask = &waitStage,
			.commandBufferCount = 1,
			.pCommandBuffers = &CommandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &m_SignalSemaphore,
		};

		// Submit command buffer to the queue and execute it. RenderFence will now block until the graphic commands finish execution.
		{
			std::lock_guard<std::mutex> lock(renderer->GetGraphicsQueueMutex());
			VK_VALIDATE(vkQueueSubmit(renderer->GetGraphicsQueue(), 1, &submitInfo, m_BlockFence), "vkQueueSubmit");
		}
    }
}
