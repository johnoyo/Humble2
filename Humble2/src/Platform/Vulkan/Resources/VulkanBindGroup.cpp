#include "VulkanBindGroup.h"

#include "Platform/Vulkan/VulkanResourceManager.h"

namespace HBL2
{
	void VulkanBindGroupCold::Destroy()
	{
		auto* rm = (VulkanResourceManager*)ResourceManager::Instance;

		BindGroupLayout.Release();

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
			rm->RemoveReimportDependency(Textures[i].dependency);
			// Textures[i].texture.Release();
		}
	}

	bool VulkanBindGroup::IsValid() const
	{
		return Cold != nullptr && Hot != nullptr;
	}

	void VulkanBindGroup::Initialize(Handle<BindGroup> self, const BindGroupDescriptor&& desc)
	{
		if (!IsValid())
		{
			return;
		}

		auto* rm = (VulkanResourceManager*)ResourceManager::Instance;

		Cold->DebugName = desc.debugName;

		HBL2_CORE_ASSERT(desc.buffers.size() < Cold->Buffers.capacity(), "Exceeded max number of buffers in a bind group!");
		for (const auto& bufferEntry : desc.buffers)
		{
			Cold->Buffers.push_back({ bufferEntry.buffer, bufferEntry.byteOffset, bufferEntry.range });
		}

		HBL2_CORE_ASSERT(desc.textures.size() < Cold->Textures.capacity(), "Exceeded max number of textures in a bind group!");
		for (const auto& textureEntry : desc.textures)
		{
			Handle<ReimportDependency> dependencyHandle = rm->AddReimportDependency(textureEntry.texture, self);
			Cold->Textures.push_back({ textureEntry.texture, textureEntry.desiredLayout, dependencyHandle });
		}

		Cold->BindGroupLayout = desc.layout;

		auto* renderer = (VulkanRenderer*)Renderer::Instance;
		auto* device = (VulkanDevice*)Device::Instance;

		VulkanBindGroupLayout* bindGroupLayout = rm->GetBindGroupLayout(Cold->BindGroupLayout.Get());

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

		if (Cold->Buffers.size() + Cold->Textures.size() == 0)
		{
			vkUpdateDescriptorSets(device->Get(), 0, nullptr, 0, nullptr);
			return;
		}

		VulkanBindGroupLayout* bindGroupLayout = rm->GetBindGroupLayout(Cold->BindGroupLayout.Get());

		StaticDArray<VkWriteDescriptorSet, VulkanBindGroupCold::MaxTextureEntries + VulkanBindGroupCold::MaxBufferEntries> writeDescriptorSet;
		writeDescriptorSet.resize(Cold->Textures.size() + Cold->Buffers.size());

		StaticDArray<VkDescriptorBufferInfo, VulkanBindGroupCold::MaxBufferEntries> descriptorBufferInfo;
		descriptorBufferInfo.resize(Cold->Buffers.size());

		StaticDArray<VkDescriptorImageInfo, VulkanBindGroupCold::MaxTextureEntries> descriptorImageInfo;
		descriptorImageInfo.resize(Cold->Textures.size());

		for (int i = 0; i < Cold->Buffers.size(); i++)
		{
			VulkanBufferHot* buffer = rm->GetBufferHot(Cold->Buffers[i].buffer);

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
			VulkanTexture* texture = rm->GetTexture(Cold->Textures[i].texture);

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

			VkImageLayout layout;

			if (Cold->Textures[i].desiredLayout == TextureLayout::UNDEFINED)
			{
				layout = texture->ImageLayout;
			}
			else
			{
				layout = VkUtils::TextureLayoutToVkImageLayout(Cold->Textures[i].desiredLayout);
			}

			descriptorImageInfo[i] =
			{
				.sampler = texture->Sampler,
				.imageView = texture->ImageView,
				.imageLayout = layout,
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

		vkUpdateDescriptorSets(device->Get(), (uint32_t)writeDescriptorSet.size(), writeDescriptorSet.data(), 0, nullptr);
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
