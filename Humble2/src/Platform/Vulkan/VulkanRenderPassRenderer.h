#pragma once

#include "Renderer\RenderPassRenderer.h"

#include "VulkanCommon.h"

namespace HBL2
{
	class VulkanRenderPasRenderer final : public RenderPassRenderer
	{
	public:
		virtual void DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws) override;

	private:
		VkCommandBuffer m_CommandBuffer;

		friend class VulkanCommandBuffer;
	};
}