#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	struct VulkanBindGroupLayout
	{
		VulkanBindGroupLayout() = default;
		VulkanBindGroupLayout(const BindGroupLayoutDescriptor&& desc)
		{
			DebugName = desc.debugName;
			BufferBindings = desc.bufferBindings;
			TextureBindings = desc.textureBindings;

			auto* device = (VulkanDevice*)Device::Instance;

			std::vector<VkDescriptorSetLayoutBinding> bindings(BufferBindings.size() + TextureBindings.size());

			for (int i = 0; i < BufferBindings.size(); i++)
			{
				VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

				switch (BufferBindings[i].type)
				{
				case BufferBindingType::UNIFORM:
					type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					break;
				case BufferBindingType::UNIFORM_DYNAMIC_OFFSET:
					type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					break;
				case BufferBindingType::STORAGE:
					type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					break;
				case BufferBindingType::READ_ONLY_STORAGE:
					type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					break;
				}

				bindings[i] =
				{
					.binding = (uint32_t)i,
					.descriptorType = type,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, // TODO: Get it from visibility field of buffer entry
					.pImmutableSamplers = nullptr,
				};
			}

			for (int i = 0; i < TextureBindings.size(); i++)
			{
				VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

				bindings[i] =
				{
					.binding = TextureBindings[i].slot,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, // TODO: Get it from visibility field of texture entry
					.pImmutableSamplers = nullptr,
				};
			}

			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext = nullptr,
				.bindingCount = (uint32_t)bindings.size(),
				.pBindings = bindings.data(),
			};

			VK_VALIDATE(vkCreateDescriptorSetLayout(device->Get(), &descriptorSetLayoutCreateInfo, nullptr, &DescriptorSetLayout), "vkCreateDescriptorSetLayout");
		}

		void Destroy()
		{
			VulkanDevice* device = (VulkanDevice*)Device::Instance;
			vkDestroyDescriptorSetLayout(device->Get(), DescriptorSetLayout, nullptr);
		}

		const char* DebugName = "";
		VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
		std::vector<BindGroupLayoutDescriptor::BufferBinding> BufferBindings;
		std::vector<BindGroupLayoutDescriptor::TextureBinding> TextureBindings;
	};
}