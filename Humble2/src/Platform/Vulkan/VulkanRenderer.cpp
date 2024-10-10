#include "VulkanRenderer.h"

#include "VulkanDevice.h"
#include "VulkanResourceManager.h"

#define VMA_IMPLEMENTATION
#include "vma\vk_mem_alloc.h"

namespace HBL2
{
	void VulkanRenderer::Initialize()
	{
		m_Device = (VulkanDevice*)Device::Instance;
		m_ResourceManager = (VulkanResourceManager*)ResourceManager::Instance;

		vkGetDeviceQueue(m_Device->Get(), m_Device->GetQueueFamilyIndices().graphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device->Get(), m_Device->GetQueueFamilyIndices().presentFamily.value(), 0, &m_PresentQueue);

		CreateAllocator();
		CreateSwapchain();
		CreateImageViews();
		CreateCommands();
	}

	void VulkanRenderer::BeginFrame()
	{
		// Wait until the GPU has finished rendering the last frame. Timeout of 1 second
		VK_VALIDATE(vkWaitForFences(m_Device->Get(), 1, &GetCurrentFrame().RenderFence, true, 1000000000), "vkWaitForFences");
		VK_VALIDATE(vkResetFences(m_Device->Get(), 1, &GetCurrentFrame().RenderFence), "vkResetFences");
		VK_VALIDATE(vkAcquireNextImageKHR(m_Device->Get(), m_SwapChain, 1000000000, GetCurrentFrame().PresentSemaphore, nullptr, &m_SwapchainImageIndex), "vkAcquireNextImageKHR");
	}

	void VulkanRenderer::EndFrame()
	{
		// Prepare the submission to the queue.
		// We want to wait on the PresentSemaphore, as that semaphore is signaled when the swapchain is ready.
		// We will signal the RenderSemaphore, to signal that rendering has finished.

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submit =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &GetCurrentFrame().PresentSemaphore,
			.pWaitDstStageMask = &waitStage,
			.commandBufferCount = 1,
			.pCommandBuffers = &GetCurrentFrame().MainCommandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &GetCurrentFrame().RenderSemaphore,
		};

