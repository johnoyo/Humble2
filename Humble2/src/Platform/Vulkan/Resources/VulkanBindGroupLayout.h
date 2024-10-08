#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct VulkanBindGroupLayout
	{
		VulkanBindGroupLayout() = default;
		VulkanBindGroupLayout(const BindGroupLayoutDescriptor&& desc)
		{
			DebugName = desc.debugName;
		}

		const char* DebugName = "";
	};
}