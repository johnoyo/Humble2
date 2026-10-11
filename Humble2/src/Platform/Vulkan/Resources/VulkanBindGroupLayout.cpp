#include "VulkanBindGroupLayout.h"

#include "Resources/ResourceManager.h"

namespace HBL2
{
	VulkanBindGroupLayout::VulkanBindGroupLayout(const BindGroupLayoutDescriptor&& desc)
	{
		DebugName = desc.debugName;

		HBL2_CORE_ASSERT(desc.bufferBindings.size() <= BufferBindings.capacity(), "Exceeded max number of buffer bindings in a bind group layout!");
		for (const auto& bufferBinding : desc.bufferBindings)
		{
			BufferBindings.emplace_back(bufferBinding);
		}

		HBL2_CORE_ASSERT(desc.textureBindings.size() <= TextureBindings.capacity(), "Exceeded max number of texture bindings in a bind group layout!");
		for (const auto& textureBinding : desc.textureBindings)
		{
			TextureBindings.emplace_back(textureBinding);
		}

		auto* device = (VulkanDevice*)Device::Instance;

		StaticDArray<VkDescriptorSetLayoutBinding, MaxTextureEntries + MaxBufferEntries> bindings;
		bindings.resize(BufferBindings.size() + TextureBindings.size());

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

			VkShaderStageFlags stage = 0;

			if (BufferBindings[i].visibility.IsSet(ShaderStage::VERTEX))
			{
				stage |= VK_SHADER_STAGE_VERTEX_BIT;
			}

			if (BufferBindings[i].visibility.IsSet(ShaderStage::FRAGMENT))
			{
				stage |= VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			if (BufferBindings[i].visibility.IsSet(ShaderStage::COMPUTE))
			{
				stage |= VK_SHADER_STAGE_COMPUTE_BIT;
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
			case TextureBindingType::SAMPLED_IMAGE:
				type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				break;
			case TextureBindingType::COMBINED_IMAGE_SAMPLER:
				type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				break;
			case TextureBindingType::STORAGE_IMAGE:
				type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				break;
			case TextureBindingType::SAMPLER:
				type = VK_DESCRIPTOR_TYPE_SAMPLER;
				break;
			}

			VkShaderStageFlags stage = 0;

			if (TextureBindings[i].visibility.IsSet(ShaderStage::VERTEX))
			{
				stage |= VK_SHADER_STAGE_VERTEX_BIT;
			}

			if (TextureBindings[i].visibility.IsSet(ShaderStage::FRAGMENT))
			{
				stage |= VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			if (TextureBindings[i].visibility.IsSet(ShaderStage::COMPUTE))
			{
				stage |= VK_SHADER_STAGE_COMPUTE_BIT;
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
			.pBindings = bindings.size() == 0 ? nullptr : bindings.data(),
		};

		VK_VALIDATE(vkCreateDescriptorSetLayout(device->Get(), &descriptorSetLayoutCreateInfo, nullptr, &DescriptorSetLayout), "vkCreateDescriptorSetLayout");

		Hash = ResourceManager::Instance->GetBindGroupLayoutHash(std::forward<const BindGroupLayoutDescriptor>(desc));
	}

	void VulkanBindGroupLayout::Destroy()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		vkDestroyDescriptorSetLayout(device->Get(), DescriptorSetLayout, nullptr);

		DebugName = nullptr;
		Hash = 0;
	}
}
