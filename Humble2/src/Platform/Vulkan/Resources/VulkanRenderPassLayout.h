#pragma once

#include "Base.h"
#include "Resources/TypeDescriptors.h"
#include "Utilities/Collections/StaticDArray.h"

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct VulkanRenderPassLayout
	{
		VulkanRenderPassLayout() = default;
		VulkanRenderPassLayout(const RenderPassLayoutDescriptor&& desc);

		void Destroy();

		static constexpr uint32_t MaxSubPasses = 4;

		const char* DebugName = "";
		Format DepthTargetFormat = Format::D32_FLOAT;
        StaticDArray<RenderPassLayoutDescriptor::SubPass, MaxSubPasses> SubPasses;
	};
}
