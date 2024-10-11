#include "VulkanCommandBuffer.h"

#include "VulkanRenderer.h"
#include "VulkanResourceManager.h"

namespace HBL2
{
    RenderPassRenderer* VulkanCommandBuffer::BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer)
    {
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

        m_CurrentRenderPassRenderer.m_CommandBuffer = m_MainCommandBuffer;

		VulkanRenderPass* vkRenderPass = rm->GetRenderPass(renderPass);
		VulkanFrameBuffer* vkFrameBuffer = rm->GetFrameBuffer(frameBuffer);

		VkClearValue clearValue;
		clearValue.color = { { 1.0f, 1.0f, 0.f, 1.0f } };
		VkClearValue depthClear;
		depthClear.depthStencil.depth = 1.f;
		VkClearValue clearValues[] = { clearValue, depthClear };

		VkRenderPassBeginInfo rpInfo ={};
		rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpInfo.pNext = nullptr;
		rpInfo.renderPass = vkRenderPass->RenderPass;
		rpInfo.renderArea.offset.x = 0;
		rpInfo.renderArea.offset.y = 0;
		rpInfo.renderArea.extent = { }; // TODO!
		rpInfo.framebuffer = vkFrameBuffer->FrameBuffer;
		rpInfo.clearValueCount = 2;
		rpInfo.pClearValues = &clearValues[0];

		vkCmdBeginRenderPass(m_MainCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        return &m_CurrentRenderPassRenderer;
    }

    void VulkanCommandBuffer::EndRenderPass(const RenderPassRenderer& renderPassRenderer)
    {
		vkCmdEndRenderPass(m_MainCommandBuffer);
		VK_VALIDATE(vkEndCommandBuffer(m_MainCommandBuffer), "vkEndCommandBuffer");
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
			.pWaitSemaphores = &renderer->GetCurrentFrame().ImageAvailableSemaphore,
			.pWaitDstStageMask = &waitStage,
			.commandBufferCount = 1,
			.pCommandBuffers = &m_MainCommandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &renderer->GetCurrentFrame().MainRenderFinishedSemaphore,
		};

		// Submit command buffer to the queue and execute it.
		// RenderFence will now block until the graphic commands finish execution
		VK_VALIDATE(vkQueueSubmit(renderer->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE), "vkQueueSubmit");
    }
}
