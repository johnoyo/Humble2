#pragma once

#include "Renderer\CommandBuffer.h"
#include "VulkanRenderPassRenderer.h"

namespace HBL2
{
	class VulkanCommandBuffer final : public CommandBuffer
	{
	public:
		VulkanCommandBuffer() = default;
		VulkanCommandBuffer(CommandBufferType type, VkCommandBuffer commandBuffer) : m_Type(type), m_CommandBuffer(commandBuffer)
		{
		}

		virtual RenderPassRenderer* BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer) override;
		virtual void EndRenderPass(const RenderPassRenderer& renderPassRenderer) override;
		virtual void Submit() override;

	private:
		VulkanRenderPasRenderer m_CurrentRenderPassRenderer;
		VkCommandBuffer m_CommandBuffer;
		CommandBufferType m_Type;
	};
}