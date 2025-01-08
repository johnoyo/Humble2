#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct VulkanRenderPassLayout
	{
		VulkanRenderPassLayout() = default;
		VulkanRenderPassLayout(const RenderPassLayoutDescriptor&& desc)
		{
			DebugName = desc.debugName;

			DepthTargetFormat = desc.depthTargetFormat;

			for (const auto& subPass : desc.subPasses)
			{
				SubPasses.push_back(subPass);
			}
		}

		const char* DebugName = "";
		Format DepthTargetFormat = Format::D32_FLOAT;
		std::vector<RenderPassLayoutDescriptor::SubPass> SubPasses;
	};
}