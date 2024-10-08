#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct VulkanTexture
	{
		VulkanTexture() = default;
		VulkanTexture(const TextureDescriptor&& desc)
		{
			DebugName = desc.debugName;
		}

		const char* DebugName = "";
	};
}