#pragma once

#include "Renderer\CommandBuffer.h"
#include "VulkanRenderPassRenderer.h"

namespace HBL2
{
	struct CommandBufferCreateInfo
	{
		CommandBufferType type;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		VkFence blockFence = VK_NULL_HANDLE;
		VkSemaphore waitSemaphore = VK_NULL_HANDLE;
		VkSemaphore signalSemaphore = VK_NULL_HANDLE;
	};

	class VulkanCommandBuffer final : public CommandBuffer
	{
	public:
		VulkanCommandBuffer() = default;
		VulkanCommandBuffer(const CommandBufferCreateInfo&& commandBufferCreateInfo) 
			: m_Type(commandBufferCreateInfo.type), CommandBuffer(commandBufferCreateInfo.commandBuffer), m_BlockFence(commandBufferCreateInfo.blockFence), m_WaitSemaphore(commandBufferCreateInfo.waitSemaphore), m_SignalSemaphore(commandBufferCreateInfo.signalSemaphore) {}

		virtual RenderPassRenderer* BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer) override;
		virtual void EndRenderPass(const RenderPassRenderer& renderPassRenderer) override;

		virtual ComputePassRenderer* BeginComputePass(Span<const Handle<Texture>> texturesWrite, Span<const Handle<Buffer>> buffersWrite) override;
		virtual void EndComputePass(const ComputePassRenderer& computePassRenderer) override;

		virtual void EndCommandRecording() override;
		virtual void Submit() override;

		VkCommandBuffer CommandBuffer;

	private:
		VulkanRenderPasRenderer m_CurrentRenderPassRenderer;
		CommandBufferType m_Type;
		VkFence m_BlockFence = VK_NULL_HANDLE;
		VkSemaphore m_WaitSemaphore = VK_NULL_HANDLE;
		VkSemaphore m_SignalSemaphore = VK_NULL_HANDLE;
		inline static bool s_AlreadyCleared = false;
	};
}