#include "VulkanCommandBuffer.h"

#include "Core/Window.h"
#include "VulkanRenderer.h"
#include "VulkanResourceManager.h"

namespace HBL2
{
    void VulkanBarrierTracker::AddImageBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkImageMemoryBarrier b)
    {
        SrcStageMask |= srcStage;
        DstStageMask |= dstStage;
        ImageBarriers.push_back(b);
    }

    void VulkanBarrierTracker::AddBufferBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkBufferMemoryBarrier b)
    {
        SrcStageMask |= srcStage;
        DstStageMask |= dstStage;
        BufferBarriers.push_back(b);
    }

    void VulkanBarrierTracker::Flush(VkCommandBuffer cmd)
    {
        if (ImageBarriers.empty() && BufferBarriers.empty())
        {
            return;
        }
        
        vkCmdPipelineBarrier(cmd, SrcStageMask, DstStageMask, 0, 0, nullptr, (uint32_t)BufferBarriers.size(), BufferBarriers.data(), (uint32_t)ImageBarriers.size(), ImageBarriers.data());
        
        SrcStageMask = 0;
        DstStageMask = 0;
        ImageBarriers.clear();
        BufferBarriers.clear();
    }

    RenderPassRenderer* VulkanCommandBuffer::BeginRenderPass(Handle<RenderPass> renderPass, Viewport&& drawArea)
    {
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

        m_CurrentRenderPassRenderer.m_CommandBuffer = CommandBuffer;

		if (!renderPass.IsValid())
		{
			return &m_CurrentRenderPassRenderer;
		}

		VulkanRenderPass* vkRenderPass = rm->GetRenderPass(renderPass);

		if (!drawArea.IsValid())
		{
			drawArea =
			{
				0, 0, vkRenderPass->Width, vkRenderPass->Height
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
			.framebuffer = vkRenderPass->FrameBuffer,
			.renderArea =
			{
				.offset = { (int32_t)drawArea.x, (int32_t)drawArea.y },
				.extent = { drawArea.width, drawArea.height },
			},
			.clearValueCount = (uint32_t)clearValues.size(),
			.pClearValues = clearValues.data(),
		};

		vkCmdBeginRenderPass(CommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        // Flush anything queued since the last pass boundary, batched into one call.
        m_BarrierTracker.Flush(CommandBuffer);
        m_PassOpen = true;

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
        m_PassOpen = false;
    }

	ComputePassRenderer* VulkanCommandBuffer::BeginComputePass(const Span<const Handle<Texture>>& texturesWrite, const Span<const Handle<Buffer>>& buffersWrite)
	{
		m_CurrentComputePassRenderer.m_CommandBuffer = CommandBuffer;
        
        m_BarrierTracker.Flush(CommandBuffer);
        m_PassOpen = true;
        
		m_TexturesWrite = texturesWrite;
		m_BuffersWrite = buffersWrite;
        
		return &m_CurrentComputePassRenderer;
	}

	void VulkanCommandBuffer::EndComputePass(const ComputePassRenderer& computePassRenderer)
	{
        for (auto texture : m_TexturesWrite)
        {
            TextureBarrier(texture, ResourceState::UnorderedAccess, ResourceState::GenericRead);
        }
        
        for (auto buffer : m_BuffersWrite)
        {
            MemoryBarrier(buffer, ResourceState::UnorderedAccess, ResourceState::GenericRead);
        }

        m_TexturesWrite = {};
        m_BuffersWrite = {};

        m_PassOpen = false;
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

    void VulkanCommandBuffer::TextureBarrier(Handle<Texture> texture, ResourceState oldState, ResourceState newState)
    {
        VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;
        VulkanTexture* vkTexture = rm->GetTexture(texture);
        
        TextureBarrier(vkTexture, oldState, newState);
    }

    void VulkanCommandBuffer::TextureBarrier(VulkanTexture* vkTexture, ResourceState oldState, ResourceState newState)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VkUtils::ResourceStateToVkImageLayout(oldState);
        barrier.newLayout = VkUtils::ResourceStateToVkImageLayout(newState);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = vkTexture->Image;
        barrier.subresourceRange.aspectMask = vkTexture->Aspect;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = (vkTexture->ImageType == TextureType::CUBE ? 6 : vkTexture->LayerCount);
        barrier.srcAccessMask = VkUtils::ResourceStateToVkAccessFlags(oldState);
        barrier.dstAccessMask = VkUtils::ResourceStateToVkAccessFlags(newState);

        m_BarrierTracker.AddImageBarrier(VkUtils::ResourceStateToVkPipelineStageFlags(oldState), VkUtils::ResourceStateToVkPipelineStageFlags(newState), barrier);

        vkTexture->ImageLayout = barrier.newLayout;

        if (m_PassOpen)
        {
            m_BarrierTracker.Flush(CommandBuffer);
        }
    }

    void VulkanCommandBuffer::MemoryBarrier(Handle<Buffer> buffer, ResourceState oldState, ResourceState newState)
    {
        VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;
        VulkanBufferHot* vkBufferHot = rm->GetBufferHot(buffer);
        VulkanBufferCold* vkBufferCold = rm->GetBufferCold(buffer);
        
        VkBufferMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = VkUtils::ResourceStateToVkAccessFlags(oldState);
        barrier.dstAccessMask = VkUtils::ResourceStateToVkAccessFlags(newState);
        barrier.buffer = vkBufferHot->Buffer;
        barrier.offset = vkBufferCold->ByteOffset;
        barrier.size = vkBufferHot->ByteSize;

        m_BarrierTracker.AddBufferBarrier(VkUtils::ResourceStateToVkPipelineStageFlags(oldState), VkUtils::ResourceStateToVkPipelineStageFlags(newState), barrier);

        if (m_PassOpen)
        {
            m_BarrierTracker.Flush(CommandBuffer);
        }
    }

    void VulkanCommandBuffer::SetSignalSemophore(VkSemaphore signalSemaphore)
    {
        m_SignalSemaphore = signalSemaphore;
    }
}
