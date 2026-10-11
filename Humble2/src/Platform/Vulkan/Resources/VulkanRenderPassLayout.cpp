#include "VulkanRenderPassLayout.h"

namespace HBL2
{
	VulkanRenderPassLayout::VulkanRenderPassLayout(const RenderPassLayoutDescriptor&& desc)
	{
		DebugName = desc.debugName;

		DepthTargetFormat = desc.depthTargetFormat;

		for (const auto& subPass : desc.subPasses)
		{
			SubPasses.push_back(subPass);
		}
	}

	void VulkanRenderPassLayout::Destroy()
	{
		SubPasses.clear();
	}
}
