#include "VulkanRenderer.h"

#include "VulkanDevice.h"
#include "VulkanResourceManager.h"

#define VMA_IMPLEMENTATION
#include "vma\vk_mem_alloc.h"

namespace HBL2
{
	void VulkanRenderer::Initialize()
	{
		m_GraphicsAPI = GraphicsAPI::VULKAN;

		m_Device = (VulkanDevice*)Device::Instance;
		m_ResourceManager = (VulkanResourceManager*)ResourceManager::Instance;

		vkGetDeviceQueue(m_Device->Get(), m_Device->GetQueueFamilyIndices().graphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device->Get(), m_Device->GetQueueFamilyIndices().presentFamily.value(), 0, &m_PresentQueue);

		CreateAllocator();

		CreateSwapchain();
		CreateImageViews();
		CreateCommands();
		CreateRenderPass();
		CreateFrameBuffers();
		CreateSyncStructures();
	}

	void VulkanRenderer::BeginFrame()
	{
		// Wait until the GPU has finished rendering the last frame. Timeout of 1 second
		VK_VALIDATE(vkWaitForFences(m_Device->Get(), 1, &GetCurrentFrame().InFlightFence, true, 1000000000), "vkWaitForFences");
		VK_VALIDATE(vkResetFences(m_Device->Get(), 1, &GetCurrentFrame().InFlightFence), "vkResetFences");
		VK_VALIDATE(vkAcquireNextImageKHR(m_Device->Get(), m_SwapChain, 1000000000, GetCurrentFrame().ImageAvailableSemaphore, nullptr, &m_SwapchainImageIndex), "vkAcquireNextImageKHR");

		// Now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
		VK_VALIDATE(vkResetCommandBuffer(GetCurrentFrame().MainCommandBuffer, 0), "vkResetCommandBuffer");

		VkCommandBuffer cmd = GetCurrentFrame().MainCommandBuffer;

		// Begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBeginInfo.pNext = nullptr;
		cmdBeginInfo.pInheritanceInfo = nullptr;
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_VALIDATE(vkBeginCommandBuffer(cmd, &cmdBeginInfo), "vkBeginCommandBuffer");

		VkClearValue clearValue;
		float flash = std::abs(std::sin(m_FrameNumber / 480.f));
		clearValue.color = { { 1.0f, flash, 0.f, 1.0f } };
		VkClearValue depthClear;
		depthClear.depthStencil.depth = 1.f;
		VkClearValue clearValues[] = { clearValue, depthClear };

		VkRenderPassBeginInfo rpInfo = {};
		rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpInfo.pNext = nullptr;
		rpInfo.renderPass = m_RenderPass;
		rpInfo.renderArea.offset.x = 0;
		rpInfo.renderArea.offset.y = 0;
		rpInfo.renderArea.extent = m_SwapChainExtent;
		rpInfo.framebuffer = m_FrameBuffers[m_SwapchainImageIndex];
		rpInfo.clearValueCount = 2;
		rpInfo.pClearValues = &clearValues[0];

		vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		// DrawObjects(cmd, m_Renderables.data(), m_Renderables.size());

		vkCmdEndRenderPass(cmd);
		VK_VALIDATE(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");
	}

	void VulkanRenderer::EndFrame()
	{
		// Prepare the submission to the queue.
		// We want to wait on the PresentSemaphore, as that semaphore is signaled when the swapchain is ready.
		// We will signal the RenderSemaphore, to signal that rendering has finished.
		
		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &GetCurrentFrame().ImageAvailableSemaphore,
			.pWaitDstStageMask = &waitStage,
			.commandBufferCount = 1,
			.pCommandBuffers = &GetCurrentFrame().MainCommandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &GetCurrentFrame().MainRenderFinishedSemaphore,
		};

		// Submit command buffer to the queue and execute it.
		// RenderFence will now block until the graphic commands finish execution
		VK_VALIDATE(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), "vkQueueSubmit");
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
			.pWaitSemaphores = &GetCurrentFrame().ImGuiRenderFinishedSemaphore,
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

		vkDestroyRenderPass(m_Device->Get(), m_RenderPass, nullptr);

		VulkanTexture* vkTexture = m_ResourceManager->GetTexture(m_DepthImage);

		for (int i = 0; i < m_FrameBuffers.size(); i++)
		{
			vkDestroyFramebuffer(m_Device->Get(), m_FrameBuffers[i], nullptr);
			vkDestroyImageView(m_Device->Get(), m_SwapChainImageViews[i], nullptr);
		}

