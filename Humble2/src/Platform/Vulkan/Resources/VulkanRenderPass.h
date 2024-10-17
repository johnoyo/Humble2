#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"
#include "Platform\Vulkan\VulkanRenderer.h"

#include "Platform\Vulkan\VulkanCommon.h"

#include "VulkanRenderPassLayout.h"

#include <string>
#include <stdint.h>

namespace HBL2
{
	struct VulkanRenderPass
	{
		VulkanRenderPass() = default;
		VulkanRenderPass(const RenderPassDescriptor&& desc);

		const char* DebugName = "";
		VkRenderPass RenderPass = VK_NULL_HANDLE;

	private:
		VkAttachmentLoadOp LoadOperationToVkAttachmentLoadOp(LoadOperation loadOperation)
		{
			switch (loadOperation)
			{
			case HBL2::LoadOperation::CLEAR:
				return VK_ATTACHMENT_LOAD_OP_CLEAR;
			case HBL2::LoadOperation::LOAD:
				return VK_ATTACHMENT_LOAD_OP_LOAD;
			case HBL2::LoadOperation::DONT_CARE:
				return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			}

			return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
		}

		VkAttachmentStoreOp StoreOperationVkAttachmentStoreOp(StoreOperation storeOperation)
		{
			switch (storeOperation)
			{
			case HBL2::StoreOperation::STORE:
				return VK_ATTACHMENT_STORE_OP_STORE;
			case HBL2::StoreOperation::DONT_CARE:
				return VK_ATTACHMENT_STORE_OP_DONT_CARE;
			}

			return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
		}

		VkImageLayout TextureLayoutToVkImageLayout(TextureLayout textureLayout)
		{
			switch (textureLayout)
			{
			case HBL2::TextureLayout::UNDEFINED:
				return VK_IMAGE_LAYOUT_UNDEFINED;
			case HBL2::TextureLayout::COPY_SRC:
				return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			case HBL2::TextureLayout::COPY_DST:
				return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			case HBL2::TextureLayout::RENDER_ATTACHMENT:
				return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			case HBL2::TextureLayout::DEPTH_STENCIL:
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			case HBL2::TextureLayout::PRESENT:
				return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			}

			return VK_IMAGE_LAYOUT_MAX_ENUM;
		}

		VkFormat FormatToVkFormat(Format format)
		{
			switch (format)
			{
			case Format::D32_FLOAT:
				return VK_FORMAT_D32_SFLOAT;
			}

			return VK_FORMAT_MAX_ENUM;
		}
	};
}