#include "VulkanRenderer.h"

#include "VulkanDevice.h"
#include "VulkanResourceManager.h"
#include <imgui_impl_vulkan.h>

#include "Utilities\TextureUtilities.h"
#include "Utilities\ShaderUtilities.h"

namespace HBL2
{
	thread_local UploadContext VulkanRenderer::s_UploadContext;

	void VulkanRenderer::PreInitialize()
	{
		m_GraphicsAPI = GraphicsAPI::VULKAN;

		m_Device = (VulkanDevice*)Device::Instance;
		m_ResourceManager = (VulkanResourceManager*)ResourceManager::Instance;

		vkGetDeviceQueue(m_Device->Get(), m_Device->GetQueueFamilyIndices().graphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device->Get(), m_Device->GetQueueFamilyIndices().presentFamily.value(), 0, &m_PresentQueue);

		CreateAllocator();

		CreateSwapchain();
		CreateImageViews();
		CreateSyncStructures();
		CreateCommands();
		CreateRenderPass();
		CreateFrameBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
	}

	void VulkanRenderer::PostInitialize()
	{
		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			m_Frames[i].GlobalPresentBindings = m_ResourceManager->CreateBindGroup({
				.debugName = "global-present-bind-group",
				.layout = GetGlobalPresentBindingsLayout(),
				.textures = { MainColorTexture },
			});
		}