		vkDestroyImageView(m_Device->Get(), vkTexture->ImageView, nullptr);
		vmaDestroyImage(m_Allocator, vkTexture->Image, vkTexture->Allocation);

		vmaDestroyAllocator(m_Allocator);
	}

	CommandBuffer* VulkanRenderer::BeginCommandRecording(CommandBufferType type)
	{
		// call vkAcquireNextImageKHR here???

		VkCommandBuffer cmd = VK_NULL_HANDLE;

		switch (type)
		{
		case HBL2::CommandBufferType::MAIN:
			cmd = GetCurrentFrame().MainCommandBuffer;
			break;
		case HBL2::CommandBufferType::OFFSCREEN:
			cmd = GetCurrentFrame().MainCommandBuffer;
			break;
		case HBL2::CommandBufferType::UI:
			cmd = GetCurrentFrame().ImGuiCommandBuffer;
			break;
		default:
			break;
		}

		// Now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
		VK_VALIDATE(vkResetCommandBuffer(cmd, 0), "vkResetCommandBuffer");

		// Begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBeginInfo.pNext = nullptr;
		cmdBeginInfo.pInheritanceInfo = nullptr;
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_VALIDATE(vkBeginCommandBuffer(cmd, &cmdBeginInfo), "vkBeginCommandBuffer");

		switch (type)
		{
		case HBL2::CommandBufferType::MAIN:
			return &m_MainCommandBuffers[m_FrameNumber % FRAME_OVERLAP];
		case HBL2::CommandBufferType::UI:
			return &m_ImGuiCommandBuffers[m_FrameNumber % FRAME_OVERLAP];
		case HBL2::CommandBufferType::OFFSCREEN:
			return &m_OffScreenCommandBuffer;
		}

		return nullptr;
	}

	void VulkanRenderer::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
	{
		VkCommandBuffer cmd = m_UploadContext.CommandBuffer;

		// begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that.
		VkCommandBufferBeginInfo cmdBeginInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr,
		};		

		VK_VALIDATE(vkBeginCommandBuffer(cmd, &cmdBeginInfo), "vkBeginCommandBuffer");

		// excecute function
		function(cmd);

		VK_VALIDATE(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");

		VkSubmitInfo submit =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &cmd,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr,
		};

		// Submit command buffer to the queue and execute it.
		// UploadFence will now block until the graphic commands finish execution
		VK_VALIDATE(vkQueueSubmit(m_GraphicsQueue, 1, &submit, m_UploadContext.UploadFence), "vkQueueSubmit");

		vkWaitForFences(m_Device->Get(), 1, &m_UploadContext.UploadFence, true, 9999999999);
		vkResetFences(m_Device->Get(), 1, &m_UploadContext.UploadFence);

		// Reset the command buffers inside the command pool
		vkResetCommandPool(m_Device->Get(), m_UploadContext.CommandPool, 0);
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

		m_DepthImage = m_ResourceManager->CreateTexture({
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

			m_MainCommandBuffers[i] = VulkanCommandBuffer(CommandBufferType::MAIN, m_Frames[i].MainCommandBuffer);
			m_ImGuiCommandBuffers[i] = VulkanCommandBuffer(CommandBufferType::UI, m_Frames[i].ImGuiCommandBuffer);
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

		m_OffScreenCommandBuffer = VulkanCommandBuffer(CommandBufferType::OFFSCREEN, m_UploadContext.CommandBuffer);
	}

	void VulkanRenderer::CreateRenderPass()
	{
		// the renderpass will use this color attachment.
		VkAttachmentDescription colorAttachment = {};
		//the attachment will have the format needed by the swapchain
		colorAttachment.format = m_SwapChainImageFormat;
		//1 sample, we won't be doing MSAA
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		// we Clear when this attachment is loaded
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		// we keep the attachment stored when the renderpass ends
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		//we don't care about stencil
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		//we don't know or care about the starting layout of the attachment
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		//after the renderpass ends, the image has to be on a layout ready for display
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef = {};
		//attachment number will index into the pAttachments array in the parent renderpass itself
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment = {};
		// Depth attachment
		depthAttachment.flags = 0;
		depthAttachment.format = m_DepthFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		//we are going to create 1 subpass, which is the minimum you can do
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		//hook the depth attachment into the subpass
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		//array of 2 attachments, one for the color, and other for depth
		VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

		//2 attachments from said array
		renderPassInfo.attachmentCount = 2;
		renderPassInfo.pAttachments = &attachments[0];
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency depthDependency = {};
		depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		depthDependency.dstSubpass = 0;
		depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depthDependency.srcAccessMask = 0;
		depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency dependencies[2] = { dependency, depthDependency };

		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = &dependencies[0];

		VkResult result = vkCreateRenderPass(m_Device->Get(), &renderPassInfo, nullptr, &m_RenderPass);
		assert(result == VK_SUCCESS);
	}

	void VulkanRenderer::CreateFrameBuffers()
	{
		VulkanTexture* vulkanDepthImage = m_ResourceManager->GetTexture(m_DepthImage);

		// Create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering.
		VkFramebufferCreateInfo frameBufferInfo = {};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.pNext = nullptr;

		frameBufferInfo.renderPass = m_RenderPass;
		frameBufferInfo.attachmentCount = 1;
		frameBufferInfo.width = m_SwapChainExtent.width;
		frameBufferInfo.height = m_SwapChainExtent.height;
		frameBufferInfo.layers = 1;

		// Grab how many images we have in the swapchain.
		const uint32_t swapChainImageCount = m_SwapChainImages.size();
		m_FrameBuffers = std::vector<VkFramebuffer>(swapChainImageCount);

		// Create framebuffers for each of the swapchain image views.
		for (int i = 0; i < swapChainImageCount; i++)
		{
			VkImageView attachments[2];
			attachments[0] = m_SwapChainImageViews[i];
			attachments[1] = vulkanDepthImage->ImageView;

			frameBufferInfo.pAttachments = attachments;
			frameBufferInfo.attachmentCount = 2;

			VkResult result = vkCreateFramebuffer(m_Device->Get(), &frameBufferInfo, nullptr, &m_FrameBuffers[i]);
			assert(result == VK_SUCCESS);
		}
	}

	void VulkanRenderer::CreateSyncStructures()
	{
		// Create synchronization structures.
		VkFenceCreateInfo fenceCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};
		

		// For the semaphores we don't need any flags.
		VkSemaphoreCreateInfo semaphoreCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
		};

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			VK_VALIDATE(vkCreateFence(m_Device->Get(), &fenceCreateInfo, nullptr, &m_Frames[i].InFlightFence), "vkCreateFence");

			//enqueue the destruction of the fence
			m_MainDeletionQueue.push_function([=]()
			{
				vkDestroyFence(m_Device->Get(), m_Frames[i].InFlightFence, nullptr);
			});

			VK_VALIDATE(vkCreateSemaphore(m_Device->Get(), &semaphoreCreateInfo, nullptr, &m_Frames[i].ImageAvailableSemaphore), "vkCreateSemaphore");
			VK_VALIDATE(vkCreateSemaphore(m_Device->Get(), &semaphoreCreateInfo, nullptr, &m_Frames[i].MainRenderFinishedSemaphore), "vkCreateSemaphore");
			VK_VALIDATE(vkCreateSemaphore(m_Device->Get(), &semaphoreCreateInfo, nullptr, &m_Frames[i].ImGuiRenderFinishedSemaphore), "vkCreateSemaphore");

			//enqueue the destruction of semaphores
			m_MainDeletionQueue.push_function([=]()
			{
				vkDestroySemaphore(m_Device->Get(), m_Frames[i].ImageAvailableSemaphore, nullptr);
				vkDestroySemaphore(m_Device->Get(), m_Frames[i].MainRenderFinishedSemaphore, nullptr);
				vkDestroySemaphore(m_Device->Get(), m_Frames[i].ImGuiRenderFinishedSemaphore, nullptr);
			});
		}

		VkFenceCreateInfo uploadFenceCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
		};

		VK_VALIDATE(vkCreateFence(m_Device->Get(), &uploadFenceCreateInfo, nullptr, &m_UploadContext.UploadFence), "vkCreateFence");

		m_MainDeletionQueue.push_function([=]()
		{
			vkDestroyFence(m_Device->Get(), m_UploadContext.UploadFence, nullptr);
		});
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
			// Chose VK_FORMAT_B8G8R8A8_UNORM over VK_FORMAT_B8G8R8A8_SRGB since ingui was rendered with washed out colors with SRGB, since it uses UNORM internally.
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}
}
