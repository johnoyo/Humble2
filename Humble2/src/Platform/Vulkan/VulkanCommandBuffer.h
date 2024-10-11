#pragma once

#include "Renderer\CommandBuffer.h"
#include "VulkanRenderPassRenderer.h"

namespace HBL2
{
	class VulkanCommandBuffer final : public CommandBuffer
	{
	public:
		virtual RenderPassRenderer* BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer) override;
		virtual void EndRenderPass(const RenderPassRenderer& renderPassRenderer) override;
		virtual void Submit() override;

	private:
		VulkanRenderPasRenderer m_CurrentRenderPassRenderer;
		VkCommandBuffer m_MainCommandBuffer;

		friend class VulkanRenderer;
	};
}