		EventDispatcher::Get().Register<FramebufferSizeEvent>([this](const FramebufferSizeEvent& e)
		{
			m_Resize = true;
			m_NewSize = { e.Width, e.Height };
		});
	}

	void VulkanRenderer::BeginFrame()
	{
		if (m_Resize)
		{
			for (uint32_t i = 0; i < FRAME_OVERLAP; ++i)
			{
				VK_VALIDATE(vkWaitForFences(m_Device->Get(), 1, &m_Frames[i].InFlightFence, VK_TRUE, UINT64_MAX), "vkWaitForFences");
			}

			Resize(m_NewSize.x, m_NewSize.y);
			m_Resize = false;
		}

		// Wait until the GPU has finished rendering the last frame. Timeout of 1 second
		VK_VALIDATE(vkWaitForFences(m_Device->Get(), 1, &GetCurrentFrame().InFlightFence, VK_TRUE, UINT64_MAX), "vkWaitForFences");
		
		// Flush any pending deletions that occured in the frames before the current one.
		m_ResourceManager->Flush(m_FrameNumber.load());

		VK_VALIDATE(vkResetFences(m_Device->Get(), 1, &GetCurrentFrame().InFlightFence), "vkResetFences");
		VK_VALIDATE(vkAcquireNextImageKHR(m_Device->Get(), m_SwapChain, UINT64_MAX, GetCurrentFrame().ImageAvailableSemaphore, nullptr, &m_SwapchainImageIndex), "vkAcquireNextImageKHR");

		m_Stats.Reset();
	}

	void VulkanRenderer::EndFrame()
	{
		// In the 1st frame there is no active scene, the rendering systems do not get called.
		// So the semaphores that renderer waits on are never updated so there are validation errors.
		// To prevent this, we have a stub pass that does no rendering but follows the flow that the renderer expects.
		if (!Context::ActiveScene.IsValid())
		{
			StubRenderPass();
		}
	}

	void VulkanRenderer::Present()
	{
		// This will put the image we just rendered into the visible window.
		// We want to wait on the ImGuiRenderFinishedSemaphore for that, as it's necessary that drawing commands have finished before the image is displayed to the user
		
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

		VkResult result;

		{
			std::lock_guard<std::mutex> lock(m_GraphicsQueueSubmitMutex);
			result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
		}

		m_FrameNumber++;

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			m_Resize = true;
		}
		else
		{
			VK_VALIDATE(result, "vkQueuePresentKHR");
		}
	}

	void VulkanRenderer::Clean()
	{
		VK_VALIDATE(vkDeviceWaitIdle(m_Device->Get()), "vkDeviceWaitIdle");

		m_MainDeletionQueue.Flush();

		m_ResourceManager->DeleteTexture(IntermediateColorTexture);
		m_ResourceManager->DeleteTexture(MainColorTexture);
		m_ResourceManager->DeleteTexture(MainDepthTexture);
		m_ResourceManager->DeleteTexture(ShadowAtlasTexture);

		vkDestroySwapchainKHR(m_Device->Get(), m_SwapChain, nullptr);

		VkRenderPass renderPass = m_ResourceManager->GetRenderPass(m_RenderPass)->RenderPass;
		vkDestroyRenderPass(m_Device->Get(), renderPass, nullptr);

		VkRenderPass renderingRenderPass = m_ResourceManager->GetRenderPass(m_RenderingRenderPass)->RenderPass;
		vkDestroyRenderPass(m_Device->Get(), renderingRenderPass, nullptr);

		m_ResourceManager->DeleteBindGroupLayout(m_ShadowBindingsLayout);
		m_ResourceManager->DeleteBindGroupLayout(m_GlobalBindingsLayout2D);
		m_ResourceManager->DeleteBindGroupLayout(m_GlobalBindingsLayout3D);
		m_ResourceManager->DeleteBindGroupLayout(m_GlobalPresentBindingsLayout);
		m_ResourceManager->DeleteBindGroupLayout(m_DebugBindingsLayout);

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			VulkanBindGroup* shadowBindGroup = m_ResourceManager->GetBindGroup(m_Frames[i].ShadowBindings);

			for (auto& bufferEntry : shadowBindGroup->Buffers)
			{
				m_ResourceManager->DeleteBuffer(bufferEntry.buffer);
			}

			m_ResourceManager->DeleteBindGroup(m_Frames[i].ShadowBindings);
			m_ResourceManager->DeleteBindGroup(m_Frames[i].GlobalBindings2D);
			m_ResourceManager->DeleteBindGroup(m_Frames[i].GlobalBindings3D);
			m_ResourceManager->DeleteBindGroup(m_Frames[i].GlobalPresentBindings);
			m_ResourceManager->DeleteBindGroup(m_Frames[i].DebugBindings);
		}

		for (int i = 0; i < m_FrameBuffers.size(); i++)
		{
			VulkanFrameBuffer* vkFrameBuffer = m_ResourceManager->GetFrameBuffer(m_FrameBuffers[i]);
			vkDestroyFramebuffer(m_Device->Get(), vkFrameBuffer->FrameBuffer, nullptr);
			vkDestroyImageView(m_Device->Get(), m_SwapChainImageViews[i], nullptr);
		}

		for (int i = 0; i < m_SwapChainColorTextures.size(); i++)
		{
			m_ResourceManager->m_TexturePool.Remove(m_SwapChainColorTextures[i]);
		}

		m_ResourceManager->DeleteTexture(m_DepthImage);

		TempUniformRingBuffer->Free();

		m_ResourceManager->FlushAll();

		vmaDestroyAllocator(m_Allocator);
	}

	void VulkanRenderer::StubRenderPass()
	{
		CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN);

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(GetMainRenderPass(), GetMainFrameBuffer());
		commandBuffer->EndRenderPass(*passRenderer);

		commandBuffer->EndCommandRecording();
		commandBuffer->Submit();
	}

	CommandBuffer* VulkanRenderer::BeginCommandRecording(CommandBufferType type)
	{
		VkCommandBuffer cmd = VK_NULL_HANDLE;
		VulkanCommandBuffer* vkCmdObj = nullptr;

		switch (type)
		{
		case HBL2::CommandBufferType::MAIN:
			cmd = GetCurrentFrame().MainCommandBuffer;
			vkCmdObj = &m_MainCommandBuffers[m_FrameNumber.load() % FRAME_OVERLAP];
			break;
		case HBL2::CommandBufferType::UI:
			cmd = GetCurrentFrame().ImGuiCommandBuffer;
			vkCmdObj = &m_ImGuiCommandBuffers[m_FrameNumber.load() % FRAME_OVERLAP];
			break;
		default:
			break;
		}

		// Now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
		VK_VALIDATE(vkResetCommandBuffer(cmd, 0), "vkResetCommandBuffer");

		// Begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr,
		};

		VK_VALIDATE(vkBeginCommandBuffer(cmd, &cmdBeginInfo), "vkBeginCommandBuffer");

		return vkCmdObj;
	}

	void* VulkanRenderer::GetColorAttachment()
	{
		Handle<BindGroup> presentBindGroupHandle = m_Frames[m_FrameNumber.load() % FRAME_OVERLAP].GlobalPresentBindings;
		VulkanBindGroup* vkPresentBindGroup = m_ResourceManager->GetBindGroup(presentBindGroupHandle);
		m_ColorAttachmentID = vkPresentBindGroup->DescriptorSet;

		return m_ColorAttachmentID;
	}

	void VulkanRenderer::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
	{
		if (s_UploadContext.CommandBuffer == VK_NULL_HANDLE)
		{
			CreateUploadContextCommands();
		}

		VkCommandBuffer cmd = s_UploadContext.CommandBuffer;

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
		{
			std::lock_guard<std::mutex> lock(m_GraphicsQueueSubmitMutex);
			VK_VALIDATE(vkQueueSubmit(m_GraphicsQueue, 1, &submit, s_UploadContext.UploadFence), "vkQueueSubmit");
		}

		vkWaitForFences(m_Device->Get(), 1, &s_UploadContext.UploadFence, true, UINT64_MAX);
		vkResetFences(m_Device->Get(), 1, &s_UploadContext.UploadFence);

		// Reset the command buffers inside the command pool
		vkResetCommandPool(m_Device->Get(), s_UploadContext.CommandPool, 0);
	}

	void VulkanRenderer::Resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
		{
			return;
		}

		{
			std::lock_guard<std::mutex> lock(m_GraphicsQueueSubmitMutex);
			VK_VALIDATE(vkDeviceWaitIdle(m_Device->Get()), "vkDeviceWaitIdle");
		}

		// Store old swapchain.
		VkSwapchainKHR oldSwapchain = m_SwapChain;

		// Cleanup old swapchain resources.
		for (auto frameBuffer : m_FrameBuffers)
		{
			m_ResourceManager->DeleteFrameBuffer(frameBuffer);
		}

		m_FrameBuffers.clear();

		for (auto imageView : m_SwapChainImageViews)
		{
			vkDestroyImageView(m_Device->Get(), imageView, nullptr);
		}

		m_SwapChainImages.clear();
		m_SwapChainImageViews.clear();
		m_SwapChainColorTextures.clear();

		// Destroy the depth buffer.
		m_ResourceManager->DeleteTexture(m_DepthImage);

		// Destroy old offscreen textures.
		m_FrameNumber++;
		m_FrameNumber++;
		m_ResourceManager->DeleteTexture(IntermediateColorTexture);
		m_ResourceManager->DeleteTexture(MainColorTexture);
		m_ResourceManager->DeleteTexture(MainDepthTexture);
		m_FrameNumber--;
		m_FrameNumber--;

		m_Device->UpdateSwapChainSupportDetails();

		// Recreate swapchain and associated resources.
		CreateSwapchain(oldSwapchain);
		CreateImageViews();
		CreateFrameBuffers();

		// Destroy the old swapchain.
		vkDestroySwapchainKHR(m_Device->Get(), oldSwapchain, nullptr);

		// Recreate the offscreen texture (render target).
		IntermediateColorTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "intermediate-color-target",
			.dimensions = { width, height, 1 },
			.format = Format::RGBA16_FLOAT,
			.internalFormat = Format::RGBA16_FLOAT,
			.usage = { TextureUsage::RENDER_ATTACHMENT, TextureUsage::SAMPLED },
			.aspect = TextureAspect::COLOR,
			.sampler =
			{
				.filter = Filter::LINEAR,
				.wrap = Wrap::CLAMP_TO_EDGE,
			}
		});

		MainColorTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "viewport-color-target",
			.dimensions = { width, height, 1 },
			.format = Format::BGRA8_UNORM,
			.internalFormat = Format::BGRA8_UNORM,
			.usage = { TextureUsage::RENDER_ATTACHMENT, TextureUsage::SAMPLED },
			.aspect = TextureAspect::COLOR,
			.sampler =
			{
				.filter = Filter::LINEAR,
				.wrap = Wrap::CLAMP_TO_EDGE,
			}
		});

		MainDepthTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "viewport-depth-target",
			.dimensions = { width, height, 1 },
			.format = Format::D32_FLOAT,
			.internalFormat = Format::D32_FLOAT,
			.usage = TextureUsage::DEPTH_STENCIL,
			.aspect = TextureAspect::DEPTH,
			.createSampler = true,
			.sampler =
			{
				.filter = Filter::NEAREST,
				.wrap = Wrap::CLAMP_TO_EDGE,
			}
		});

		// Update descriptor sets (for the quad shader).
		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			m_ResourceManager->DeleteBindGroup(m_Frames[i].GlobalPresentBindings);

			m_Frames[i].GlobalPresentBindings = m_ResourceManager->CreateBindGroup({
				.debugName = "global-present-bind-group",
				.layout = Renderer::Instance->GetGlobalPresentBindingsLayout(),
				.textures = { MainColorTexture },  // Updated with new size
			});

			VulkanBindGroup* vkGlobalPresentBindings = m_ResourceManager->GetBindGroup(m_Frames[i].GlobalPresentBindings);
			vkGlobalPresentBindings->Update();
		}

		// Call on resize callbacks.
		for (auto&& [name, callback] : m_OnResizeCallbacks)
		{
			callback(width, height);
		}
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

	void VulkanRenderer::CreateSwapchain(VkSwapchainKHR oldSwapchain)
	{
		const SwapChainSupportDetails& swapChainSupport = m_Device->GetSwapChainSupportDetails();

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes, Window::Instance->GetSpec().VerticalSync);
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
		createInfo.oldSwapchain = oldSwapchain;

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
			.aspect = TextureAspect::DEPTH,
			.createSampler = false,
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

			VulkanTexture swapchainImage;
			swapchainImage.Image = m_SwapChainImages[i];
			swapchainImage.ImageView = m_SwapChainImageViews[i];
			swapchainImage.Extent = { m_SwapChainExtent.width, m_SwapChainExtent.height, 1 };
			swapchainImage.Aspect = VK_IMAGE_ASPECT_COLOR_BIT;
			m_SwapChainColorTextures.push_back(m_ResourceManager->m_TexturePool.Insert(std::move(swapchainImage)));
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
			m_MainDeletionQueue.PushFunction([=]()
			{
				vkDestroyFence(m_Device->Get(), m_Frames[i].InFlightFence, nullptr);
			});

			VK_VALIDATE(vkCreateSemaphore(m_Device->Get(), &semaphoreCreateInfo, nullptr, &m_Frames[i].ImageAvailableSemaphore), "vkCreateSemaphore");
			VK_VALIDATE(vkCreateSemaphore(m_Device->Get(), &semaphoreCreateInfo, nullptr, &m_Frames[i].MainRenderFinishedSemaphore), "vkCreateSemaphore");
			VK_VALIDATE(vkCreateSemaphore(m_Device->Get(), &semaphoreCreateInfo, nullptr, &m_Frames[i].ImGuiRenderFinishedSemaphore), "vkCreateSemaphore");

			//enqueue the destruction of semaphores
			m_MainDeletionQueue.PushFunction([=]()
			{
				vkDestroySemaphore(m_Device->Get(), m_Frames[i].ImageAvailableSemaphore, nullptr);
				vkDestroySemaphore(m_Device->Get(), m_Frames[i].MainRenderFinishedSemaphore, nullptr);
				vkDestroySemaphore(m_Device->Get(), m_Frames[i].ImGuiRenderFinishedSemaphore, nullptr);
			});
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

		VkCommandPoolCreateInfo secondaryCommandPoolInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueFamilyIndex = indices.graphicsFamily.value(),
		};

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			// Primary
			VK_VALIDATE(vkCreateCommandPool(m_Device->Get(), &commandPoolInfo, nullptr, &m_Frames[i].CommandPool), "vkCreateCommandPool");

			// Allocate the default and ImGui command buffers that we will use for regular and imgui rendering.
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

			m_MainDeletionQueue.PushFunction([=]()
			{
				vkDestroyCommandPool(m_Device->Get(), m_Frames[i].CommandPool, nullptr);
			});

			m_MainCommandBuffers[i] = VulkanCommandBuffer({
				.type = CommandBufferType::MAIN,
				.commandBuffer = m_Frames[i].MainCommandBuffer,
				.blockFence = VK_NULL_HANDLE,
				.waitSemaphore = m_Frames[i].ImageAvailableSemaphore,
				.signalSemaphore = m_Frames[i].MainRenderFinishedSemaphore,
			});

			m_ImGuiCommandBuffers[i] = VulkanCommandBuffer({
				.type = CommandBufferType::UI,
				.commandBuffer = m_Frames[i].ImGuiCommandBuffer,
				.blockFence = m_Frames[i].InFlightFence,
				.waitSemaphore = m_Frames[i].MainRenderFinishedSemaphore,
				.signalSemaphore = m_Frames[i].ImGuiRenderFinishedSemaphore,
			});
		}

		CreateUploadContextCommands();
	}

	void VulkanRenderer::CreateUploadContextCommands()
	{
		// Create the upload fence.
		VkFenceCreateInfo uploadFenceCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
		};

		VK_VALIDATE(vkCreateFence(m_Device->Get(), &uploadFenceCreateInfo, nullptr, &s_UploadContext.UploadFence), "vkCreateFence");
		
		const QueueFamilyIndices& indices = m_Device->GetQueueFamilyIndices();
		
		// Create command buffer pool.
		VkCommandPoolCreateInfo uploadCommandPoolInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = indices.graphicsFamily.value(),
		};		

		VK_VALIDATE(vkCreateCommandPool(m_Device->Get(), &uploadCommandPoolInfo, nullptr, &s_UploadContext.CommandPool), "vkCreateCommandPool");


		// Allocate the default command buffer that we will use for the instant commands
		VkCommandBufferAllocateInfo cmdAllocInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = s_UploadContext.CommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};		

		VK_VALIDATE(vkAllocateCommandBuffers(m_Device->Get(), &cmdAllocInfo, &s_UploadContext.CommandBuffer), "vkAllocateCommandBuffers");
		
		// Copy upload context to correctly capture it by value in the lambda.
		auto ctx = s_UploadContext;
		
		// Queue resources for deletion.
		{
			std::lock_guard<std::mutex> lock(m_DeletionQueueMutex);

			m_MainDeletionQueue.PushFunction([this, ctx]()
			{
				vkDestroyCommandPool(m_Device->Get(), ctx.CommandPool, nullptr);
				vkDestroyFence(m_Device->Get(), ctx.UploadFence, nullptr);
			});
		}
	}

	void VulkanRenderer::CreateRenderPass()
	{
		Handle<RenderPassLayout> renderPassLayout = ResourceManager::Instance->CreateRenderPassLayout({
			.debugName = "main-renderpass-layout",
			.depthTargetFormat = Format::D32_FLOAT,
			.subPasses = {
				{ .depthTarget = true, .colorTargets = 1, },
			},
		});

		m_RenderPass = ResourceManager::Instance->CreateRenderPass({
			.debugName = "main-renderpass",
			.layout = renderPassLayout,
			.depthTarget = {
				.loadOp = LoadOperation::CLEAR,
				.storeOp = StoreOperation::STORE,
				.stencilLoadOp = LoadOperation::DONT_CARE,
				.stencilStoreOp = StoreOperation::DONT_CARE,
				.prevUsage = TextureLayout::UNDEFINED,
				.nextUsage = TextureLayout::DEPTH_STENCIL,
			},
			.colorTargets = {
				{
					.loadOp = LoadOperation::CLEAR,
					.storeOp = StoreOperation::STORE,
					.prevUsage = TextureLayout::UNDEFINED,
					.nextUsage = TextureLayout::RENDER_ATTACHMENT,
				},
			},
		});

		m_RenderingRenderPass = ResourceManager::Instance->CreateRenderPass({
			.debugName = "main-renderpass",
			.layout = renderPassLayout,
			.depthTarget = {
				.loadOp = LoadOperation::CLEAR,
				.storeOp = StoreOperation::STORE,
				.stencilLoadOp = LoadOperation::DONT_CARE,
				.stencilStoreOp = StoreOperation::DONT_CARE,
				.prevUsage = TextureLayout::UNDEFINED,
				.nextUsage = TextureLayout::DEPTH_STENCIL,
			},
			.colorTargets = {
				{
					.format = Format::RGBA16_FLOAT,
					.loadOp = LoadOperation::CLEAR,
					.storeOp = StoreOperation::STORE,
					.prevUsage = TextureLayout::UNDEFINED,
					.nextUsage = TextureLayout::RENDER_ATTACHMENT,
				},
			},
		});
	}

	void VulkanRenderer::CreateFrameBuffers()
	{
		VulkanTexture* vulkanDepthImage = m_ResourceManager->GetTexture(m_DepthImage);
		VulkanRenderPass* vkRenderPass = m_ResourceManager->GetRenderPass(m_RenderPass);

		// Grab how many images we have in the swapchain.
		const uint32_t swapChainImageCount = m_SwapChainImages.size();
		m_FrameBuffers = std::vector<Handle<FrameBuffer>>(swapChainImageCount);

		for (int i = 0; i < swapChainImageCount; i++)
		{
			m_FrameBuffers[i] = m_ResourceManager->CreateFrameBuffer({
				.debugName = "swapchain-framebuffer",
				.width = m_SwapChainExtent.width,
				.height = m_SwapChainExtent.height,
				.renderPass = m_RenderPass,
				.depthTarget = m_DepthImage,
				.colorTargets = { m_SwapChainColorTextures[i] },
			});
		}
	}

	void VulkanRenderer::CreateDescriptorPool()
	{
		const uint32_t poolSize = 512;

		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, poolSize },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, poolSize },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, poolSize },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, poolSize },
		};

		VkDescriptorPoolCreateInfo tDescriptorPoolInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = 0,
			.maxSets = poolSize,
			.poolSizeCount = std::size(poolSizes),
			.pPoolSizes = poolSizes,
		};

		VK_VALIDATE(vkCreateDescriptorPool(m_Device->Get(), &tDescriptorPoolInfo, NULL, &m_DescriptorPool), "vkCreateDescriptorPool");

		m_MainDeletionQueue.PushFunction([=]()
		{
			vkDestroyDescriptorPool(m_Device->Get(), m_DescriptorPool, nullptr);
		});
	}

	void VulkanRenderer::CreateDescriptorSets()
	{
		// Global bindings for the 2D rendering.
		m_GlobalBindingsLayout2D = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "unlit-colored-layout",
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			auto cameraBuffer2D = m_ResourceManager->CreateBuffer({
				.debugName = "camera-uniform-buffer",
				.usage = BufferUsage::UNIFORM,
				.usageHint = BufferUsageHint::DYNAMIC,
				.memoryUsage = MemoryUsage::GPU_CPU,
				.byteSize = 64,
				.initialData = nullptr,
			});

			m_Frames[i].GlobalBindings2D = m_ResourceManager->CreateBindGroup({
				.debugName = "unlit-colored-bind-group",
				.layout = m_GlobalBindingsLayout2D,
				.buffers = {
					{ .buffer = cameraBuffer2D },
				}
			});
		}

		// Bindings for shadow rendering.
		m_ShadowBindingsLayout = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "shadow-bindings-layout",
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM_DYNAMIC_OFFSET,
				},
			},
		});

		uint64_t uniformOffset = Device::Instance->GetGPUProperties().limits.minUniformBufferOffsetAlignment;
		uint32_t alignedSize = UniformRingBuffer::CeilToNextMultiple(sizeof(glm::mat4), uniformOffset);

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			auto lightSpaceBuffer = m_ResourceManager->CreateBuffer({
				.debugName = "light-space-buffer",
				.usage = BufferUsage::UNIFORM,
				.usageHint = BufferUsageHint::DYNAMIC,
				.memoryUsage = MemoryUsage::GPU_CPU,
				.byteSize = 16 * alignedSize,
				.initialData = nullptr
			});

			m_Frames[i].ShadowBindings = m_ResourceManager->CreateBindGroup({
				.debugName = "shadow-bind-group",
				.layout = m_ShadowBindingsLayout,
				.buffers = {
					{ .buffer = lightSpaceBuffer, .range = 64 }, // TODO: Investigate if '.range' should be alignedSize! 
				}
			});
		}

		// Global bindings for the 3D rendering.
		m_GlobalBindingsLayout3D = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "global-bind-group-layout",
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM,
				},
				{
					.slot = 1,
					.visibility = ShaderStage::FRAGMENT,
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			auto cameraBuffer3D = m_ResourceManager->CreateBuffer({
				.debugName = "camera-uniform-buffer",
				.usage = BufferUsage::UNIFORM,
				.usageHint = BufferUsageHint::DYNAMIC,
				.memoryUsage = MemoryUsage::GPU_CPU,
				.byteSize = sizeof(CameraData),
				.initialData = nullptr,
			});

			auto lightBuffer = m_ResourceManager->CreateBuffer({
				.debugName = "light-uniform-buffer",
				.usage = BufferUsage::UNIFORM,
				.usageHint = BufferUsageHint::DYNAMIC,
				.memoryUsage = MemoryUsage::GPU_CPU,
				.byteSize = sizeof(LightData),
				.initialData = nullptr,
			});

			m_Frames[i].GlobalBindings3D = m_ResourceManager->CreateBindGroup({
				.debugName = "global-bind-group",
				.layout = m_GlobalBindingsLayout3D,
				.buffers = {
					{ .buffer = cameraBuffer3D },
					{ .buffer = lightBuffer },
				}
			});
		}

		// Global bindings for presenting to offscreen texture
		m_GlobalPresentBindingsLayout = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "global-present-bind-group-layout",
			.textureBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::FRAGMENT,
				},
			},
		});

		// Bindings for debug rendering.
		m_DebugBindingsLayout = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "debug-draw-bind-group-layout",
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			auto cameraBuffer = m_ResourceManager->CreateBuffer({
				.debugName = "debug-draw-camera-uniform-buffer",
				.usage = BufferUsage::UNIFORM,
				.usageHint = BufferUsageHint::DYNAMIC,
				.memoryUsage = MemoryUsage::GPU_CPU,
				.byteSize = sizeof(CameraData),
				.initialData = nullptr,
			});

		 	m_Frames[i].DebugBindings = m_ResourceManager->CreateBindGroup({
				.debugName = "debug-draw-bind-group",
				.layout = m_DebugBindingsLayout,
				.buffers = { { .buffer = cameraBuffer } }
			});
		}
	}

	// Helper

	VkPresentModeKHR VulkanRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool verticalSync)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (verticalSync)
			{
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					return availablePresentMode;
				}
			}
			else
			{
				if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					return availablePresentMode;
				}
			}
		}

		// Fallback.
		if (verticalSync)
		{
			return VK_PRESENT_MODE_FIFO_KHR;
		}

		return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
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
			// if (availableFormat.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 && availableFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}
}
