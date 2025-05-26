#pragma once

#include <Base.h>
#include <Renderer\Enums.h>

#include <Utilities/Collections/BitFlags.h>

#include "vulkan\vulkan.h"
#include "vma\vk_mem_alloc.h"

#include <string>
#include <format>

#ifdef DEBUG
	#define VK_VALIDATE(result, vkFunction) HBL2_CORE_ASSERT(result == VK_SUCCESS, std::format("Vulkan function: {}, failed!", vkFunction));
#elif RELEASE
	#define VK_VALIDATE(result, vkFunction) HBL2_CORE_ASSERT(result == VK_SUCCESS, std::format("Vulkan function: {}, failed!", vkFunction));
#else
	#define VK_VALIDATE(result, vkFunction) result
#endif

namespace HBL2
{
	namespace VkUtils
	{
		VkFormat VertexFormatToVkFormat(VertexFormat vertexFormat);

		VkBlendOp BlendOperationToVkBlendOp(BlendOperation blendOperation);

		VkBlendFactor BlendFactorToVkBlendFactor(BlendFactor blendFactor);

		VkCompareOp CompareToVkCompareOp(Compare compare);

		VkFilter FilterToVkFilter(Filter filter);

		VkSamplerAddressMode WrapToVkSamplerAddressMode(Wrap wrap);

		VkPrimitiveTopology TopologyToVkPrimitiveTopology(Topology topology);

		VkPolygonMode PolygonModeToVkPolygonMode(PolygonMode polygonMode);

		VkCullModeFlags CullModeToVkCullModeFlags(CullMode cullMode);

		VkFrontFace FrontFaceToVkFrontFace(FrontFace frontFace);

		VkImageType TextureTypeToVkImageType(TextureType textureType);

		VkImageViewType TextureTypeToVkVkImageViewType(TextureType textureType);

		VkFormat FormatToVkFormat(Format format);

		VkImageAspectFlags TextureAspectToVkImageAspectFlags(TextureAspect textureAspect);

		VkImageUsageFlags TextureUsageToVkImageUsageFlags(TextureUsage textureUsage);

		VkImageUsageFlags TextureUsageFlagToVkImageUsageFlags(BitFlags<TextureUsage> textureUsageFlags);

		VkImageLayout TextureLayoutToVkImageLayout(TextureLayout textureLayout);

		VkAttachmentLoadOp LoadOperationToVkAttachmentLoadOp(LoadOperation loadOperation);

		VkAttachmentStoreOp StoreOperationVkAttachmentStoreOp(StoreOperation storeOperation);

		VmaMemoryUsage MemoryUsageToVmaMemoryUsage(MemoryUsage memoryUsage);

		VkBufferUsageFlags BufferUsageToVkBufferUsageFlags(BufferUsage bufferUsage);

		VkPipelineStageFlags PipelineStageToVkPipelineStageFlags(PipelineStage pipelineStage);

		VkPipelineStageFlags CurrentTextureLayoutToVkPipelineStageFlags(TextureLayout currentLayout);

		VkPipelineStageFlags NewTextureLayoutToVkPipelineStageFlags(TextureLayout newLayout);

		VkAccessFlags CurrentTextureLayoutToVkAccessFlags(TextureLayout currentLayout);

		VkAccessFlags NewTextureLayoutToVkAccessFlags(TextureLayout newLayout);
	}
}
