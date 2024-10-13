#include "VulkanImGuiRenderer.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace HBL2
{
	void VulkanImGuiRenderer::Initialize()
	{
		m_Device = (VulkanDevice*)Device::Instance;
		m_Renderer = (VulkanRenderer*)Renderer::Instance;
		m_ResourceManager = (VulkanResourceManager*)ResourceManager::Instance;

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

		VulkanRenderPass* renderPass = m_ResourceManager->GetRenderPass(m_ImGuiRenderPass);

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
		ImGui_ImplVulkan_Init(&initInfo, renderPass->RenderPass);

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
		ImGuizmo::BeginFrame();
	}

	void VulkanImGuiRenderer::EndFrame()
	{
		// Rendering
		ImGui::Render();

		CommandBuffer* commandBuffer = m_Renderer->BeginCommandRecording(CommandBufferType::UI);
		RenderPassRenderer* renderPassRenderer = commandBuffer->BeginRenderPass(m_ImGuiRenderPass, Handle<FrameBuffer>());

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_Renderer->GetCurrentFrame().ImGuiCommandBuffer);

		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		commandBuffer->EndRenderPass(*renderPassRenderer);
		commandBuffer->Submit();
	}

	void VulkanImGuiRenderer::Clean()
	{
		vkDeviceWaitIdle(m_Device->Get());

		VulkanRenderPass* renderPass = m_ResourceManager->GetRenderPass(m_ImGuiRenderPass);

		vkDestroyDescriptorPool(m_Device->Get(), m_ImGuiPool, nullptr);
		vkDestroyRenderPass(m_Device->Get(), renderPass->RenderPass, nullptr);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void VulkanImGuiRenderer::CreateRenderPass()
	{
		Handle<RenderPassLayout> renderPassLayout = m_ResourceManager->CreateRenderPassLayout({
			.debugName = "imgui-renderpass-layout",
			.depthTargetFormat = Format::D32_FLOAT,
			.subPasses = {
				{ .depthTarget = true, .colorTargets = 1, },
			},
		});

		m_ImGuiRenderPass = m_ResourceManager->CreateRenderPass({
			.debugName = "imgui-renderpass",
			.layout = renderPassLayout,
			.depthTarget = {
				.loadOp = LoadOperation::LOAD,
				.storeOp = StoreOperation::STORE,
				.stencilLoadOp = LoadOperation::DONT_CARE,
				.stencilStoreOp = StoreOperation::DONT_CARE,
				.prevUsage = TextureUsage::DEPTH_STENCIL,
				.nextUsage = TextureUsage::DEPTH_STENCIL,
			},
			.colorTargets = {
				{
					.loadOp = LoadOperation::LOAD,
					.storeOp = StoreOperation::STORE,
					.prevUsage = TextureUsage::RENDER_ATTACHMENT,
					.nextUsage = TextureUsage::PRESENT,
				},
			},
		});
	}
}
