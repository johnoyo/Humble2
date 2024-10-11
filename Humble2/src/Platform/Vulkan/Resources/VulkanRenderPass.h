#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct VulkanRenderPass
	{
		VulkanRenderPass() = default;
		VulkanRenderPass(const RenderPassDescriptor&& desc)
		{
			DebugName = desc.debugName;
		}

		const char* DebugName = "";
		VkRenderPass RenderPass;
	};
}