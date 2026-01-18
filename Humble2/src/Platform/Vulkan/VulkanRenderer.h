#pragma once

#include "Base.h"
#include "Core\Context.h"

#include "Scene\Components.h"
#include "Renderer\Renderer.h"

#include "VulkanCommandBuffer.h"

#include "VulkanCommon.h"

#include "Utilities\Collections\DeletionQueue.h"

namespace HBL2
{
	class VulkanDevice;
	class VulkanResourceManager;

	constexpr unsigned int FRAME_OVERLAP = 2;

	struct VkFrameData
	{
		VkSemaphore ImageAvailableSemaphore; // Signal from swapchain.
		VkSemaphore MainRenderFinishedSemaphore; // Signal that main render is done.
		VkSemaphore ImGuiRenderFinishedSemaphore; // Signal that UI render is done.

		VkFence InFlightFence;

		VkCommandPool CommandPool;
		VkCommandBuffer MainCommandBuffer;
		VkCommandBuffer ImGuiCommandBuffer;

		Handle<BindGroup> ShadowBindings;
		Handle<BindGroup> DebugBindings;
		Handle<BindGroup> GlobalBindings2D;
		Handle<BindGroup> GlobalBindings3D;
		Handle<BindGroup> GlobalPresentBindings;
	};

	struct UploadContext
	{
		VkFence UploadFence = VK_NULL_HANDLE;
		VkCommandPool CommandPool = VK_NULL_HANDLE;
		VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
	};

	class VulkanRenderer final : public Renderer
	{
	public:
		virtual ~VulkanRenderer() = default;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;
		virtual void Present() override;
		virtual void Clean() override;

		virtual CommandBuffer* BeginCommandRecording(CommandBufferType type) override;

		virtual void* GetDepthAttachment() override { return nullptr; }
		virtual void* GetColorAttachment() override;		

		virtual void SetViewportAttachment(Handle<Texture> viewportTexture) {}
		virtual void* GetViewportAttachment() override { return m_ColorAttachmentID; }

		virtual Handle<BindGroup> GetShadowBindings() override { return m_Frames[m_FrameNumber.load() % FRAME_OVERLAP].ShadowBindings; }
		virtual Handle<BindGroup> GetGlobalBindings2D() override { return m_Frames[m_FrameNumber.load() % FRAME_OVERLAP].GlobalBindings2D; }
		virtual Handle<BindGroup> GetGlobalBindings3D() override { return m_Frames[m_FrameNumber.load() % FRAME_OVERLAP].GlobalBindings3D; }
		virtual Handle<BindGroup> GetGlobalPresentBindings() override { return m_Frames[m_FrameNumber.load() % FRAME_OVERLAP].GlobalPresentBindings; }
		virtual Handle<BindGroup> GetDebugBindings() override { return m_Frames[m_FrameNumber.load() % FRAME_OVERLAP].DebugBindings; }

		virtual Handle<FrameBuffer> GetMainFrameBuffer() override { return m_FrameBuffers[m_SwapchainImageIndex]; }

		const VmaAllocator& GetAllocator() const { return m_Allocator; } // TODO: Move to VulkanResourceManager

		virtual const uint32_t GetFrameIndex() const override { return m_FrameNumber.load() % FRAME_OVERLAP; }
		const VkFrameData& GetCurrentFrame() const { return m_Frames[m_FrameNumber.load() % FRAME_OVERLAP]; }
		const VkFormat& GetSwapchainImageFormat() const { return m_SwapChainImageFormat; }
		const VkExtent2D& GetSwapchainExtent() const { return m_SwapChainExtent; }

		const VkQueue& GetGraphicsQueue() const { return m_GraphicsQueue; }
		const VkQueue& GetPresentQueue() const { return m_PresentQueue; }

		const VkDescriptorPool& GetDescriptorPool() const { return m_DescriptorPool; }

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

		void Resize(uint32_t width, uint32_t height);

	protected:
		virtual void PreInitialize() override;
		virtual void PostInitialize() override;

	private:
		void CreateAllocator();
		void CreateSwapchain(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
		void CreateImageViews();
		void CreateSyncStructures();
		void CreateCommands();
		void CreateUploadContextCommands();
		void CreateRenderPass();
		void CreateFrameBuffers();
		void CreateDescriptorPool();
		void CreateDescriptorSets();

		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool verticalSync);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		void StubRenderPass();

	private:
		VulkanDevice* m_Device;
		VulkanResourceManager* m_ResourceManager;
		VmaAllocator m_Allocator;
		DeletionQueue m_MainDeletionQueue;

		VkFrameData m_Frames[FRAME_OVERLAP];
		
		VulkanCommandBuffer m_MainCommandBuffers[FRAME_OVERLAP];
		VulkanCommandBuffer m_ImGuiCommandBuffers[FRAME_OVERLAP];
		static thread_local UploadContext s_UploadContext;

		uint32_t m_SwapchainImageIndex = 0;

		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;

		VkSwapchainKHR m_SwapChain;
		VkFormat m_SwapChainImageFormat;
		VkExtent2D m_SwapChainExtent;
		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;

		VkFormat m_DepthFormat;
		Handle<Texture> m_DepthImage;
		std::vector<Handle<Texture>> m_SwapChainColorTextures;

		std::vector<Handle<FrameBuffer>> m_FrameBuffers;

		VkDescriptorPool m_DescriptorPool;
		VkDescriptorSet m_ColorAttachmentID = VK_NULL_HANDLE;

		bool m_Resize = false;
		glm::uvec2 m_NewSize{};
	};
}