#include "VulkanImGuiRenderer.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace HBL2
{
	void VulkanImGuiRenderer::Initialize()
	{
		m_Device = (VulkanDevice*)Device::Instance;
		m_Renderer = (VulkanRenderer*)Renderer::Instance;

		// Create descriptor pool for IMGUI the size of the pool is very oversize, but it's copied from imgui demo itself.
		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(poolSizes);
		pool_info.pPoolSizes = poolSizes;

		VkDescriptorPool m_ImGuiPool;
		VK_VALIDATE(vkCreateDescriptorPool(m_Device->Get(), &pool_info, nullptr, &m_ImGuiPool), "vkCreateDescriptorPool");

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		CreateRenderPass();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(Window::Instance->GetHandle(), true);

		ImGui_ImplVulkan_InitInfo initInfo =
		{
			.Instance = m_Device->GetInstance(),
			.PhysicalDevice = m_Device->GetPhysicalDevice(),
			.Device = m_Device->Get(),
			.Queue = m_Renderer->GetGraphicsQueue(),
			.DescriptorPool = m_ImGuiPool,
			.Subpass = 0,
			.MinImageCount = 3,
			.ImageCount = 3,
			.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		};
		ImGui_ImplVulkan_Init(&initInfo, m_ImGuiRenderPass);

		m_Renderer->ImmediateSubmit([=](VkCommandBuffer cmd)
		{
			ImGui_ImplVulkan_CreateFontsTexture(cmd);
		});
	}

	void VulkanImGuiRenderer::BeginFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void VulkanImGuiRenderer::EndFrame()
	{
		// Rendering
		ImGui::Render();

		VkRenderPassBeginInfo renderPassInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = m_ImGuiRenderPass,
			.framebuffer = m_Renderer->GetMainFrameBuffer(),
			.renderArea = 
			{
				.offset = { 0, 0 },
				.extent = { Window::Instance->GetExtents().x, Window::Instance->GetExtents().y },
			},
			.clearValueCount = 0,
			.pClearValues = nullptr,
		};

		VK_VALIDATE(vkResetCommandBuffer(m_Renderer->GetCurrentFrame().ImGuiCommandBuffer, 0), "vkResetCommandBuffer");

		VkCommandBuffer cmd = m_Renderer->GetCurrentFrame().ImGuiCommandBuffer;

		VkCommandBufferBeginInfo cmdBeginInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr,
		};		

		VK_VALIDATE(vkBeginCommandBuffer(cmd, &cmdBeginInfo), "vkBeginCommandBuffer");

		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		vkCmdEndRenderPass(cmd);

		VK_VALIDATE(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");

		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &m_Renderer->GetCurrentFrame().MainRenderFinishedSemaphore,
			.pWaitDstStageMask = &waitStage,
			.commandBufferCount = 1,
			.pCommandBuffers = &m_Renderer->GetCurrentFrame().ImGuiCommandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &m_Renderer->GetCurrentFrame().ImGuiRenderFinishedSemaphore,
		};

		// Submit command buffer to the queue and execute it.
		// RenderFence will now block until the graphic commands finish execution
		VK_VALIDATE(vkQueueSubmit(m_Renderer->GetGraphicsQueue(), 1, &submitInfo, m_Renderer->GetCurrentFrame().InFlightFence), "vkQueueSubmit");
	}

	void VulkanImGuiRenderer::Clean()
	{
		vkDestroyDescriptorPool(m_Device->Get(), m_ImGuiPool, nullptr);
		vkDestroyRenderPass(m_Device->Get(), m_ImGuiRenderPass, nullptr);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void VulkanImGuiRenderer::CreateRenderPass()
	{
		// the renderpass will use this color attachment.
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = m_Renderer->GetSwapchainImageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Generally 1 sample for UI
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Load existing contents (the scene)
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the final output (UI + scene)
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout after scene render pass
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Layout for presenting to the screen

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Depth attachment
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.flags = 0;
		depthAttachment.format = VK_FORMAT_D32_SFLOAT;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Load existing depth information (optional)
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Often not needed for UI
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // After main scene pass
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		//we are going to create 1 subpass, which is the minimum you can do
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
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

		VK_VALIDATE(vkCreateRenderPass(m_Device->Get(), &renderPassInfo, nullptr, &m_ImGuiRenderPass), "vkCreateRenderPass");
	}
}
