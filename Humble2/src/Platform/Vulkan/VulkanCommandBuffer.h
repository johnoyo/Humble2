#pragma once

#include "Renderer/CommandBuffer.h"
#include "VulkanRenderPassRenderer.h"
#include "VulkanComputePassRenderer.h"

#include "Utilities/Collections/StaticDArray.h"

namespace HBL2
{
    struct VulkanTexture;

    struct VulkanBarrierTracker
    {
        VkPipelineStageFlags SrcStageMask = 0;
        VkPipelineStageFlags DstStageMask = 0;
        StaticDArray<VkImageMemoryBarrier, 8>  ImageBarriers;
        StaticDArray<VkBufferMemoryBarrier, 8> BufferBarriers;

        void AddImageBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkImageMemoryBarrier b);
        void AddBufferBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkBufferMemoryBarrier b);
        void Flush(VkCommandBuffer cmd);
    };

	struct VkCommandBufferCreateInfo
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
		VulkanCommandBuffer(const VkCommandBufferCreateInfo&& commandBufferCreateInfo) 
			: m_Type(commandBufferCreateInfo.type), CommandBuffer(commandBufferCreateInfo.commandBuffer),
			  m_BlockFence(commandBufferCreateInfo.blockFence), m_WaitSemaphore(commandBufferCreateInfo.waitSemaphore),
			  m_SignalSemaphore(commandBufferCreateInfo.signalSemaphore) {}

		virtual RenderPassRenderer* BeginRenderPass(Handle<RenderPass> renderPass, Viewport&& drawArea = {}) override;
		virtual void EndRenderPass(const RenderPassRenderer& renderPassRenderer) override;

		virtual ComputePassRenderer* BeginComputePass(const Span<const Handle<Texture>>& texturesWrite, const Span<const Handle<Buffer>>& buffersWrite) override;
		virtual void EndComputePass(const ComputePassRenderer& computePassRenderer) override;

		virtual void EndCommandRecording() override;
		virtual void Submit() override;
        
        void TextureBarrier(Handle<Texture> texture, ResourceState oldState, ResourceState newState);
        void TextureBarrier(VulkanTexture* vkTexture, ResourceState oldState, ResourceState newState);
        void MemoryBarrier(Handle<Buffer> buffer, ResourceState oldState, ResourceState newState);

		VkCommandBuffer CommandBuffer;
        
        void SetSignalSemophore(VkSemaphore signalSemaphore);

	private:
		CommandBufferType m_Type;

		VulkanRenderPassRenderer m_CurrentRenderPassRenderer;
		VkFence m_BlockFence = VK_NULL_HANDLE;
		VkSemaphore m_WaitSemaphore = VK_NULL_HANDLE;
		VkSemaphore m_SignalSemaphore = VK_NULL_HANDLE;

		VulkanComputePassRenderer m_CurrentComputePassRenderer;
		Span<const Handle<Texture>> m_TexturesWrite;
		Span<const Handle<Buffer>> m_BuffersWrite;
        
        VulkanBarrierTracker m_BarrierTracker;
        bool m_PassOpen = false;

		inline static bool s_AlreadyCleared = false;
	};
}
