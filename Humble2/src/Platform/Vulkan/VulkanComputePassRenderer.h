#pragma once

#include "Renderer/ComputePassRenderer.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	class VulkanComputePassRenderer final : public ComputePassRenderer
	{
	public:
		virtual void Dispatch(const Span<const HBL2::Dispatch>& dispatches) override;

	private:
		VkCommandBuffer m_CommandBuffer;
		friend class VulkanCommandBuffer;
	};
}