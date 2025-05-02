#include "VulkanBindGroupLayout.h"

namespace HBL2
{
	VulkanBindGroupLayout::VulkanBindGroupLayout(const BindGroupLayoutDescriptor&& desc)
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
			case BufferBindingType::STORAGE_IMAGE:
				type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				break;
			}

			VkShaderStageFlags stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

			switch (BufferBindings[i].visibility)
			{
			case ShaderStage::VERTEX:
				stage = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case ShaderStage::FRAGMENT:
				stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			case ShaderStage::COMPUTE:
				stage = VK_SHADER_STAGE_COMPUTE_BIT;
				break;
			}

			bindings[i] =
			{
				.binding = BufferBindings[i].slot,
				.descriptorType = type,
				.descriptorCount = 1,
				.stageFlags = stage,
				.pImmutableSamplers = nullptr,
			};
		}

		for (int i = 0; i < TextureBindings.size(); i++)
		{
			VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

			switch (TextureBindings[i].type)
			{
			case TextureBindingType::IMAGE_SAMPLER:
				type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				break;
			case TextureBindingType::STORAGE_IMAGE:
				type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				break;
			}

			VkShaderStageFlags stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

			switch (TextureBindings[i].visibility)
			{
			case ShaderStage::VERTEX:
				stage = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case ShaderStage::FRAGMENT:
				stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			case ShaderStage::COMPUTE:
				stage = VK_SHADER_STAGE_COMPUTE_BIT;
				break;
			}

			bindings[BufferBindings.size() + i] =
			{
				.binding = TextureBindings[i].slot,
				.descriptorType = type,
				.descriptorCount = 1,
				.stageFlags = stage,
				.pImmutableSamplers = nullptr,
			};
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = (uint32_t)bindings.size(),
			.pBindings = bindings.data(),
		};

		VK_VALIDATE(vkCreateDescriptorSetLayout(device->Get(), &descriptorSetLayoutCreateInfo, nullptr, &DescriptorSetLayout), "vkCreateDescriptorSetLayout");
	}

	void VulkanBindGroupLayout::Destroy()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		vkDestroyDescriptorSetLayout(device->Get(), DescriptorSetLayout, nullptr);
	}
}
