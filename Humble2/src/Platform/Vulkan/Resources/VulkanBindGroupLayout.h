#pragma once

#include "Base.h"
#include "Resources/TypeDescriptors.h"

#include "Platform/Vulkan/VulkanDevice.h"

#include "Platform/Vulkan/VulkanCommon.h"

namespace HBL2
{
	struct VulkanBindGroupLayout
	{
		static constexpr uint32_t MaxTextureEntries = 6;
		static constexpr uint32_t MaxBufferEntries = 6;

		VulkanBindGroupLayout() = default;
		VulkanBindGroupLayout(const BindGroupLayoutDescriptor&& desc);

		void Destroy();

		uint64_t Hash = 0;
		const char* DebugName = "";
		VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
		StaticDArray<BindGroupLayoutDescriptor::BufferBinding, MaxBufferEntries> BufferBindings;
		StaticDArray<BindGroupLayoutDescriptor::TextureBinding, MaxTextureEntries> TextureBindings;
	};
}
