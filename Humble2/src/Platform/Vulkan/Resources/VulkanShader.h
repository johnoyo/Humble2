#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	struct VulkanShader
	{
		VulkanShader() = default;
		VulkanShader(const ShaderDescriptor&& desc)
		{
			DebugName = desc.debugName;
		}

		const char* DebugName = "";
		VkPipeline Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
		VkShaderModule FragmentShaderModule = VK_NULL_HANDLE;
		VkShaderModule VertexShaderModule = VK_NULL_HANDLE;
	};
}