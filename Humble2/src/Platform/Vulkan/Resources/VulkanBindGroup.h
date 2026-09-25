#pragma once

#include "Base.h"
#include "Resources/RefCounted.h"
#include "Resources/TypeDescriptors.h"

#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanRenderer.h"

#include "Platform/Vulkan/VulkanCommon.h"

namespace HBL2
{
	struct VulkanBindGroupHot
	{
		VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
	};

	struct VulkanBindGroupCold : public RefCounted
	{
		static constexpr uint32_t MaxTextureEntries = 6;
		static constexpr uint32_t MaxBufferEntries = 6;

		struct TextureEntry
		{
			Handle<Texture> texture;
			TextureLayout desiredLayout = TextureLayout::UNDEFINED;
			Handle<ReimportDependency> dependency;
		};
		struct BufferEntry
		{
			Handle<Buffer> buffer;
			uint32_t byteOffset = 0;
			uint32_t range = 0;
		};

		const char* DebugName = "";
		RefHandle<BindGroupLayout> BindGroupLayout;
		StaticDArray<TextureEntry, MaxTextureEntries> Textures;
		StaticDArray<BufferEntry, MaxBufferEntries> Buffers;

		void Destroy();
	};

	// Helper struct for centralised operations on hot and cold data.
	// NOTE: Use with SplitPool::Get to retrieve the Hot and Cold data from the pool.
	struct VulkanBindGroup
	{
		VulkanBindGroup() = default;

		bool IsValid() const;

		void Initialize(Handle<BindGroup> self, const BindGroupDescriptor&& desc);
		void Update();
		void Destroy();

		VulkanBindGroupHot* Hot = nullptr;
		VulkanBindGroupCold* Cold = nullptr;
	};
}
