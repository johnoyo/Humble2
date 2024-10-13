#pragma once

#include "Base.h"
#include "Core\Context.h"

#include "Scene\Components.h"
#include "Renderer\Renderer.h"

#include "VulkanCommandBuffer.h"

#include "VulkanCommon.h"

namespace HBL2
{
	class VulkanDevice;
	class VulkanResourceManager;

	constexpr unsigned int FRAME_OVERLAP = 2;

	struct FrameData
	{
		VkSemaphore ImageAvailableSemaphore; // Signal from swapchain.
		VkSemaphore MainRenderFinishedSemaphore; // Signal that main rendering is done.
		VkSemaphore ImGuiRenderFinishedSemaphore; // Signal that UI render is done.

		VkFence InFlightFence;

		VkCommandPool CommandPool;
		VkCommandBuffer MainCommandBuffer;
		VkCommandBuffer ImGuiCommandBuffer;

		VkDescriptorSet GlobalDescriptor;
	};

	struct UploadContext
	{
		VkFence UploadFence;
		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffer;
	};

	struct DeletionQueue
	{
		std::deque<std::function<void()>> deletors;

		void push_function(std::function<void()>&& function)
		{
			deletors.push_back(function);
		}

		void flush()
		{
			for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
			{
				(*it)();
			}

			deletors.clear();
		}
	};

	class VulkanRenderer final : public Renderer
	{
	public:
		virtual ~VulkanRenderer() = default;

		virtual void Initialize() override;
		virtual void BeginFrame() override;
		virtual void EndFrame() override;
		virtual void Present() override;
		virtual void Clean() override;

		virtual CommandBuffer* BeginCommandRecording(CommandBufferType type) override;

		virtual void ResizeFrameBuffer(uint32_t width, uint32_t height) override {}
		virtual void* GetDepthAttachment() override { return nullptr; }
		virtual void* GetColorAttachment() override { return nullptr; }

		virtual void SetPipeline(Handle<Shader> shader) override {}
		virtual void SetBuffers(Handle<Mesh> mesh, Handle<Material> material) override {}
		virtual void SetBindGroups(Handle<Material> material) override {}
		virtual void SetBindGroup(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset, uint32_t size) override {}
		virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) override {}
		virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) override {}
		virtual void WriteBuffer(Handle<Buffer> buffer, intptr_t offset) override {}
		virtual void WriteBuffer(Handle<BindGroup> bindGroup, uint32_t bufferIndex) override {}
		virtual void WriteBuffer(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset) override {}
		virtual void Draw(Handle<Mesh> mesh) override {}
		virtual void DrawIndexed(Handle<Mesh> mesh) override {}

		const VmaAllocator& GetAllocator() const { return m_Allocator; }
		const FrameData& GetCurrentFrame() const { return m_Frames[m_FrameNumber % FRAME_OVERLAP]; }
		const VkFormat& GetSwapchainImageFormat() const { return m_SwapChainImageFormat; }
		const VkFramebuffer& GetMainFrameBuffer() const { return m_FrameBuffers[m_SwapchainImageIndex]; }
		const VkRenderPass& GetMainRenderPass() const { return m_RenderPass; }
		const VkQueue& GetGraphicsQueue() const { return m_GraphicsQueue; }
		const VkQueue& GetPresentQueue() const { return m_PresentQueue; }

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

	private:
		void CreateAllocator();
		void CreateSwapchain();
		void CreateImageViews();
		void CreateCommands();
		void CreateRenderPass();
		void CreateFrameBuffers();
		void CreateSyncStructures();

		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	private:
		VulkanDevice* m_Device;
		VulkanResourceManager* m_ResourceManager;
		VmaAllocator m_Allocator;
		DeletionQueue m_MainDeletionQueue;

		FrameData m_Frames[FRAME_OVERLAP];
		VulkanCommandBuffer m_MainCommandBuffers[FRAME_OVERLAP];
		VulkanCommandBuffer m_ImGuiCommandBuffers[FRAME_OVERLAP];
		VulkanCommandBuffer m_OffScreenCommandBuffer;
		UploadContext m_UploadContext;

		uint32_t m_FrameNumber = 0;
		uint32_t m_SwapchainImageIndex = 0;

		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;

		VkSwapchainKHR m_SwapChain;
		VkFormat m_SwapChainImageFormat;
		VkExtent2D m_SwapChainExtent;
		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;

		Handle<Texture> m_DepthImage;
		VkFormat m_DepthFormat;

		VkRenderPass m_RenderPass;
		std::vector<VkFramebuffer> m_FrameBuffers;
	};
}