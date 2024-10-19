#include "VulkanBindGroup.h"

#include "Platform\Vulkan\VulkanResourceManager.h"

namespace HBL2
{
	VulkanBindGroup::VulkanBindGroup(const BindGroupDescriptor&& desc)
	{
		DebugName = desc.debugName;

		Buffers = desc.buffers;
		Textures = desc.textures;
		BindGroupLayout = desc.layout;

		auto* rm = (VulkanResourceManager*)ResourceManager::Instance;

		VulkanBindGroupLayout* bindGroupLayout = rm->GetBindGroupLayout(BindGroupLayout);

		for (int i = 0; i < Buffers.size(); i++)
		{
			const auto& bufferEntry = Buffers[i];

			VulkanBuffer* vkBuffer = rm->GetBuffer(bufferEntry.buffer);

			VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

			switch (bindGroupLayout->BufferBindings[i].type)
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

			VkDescriptorSetLayoutBinding setBinding =
			{
				.binding = (uint32_t)i,
				.descriptorType = type,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, // TODO: Get it from visibility field of buffer entry
				.pImmutableSamplers = nullptr,
			};
		}

		for (int i = 0; i < Textures.size(); i++)
		{
			VulkanTexture* vkTexture = rm->GetTexture(Textures[i]);

			VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

			VkDescriptorSetLayoutBinding setBinding =
			{
				.binding = bindGroupLayout->TextureBindings[i].slot,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, // TODO: Get it from visibility field of texture entry
				.pImmutableSamplers = nullptr,
			};
		}
	}
}
