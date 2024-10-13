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

        m_CurrentRenderPassRenderer.m_CommandBuffer = m_CommandBuffer;

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
		rpInfo.renderArea.offset = { 0, 0 };
		rpInfo.renderArea.extent = { Window::Instance->GetExtents().x, Window::Instance->GetExtents().y };
		rpInfo.framebuffer = renderer->GetMainFrameBuffer(); // vkFrameBuffer->FrameBuffer; // TODO!

		if (m_Type == CommandBufferType::MAIN)
		{
			rpInfo.clearValueCount = 2;
			rpInfo.pClearValues = &clearValues[0];
		}
		else
		{
			rpInfo.clearValueCount = 0;
			rpInfo.pClearValues = nullptr;
		}

		vkCmdBeginRenderPass(m_CommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        return &m_CurrentRenderPassRenderer;
    }

    void VulkanCommandBuffer::EndRenderPass(const RenderPassRenderer& renderPassRenderer)
    {
		vkCmdEndRenderPass(m_CommandBuffer);
		VK_VALIDATE(vkEndCommandBuffer(m_CommandBuffer), "vkEndCommandBuffer");
    }

    void VulkanCommandBuffer::Submit()
    {
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

		VkFence blockFence = VK_NULL_HANDLE;
		VkSemaphore waitSemaphore = VK_NULL_HANDLE;
		VkSemaphore signalSemaphore = VK_NULL_HANDLE;

		switch (m_Type)
		{
		case HBL2::CommandBufferType::MAIN:
			blockFence = VK_NULL_HANDLE;
			waitSemaphore = renderer->GetCurrentFrame().ImageAvailableSemaphore;
			signalSemaphore = renderer->GetCurrentFrame().MainRenderFinishedSemaphore;
			break;
		case HBL2::CommandBufferType::OFFSCREEN:
			break;
		case HBL2::CommandBufferType::UI:
			blockFence = renderer->GetCurrentFrame().InFlightFence;
			waitSemaphore = renderer->GetCurrentFrame().MainRenderFinishedSemaphore;
			signalSemaphore = renderer->GetCurrentFrame().ImGuiRenderFinishedSemaphore;
			break;
		default:
			blockFence = VK_NULL_HANDLE; // TODO: Use renderer->ploadContext.UploadFence
			waitSemaphore = VK_NULL_HANDLE;
			signalSemaphore = VK_NULL_HANDLE;
			break;
		}

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &waitSemaphore,
			.pWaitDstStageMask = &waitStage,
			.commandBufferCount = 1,
			.pCommandBuffers = &m_CommandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &signalSemaphore,
		};

		// Submit command buffer to the queue and execute it. RenderFence will now block until the graphic commands finish execution.
		VK_VALIDATE(vkQueueSubmit(renderer->GetGraphicsQueue(), 1, &submitInfo, blockFence), "vkQueueSubmit");

		// TODO: Wait for fences and reset fence when on offscreen.
    }
}
