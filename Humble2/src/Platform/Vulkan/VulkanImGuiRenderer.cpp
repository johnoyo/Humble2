#include "VulkanImGuiRenderer.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define VK_VALIDATE(result, vkFunction) HBL2_CORE_ASSERT(result == VK_SUCCESS, std::format("Vulkan function: {}, failed!", vkFunction));

namespace HBL2
{
	void VulkanImGuiRenderer::Initialize()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;

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
		VK_VALIDATE(vkCreateDescriptorPool(device->Get(), &pool_info, nullptr, &m_ImGuiPool), "vkCreateDescriptorPool");

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

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(Window::Instance->GetHandle(), true);

		ImGui_ImplVulkan_InitInfo init_info =
		{
			.Instance = device->GetInstance(),
			.PhysicalDevice = device->GetPhysicalDevice(),
			.Device = device->Get(),
			.DescriptorPool = m_ImGuiPool,
			.Subpass = 0,
			.MinImageCount = 3,
			.ImageCount = 3,
			.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		};
		// ImGui_ImplVulkan_Init(&init_info, wd->RenderPass); TODO: Add render pass when we have it.
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

		VkClearValue clearValue;
		clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } }; // TODO: Get from renderer

		VkRenderPassBeginInfo renderPassInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			// .renderPass = renderPass, TODO: Add when available
			.renderArea = 
			{
				.offset = { 0, 0 },
				.extent = { Window::Instance->GetExtents().x, Window::Instance->GetExtents().y },
			},
			// .framebuffer = framebuffer, TODO: Add when available
			.clearValueCount = 2,
			.pClearValues = &clearValue,
		};

		VkCommandBuffer cmd; // = Renderer::Instance->GetCurrentFrame().ImGuiCommandBuffer; TODO: implement this.

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
	}

	void VulkanImGuiRenderer::Clean()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		vkDestroyDescriptorPool(device->Get(), m_ImGuiPool, nullptr);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}
