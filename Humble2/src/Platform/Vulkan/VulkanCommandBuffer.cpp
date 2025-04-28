#include "VulkanCommandBuffer.h"

#include "Core\Window.h"
#include "VulkanRenderer.h"
#include "VulkanResourceManager.h"

namespace HBL2
{
    RenderPassRenderer* VulkanCommandBuffer::BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer)
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
				.offset = { 0, 0 },
				.extent = { vkFrameBuffer->Width, vkFrameBuffer->Height },
			},
			.clearValueCount = (uint32_t)clearValues.size(),
			.pClearValues = clearValues.data(),
		};

		vkCmdBeginRenderPass(CommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Set viewport
		VkViewport viewport =
		{
			.x = 0.0f,
			.y = 0.0f,
			.width = (float)vkFrameBuffer->Width,
			.height = (float)vkFrameBuffer->Height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};

		vkCmdSetViewport(CommandBuffer, 0, 1, &viewport);

		// Set scissor
		VkRect2D scissor =
		{
			.offset = { 0, 0 },
			.extent = { vkFrameBuffer->Width, vkFrameBuffer->Height },
		};

		vkCmdSetScissor(CommandBuffer, 0, 1, &scissor);

        return &m_CurrentRenderPassRenderer;
    }

    void VulkanCommandBuffer::EndRenderPass(const RenderPassRenderer& renderPassRenderer)
    {
		vkCmdEndRenderPass(CommandBuffer);
		VK_VALIDATE(vkEndCommandBuffer(CommandBuffer), "vkEndCommandBuffer");
    }

	ComputePassRenderer* VulkanCommandBuffer::BeginComputePass(Span<const Handle<Texture>> texturesWrite, Span<const Handle<Buffer>> buffersWrite)
	{
		return nullptr;
	}

	void VulkanCommandBuffer::EndComputePass(const ComputePassRenderer& computePassRenderer)
	{
		return;
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
		VK_VALIDATE(vkQueueSubmit(renderer->GetGraphicsQueue(), 1, &submitInfo, m_BlockFence), "vkQueueSubmit");
    }
}
