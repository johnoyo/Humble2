#include "VulkanBindGroup.h"

#include "Platform\Vulkan\VulkanResourceManager.h"

namespace HBL2
{
	void VulkanBindGroupCold::Destroy()
	{
		auto* rm = (VulkanResourceManager*)ResourceManager::Instance;

		for (int i = 0; i < Buffers.size(); i++)
		{
			// If the range is not 0, this means its a dynamic uniform buffer meaning that is shared across bindgroup, so do not delete.
			if (Buffers[i].range == 0)
			{
				rm->DeleteBuffer(Buffers[i].buffer);
			}
		}

		for (int i = 0; i < Textures.size(); i++)
		{
			// NOTE(John): Do not delete texture since we might use it else where.
			// rm->DeleteTexture(Textures[i]);
		}

		// NOTE: Maybe do not delete this as well, since it might be shared.
		// rm->DeleteBindGroupLayout(BindGroupLayout);
	}

	bool VulkanBindGroup::IsValid() const
	{
		return Cold != nullptr && Hot != nullptr;
	}

	void VulkanBindGroup::Initialize(const BindGroupDescriptor&& desc)
	{
		if (!IsValid())
		{
			return;
		}

		Cold->DebugName = desc.debugName;

		Cold->Buffers = desc.buffers;
		Cold->Textures = desc.textures;
		Cold->BindGroupLayout = desc.layout;

		auto* rm = (VulkanResourceManager*)ResourceManager::Instance;
		auto* renderer = (VulkanRenderer*)Renderer::Instance;
		auto* device = (VulkanDevice*)Device::Instance;

		VulkanBindGroupLayout* bindGroupLayout = rm->GetBindGroupLayout(Cold->BindGroupLayout);

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = renderer->GetDescriptorPool(),
			.descriptorSetCount = 1,
			.pSetLayouts = &bindGroupLayout->DescriptorSetLayout,
		};

		VK_VALIDATE(vkAllocateDescriptorSets(device->Get(), &descriptorSetAllocateInfo, &Hot->DescriptorSet), "vkAllocateDescriptorSets");

		Update();
	}
	
	void VulkanBindGroup::Update()
	{
		if (!IsValid())
		{
			return;
		}

		auto* rm = (VulkanResourceManager*)ResourceManager::Instance;
		auto* device = (VulkanDevice*)Device::Instance;

		VulkanBindGroupLayout* bindGroupLayout = rm->GetBindGroupLayout(Cold->BindGroupLayout);

		std::vector<VkWriteDescriptorSet> writeDescriptorSet(Cold->Buffers.size() + Cold->Textures.size());
		std::vector<VkDescriptorBufferInfo> descriptorBufferInfo(Cold->Buffers.size());
		std::vector<VkDescriptorImageInfo> descriptorImageInfo(Cold->Textures.size());

		for (int i = 0; i < Cold->Buffers.size(); i++)
		{
			VulkanBuffer* buffer = rm->GetBuffer(Cold->Buffers[i].buffer);

			descriptorBufferInfo[i] =
			{
				.buffer = buffer->Buffer,
				.offset = Cold->Buffers[i].byteOffset,
				.range = Cold->Buffers[i].range == 0 ? buffer->ByteSize : Cold->Buffers[i].range,
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
				.dstSet = Hot->DescriptorSet,
				.dstBinding = bindGroupLayout->BufferBindings[i].slot,
				.descriptorCount = 1,
				.descriptorType = descriptorType,
				.pBufferInfo = &descriptorBufferInfo[i],
			};
		}

		for (int i = 0; i < Cold->Textures.size(); i++)
		{
			VulkanTexture* texture = rm->GetTexture(Cold->Textures[i]);

			VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

			switch (bindGroupLayout->TextureBindings[i].type)
			{
			case TextureBindingType::IMAGE_SAMPLER:
				type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				break;
			case TextureBindingType::STORAGE_IMAGE:
				type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				break;
			}

			descriptorImageInfo[i] =
			{
				.sampler = texture->Sampler,
				.imageView = texture->ImageView,
				.imageLayout = texture->ImageLayout,
			};

			writeDescriptorSet[Cold->Buffers.size() + i] =
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = Hot->DescriptorSet,
				.dstBinding = bindGroupLayout->TextureBindings[i].slot,
				.descriptorCount = 1,
				.descriptorType = type,
				.pImageInfo = &descriptorImageInfo[i],
			};
		}

		vkUpdateDescriptorSets(device->Get(), writeDescriptorSet.size(), writeDescriptorSet.data(), 0, nullptr);
	}

	void VulkanBindGroup::Destroy()
	{
		if (!IsValid())
		{
			return;
		}

		Cold->Destroy();
	}
}