		// Submit command buffer to the queue and execute it.
		// RenderFence will now block until the graphic commands finish execution
		VK_VALIDATE(vkQueueSubmit(m_GraphicsQueue, 1, &submit, GetCurrentFrame().RenderFence), "vkQueueSubmit");
	}

	void VulkanRenderer::Present()
	{
		// This will put the image we just rendered into the visible window.
		// We want to wait on the renderSemaphore for that, as it's necessary that drawing commands have finished before the image is displayed to the user
		
		VkPresentInfoKHR presentInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &GetCurrentFrame().RenderSemaphore,
			.swapchainCount = 1,
			.pSwapchains = &m_SwapChain,
			.pImageIndices = &m_SwapchainImageIndex,
		};		

		VK_VALIDATE(vkQueuePresentKHR(m_PresentQueue, &presentInfo), "vkQueuePresentKHR");

		m_FrameNumber++;
	}

	void VulkanRenderer::Clean()
	{
		vkDeviceWaitIdle(m_Device->Get());

		m_MainDeletionQueue.flush();

		vkDestroySwapchainKHR(m_Device->Get(), m_SwapChain, nullptr);

		VulkanTexture* vkTexture = m_ResourceManager->GetTexture(m_DepthImage);

		vkDestroyImageView(m_Device->Get(), vkTexture->ImageView, nullptr);
		vmaDestroyImage(m_Allocator, vkTexture->Image, vkTexture->Allocation);

		vmaDestroyAllocator(m_Allocator);
	}

	CommandBuffer* VulkanRenderer::BeginCommandRecording(CommandBufferType type)
	{
		// call vkAcquireNextImageKHR here???
		return nullptr;
	}

	void VulkanRenderer::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
	{
	}

	void VulkanRenderer::CreateAllocator()
	{
		VmaAllocatorCreateInfo allocatorInfo = 
		{
			.physicalDevice = m_Device->GetPhysicalDevice(),
			.device = m_Device->Get(),
			.instance = m_Device->GetInstance(),
		};

		VK_VALIDATE(vmaCreateAllocator(&allocatorInfo, &m_Allocator), "vmaCreateAllocator");
	}

	void VulkanRenderer::CreateSwapchain()
	{
		const SwapChainSupportDetails& swapChainSupport = m_Device->GetSwapChainSupportDetails();

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

		uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;

		if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.Capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Device->GetSurface();
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		const QueueFamilyIndices& indices = m_Device->GetQueueFamilyIndices();
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VK_VALIDATE(vkCreateSwapchainKHR(m_Device->Get(), &createInfo, nullptr, &m_SwapChain), "vkCreateSwapchainKHR");

		// Retrieving the swap chain images.
		vkGetSwapchainImagesKHR(m_Device->Get(), m_SwapChain, &imageCount, nullptr);
		m_SwapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device->Get(), m_SwapChain, &imageCount, m_SwapChainImages.data());

		m_SwapChainImageFormat = surfaceFormat.format;
		m_SwapChainExtent = extent;

		m_DepthFormat = VK_FORMAT_D32_SFLOAT;

		m_DepthImage = ResourceManager::Instance->CreateTexture({
			.debugName = "vk-swapchain-depth-image",
			.dimensions = { m_SwapChainExtent.width, m_SwapChainExtent.height, 1 },
			.format = Format::D32_FLOAT,
			.internalFormat = Format::D32_FLOAT,
			.usage = TextureUsage::DEPTH_STENCIL,
		});
	}

	void VulkanRenderer::CreateImageViews()
	{
		m_SwapChainImageViews.resize(m_SwapChainImages.size());

		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_SwapChainImages[i];

			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_SwapChainImageFormat;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			VK_VALIDATE(vkCreateImageView(m_Device->Get(), &createInfo, nullptr, &m_SwapChainImageViews[i]), "vkCreateImageView");
		}
	}

	void VulkanRenderer::CreateCommands()
	{
		const QueueFamilyIndices& indices = m_Device->GetQueueFamilyIndices();

		// Create a command pool for commands submitted to the graphics queue. We also want the pool to allow for resetting of individual command buffers
		VkCommandPoolCreateInfo commandPoolInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = indices.graphicsFamily.value(),
		};		

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			VK_VALIDATE(vkCreateCommandPool(m_Device->Get(), &commandPoolInfo, nullptr, &m_Frames[i].CommandPool), "vkCreateCommandPool");

			// Allocate the default and ImGui command buffer that we will use for regular and imgui rendering.
			VkCommandBuffer commandBuffers[2];
			VkCommandBufferAllocateInfo cmdAllocInfo =
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.pNext = nullptr,
				.commandPool = m_Frames[i].CommandPool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = 2,
			};			

			VK_VALIDATE(vkAllocateCommandBuffers(m_Device->Get(), &cmdAllocInfo, commandBuffers), "vkAllocateCommandBuffers");

			m_Frames[i].MainCommandBuffer = commandBuffers[0];
			m_Frames[i].ImGuiCommandBuffer = commandBuffers[1];

			m_MainDeletionQueue.push_function([=]()
			{
				vkDestroyCommandPool(m_Device->Get(), m_Frames[i].CommandPool, nullptr);
			});
		}

		VkCommandPoolCreateInfo uploadCommandPoolInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = indices.graphicsFamily.value(),
		};		

		VK_VALIDATE(vkCreateCommandPool(m_Device->Get(), &uploadCommandPoolInfo, nullptr, &m_UploadContext.CommandPool), "vkCreateCommandPool");

		m_MainDeletionQueue.push_function([=]()
		{
			vkDestroyCommandPool(m_Device->Get(), m_UploadContext.CommandPool, nullptr);
		});

		// Allocate the default command buffer that we will use for the instant commands
		VkCommandBufferAllocateInfo cmdAllocInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = m_UploadContext.CommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};		

		VK_VALIDATE(vkAllocateCommandBuffers(m_Device->Get(), &cmdAllocInfo, &m_UploadContext.CommandBuffer), "vkAllocateCommandBuffers");
	}

	// Helper

	VkPresentModeKHR VulkanRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(Window::Instance->GetHandle(), &width, &height);

			VkExtent2D actualExtent =
			{
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	VkSurfaceFormatKHR VulkanRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}
}
