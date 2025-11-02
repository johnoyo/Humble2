#include "VulkanImGuiRenderer.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "Core\Input.h"

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

		VkDescriptorPoolCreateInfo poolInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			.maxSets = 1000,
			.poolSizeCount = std::size(poolSizes),
			.pPoolSizes = poolSizes,
		};

		VK_VALIDATE(vkCreateDescriptorPool(m_Device->Get(), &poolInfo, nullptr, &m_ImGuiPool), "vkCreateDescriptorPool");

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
			.MinImageCount = 3,
			.ImageCount = 3,
			.PipelineInfoMain =
			{
				.RenderPass = renderPass->RenderPass,
				.Subpass = 0,
				.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
			}
		};
		ImGui_ImplVulkan_Init(&initInfo);
	}

	void VulkanImGuiRenderer::BeginFrame()
	{
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void VulkanImGuiRenderer::EndFrame()
	{
		ImGui::Render();
		Render();
	}

	void VulkanImGuiRenderer::Render()
	{
		ImGui_ImplVulkan_NewFrame();

		CommandBuffer* commandBuffer = m_Renderer->BeginCommandRecording(CommandBufferType::UI);
		RenderPassRenderer* renderPassRenderer = commandBuffer->BeginRenderPass(m_ImGuiRenderPass, m_Renderer->GetMainFrameBuffer());

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_Renderer->GetCurrentFrame().ImGuiCommandBuffer);

		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		commandBuffer->EndRenderPass(*renderPassRenderer);

		commandBuffer->EndCommandRecording();
		commandBuffer->Submit();
	}

	void VulkanImGuiRenderer::Clean()
	{
		vkDeviceWaitIdle(m_Device->Get());

		ImGui_ImplVulkan_Shutdown();

		vkDestroyDescriptorPool(m_Device->Get(), m_ImGuiPool, nullptr);

		VulkanRenderPass* renderPass = m_ResourceManager->GetRenderPass(m_ImGuiRenderPass);
		vkDestroyRenderPass(m_Device->Get(), renderPass->RenderPass, nullptr);

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
				.prevUsage = TextureLayout::DEPTH_STENCIL,
				.nextUsage = TextureLayout::DEPTH_STENCIL,
			},
			.colorTargets = {
				{
					.loadOp = LoadOperation::LOAD,
					.storeOp = StoreOperation::STORE,
					.prevUsage = TextureLayout::RENDER_ATTACHMENT,
					.nextUsage = TextureLayout::PRESENT,
				},
			},
		});
	}
}
