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
		auto* renderer = (VulkanRenderer*)Renderer::Instance;
		auto* device = (VulkanDevice*)Device::Instance;

		VulkanBindGroupLayout* bindGroupLayout = rm->GetBindGroupLayout(BindGroupLayout);

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = renderer->GetDescriptorPool(),
			.descriptorSetCount = 1,
			.pSetLayouts = &bindGroupLayout->DescriptorSetLayout,
		};

		VK_VALIDATE(vkAllocateDescriptorSets(device->Get(), &descriptorSetAllocateInfo, &DescriptorSet), "vkAllocateDescriptorSets");

		std::vector<VkWriteDescriptorSet> writeDescriptorSet(Buffers.size() + Textures.size());
		std::vector<VkDescriptorBufferInfo> descriptorBufferInfo(Buffers.size());
		std::vector<VkDescriptorImageInfo> descriptorImageInfo(Textures.size());

		for (int i = 0; i < Buffers.size(); i++)
		{
			VulkanBuffer* buffer = rm->GetBuffer(Buffers[i].buffer);

			descriptorBufferInfo[i] =
			{
				.buffer = buffer->Buffer,
				.offset = Buffers[i].byteOffset,
				.range = buffer->ByteSize,
			};

			VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;

			switch (bindGroupLayout->BufferBindings[i].type)
			{
			case BufferBindingType::UNIFORM:
				descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				break;
			case BufferBindingType::UNIFORM_DYNAMIC_OFFSET:
				descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				break;
			case BufferBindingType::STORAGE:
				descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				break;
			case BufferBindingType::READ_ONLY_STORAGE:
				descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				break;
			}

			writeDescriptorSet[i] =
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = DescriptorSet,
				.dstBinding = bindGroupLayout->BufferBindings[i].slot,
				.descriptorCount = 1,
				.descriptorType = descriptorType,
				.pBufferInfo = &descriptorBufferInfo[i],
			};
		}

		for (int i = 0; i < Textures.size(); i++)
		{
			VulkanTexture* texture = rm->GetTexture(Textures[i]);

			descriptorImageInfo[i] =
			{
				.sampler = texture->Sampler,
				.imageView = texture->ImageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			writeDescriptorSet[Buffers.size() + i] =
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = DescriptorSet,
				.dstBinding = bindGroupLayout->TextureBindings[i].slot,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &descriptorImageInfo[i],
			};
		}

		vkUpdateDescriptorSets(device->Get(), writeDescriptorSet.size(), writeDescriptorSet.data(), 0, nullptr);
	}
}
