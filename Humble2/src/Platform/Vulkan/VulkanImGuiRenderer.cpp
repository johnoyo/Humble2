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

		ImGui_ImplVulkan_InitInfo initInfo = { 0 };
		initInfo.Instance = m_Device->GetInstance();
		initInfo.PhysicalDevice = m_Device->GetPhysicalDevice();
		initInfo.Device = m_Device->Get();
		initInfo.Queue = m_Renderer->GetGraphicsQueue();
		initInfo.DescriptorPool = m_ImGuiPool;
		initInfo.MinImageCount = 2l;
		initInfo.ImageCount = 2;
		initInfo.PipelineInfoMain =
		{
			.RenderPass = renderPass->RenderPass,
			.Subpass = 0,
			.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
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

		{
			ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
			std::vector<ImTextureData*> pending;
			pending.reserve(pio.Textures.Size);

			for (ImTextureData* tex : pio.Textures)
			{
				if (tex->Status != ImTextureStatus_OK)
				{
					pending.push_back(tex);
				}
			}

			if (!pending.empty())
			{
				m_Renderer->SubmitBlocking([pending = std::move(pending)]() mutable
				{
					for (ImTextureData* tex : pending)
					{
						// NOTE: This ImGui function causes the validation error: Validation layer: Validation Error: [ VUID-vkDestroyBuffer-buffer-00922 ] |
						// MessageID = 0xe4549c11 | vkDestroyBuffer():  can't be called on VkBuffer 0x88693900000000c0[] that is currently in use by VkDescriptorSet 0x67dd1700000000e0[].
						// The Vulkan spec states: All submitted commands that refer to buffer, either directly or via a VkBufferView, must have completed execution
						// (https://vulkan.lunarg.com/doc/view/1.3.290.0/windows/1.3-extensions/vkspec.html#VUID-vkDestroyBuffer-buffer-00922)
						// 
						// UPDATE: Not setting the 'DrawData.Textures = nullptr' as suggested by ImGui fixes this error, it could produce other problems though, keep an eye out.
						ImGui_ImplVulkan_UpdateTexture(tex);
					}
				});
			}
		}

		{
			// Update and Render additional Platform Windows.
			ImGuiIO& io = ImGui::GetIO(); (void)io;

			// NOTE: If we dont call 'ImGui::UpdatePlatformWindows()' before the next 'ImGui::NewFrame()' call we hit an assert.
			// So to prevent it we update and render here prematurely.
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();

				m_Renderer->SubmitBlocking([]()
				{
					ImGui::RenderPlatformWindowsDefault();
				});
			}
		}

		m_Renderer->CollectImGuiRenderData(ImGui::GetDrawData(), ImGui::GetTime());
	}

	void VulkanImGuiRenderer::Render(const FrameData& frameData)
	{
		ImDrawData* data = (ImDrawData*)&frameData.ImGuiRenderData.DrawData;

		ImGui_ImplVulkan_NewFrame();

		CommandBuffer* commandBuffer = m_Renderer->BeginCommandRecording(CommandBufferType::UI);
		RenderPassRenderer* renderPassRenderer = commandBuffer->BeginRenderPass(m_ImGuiRenderPass, m_Renderer->GetMainFrameBuffer());

		ImGui_ImplVulkan_RenderDrawData(data, m_Renderer->GetCurrentFrame().ImGuiCommandBuffer);

		commandBuffer->EndRenderPass(*renderPassRenderer);
		commandBuffer->EndCommandRecording();
		commandBuffer->Submit();
	}

	void VulkanImGuiRenderer::Clean()
	{
		vkDeviceWaitIdle(m_Device->Get());

		m_Renderer->ClearFrameDataBuffer();

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
