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

	struct FrameData
	{
		VkSemaphore ImageAvailableSemaphore; // Signal from swapchain.

		VkSemaphore ShadowRenderFinishedSemaphore; // Signal that shadow rendering is done.
		VkSemaphore OpaqueRenderFinishedSemaphore; // Signal that opaque rendering is done.
		VkSemaphore SkyboxRenderFinishedSemaphore; // Signal that skybox rendering is done.
		VkSemaphore TransparentRenderFinishedSemaphore; // Signal that transparent rendering is done.

		VkSemaphore OpaqueSpriteRenderFinishedSemaphore; // Signal that opaque rendering is done.

		VkSemaphore PostProcessRenderFinishedSemaphore; // Signal that post process rendering is done.
		VkSemaphore PresentRenderFinishedSemaphore; // Signal that rendering onto the full screen quad is done.

		VkSemaphore ImGuiRenderFinishedSemaphore; // Signal that UI render is done.

		VkFence InFlightFence;

		VkCommandPool CommandPool;
		VkCommandBuffer ImGuiCommandBuffer;

		VkCommandBuffer ShadowCommandBuffer;
		VkCommandBuffer OpaqueCommandBuffer;
		VkCommandBuffer SkyboxCommandBuffer;
		VkCommandBuffer TransparentCommandBuffer;

		VkCommandBuffer OpaqueSpriteCommandBuffer;

		VkCommandBuffer PostProcessCommandBuffer;
		VkCommandBuffer PresentCommandBuffer;

		VkCommandPool SecondaryCommandPool;
		std::vector<VkCommandBuffer> SecondaryShadowCommandBuffers;
		std::vector<VkCommandBuffer> SecondaryOpaqueCommandBuffers;
		std::vector<VkCommandBuffer> SecondarySkyboxCommandBuffers;
		std::vector<VkCommandBuffer> SecondaryTransparentCommandBuffers;

		std::vector<VkCommandBuffer> SecondaryOpaqueSpriteCommandBuffers;

		std::vector<VkCommandBuffer> SecondaryPostProcessCommandBuffers;

		Handle<BindGroup> GlobalBindings2D;
		Handle<BindGroup> GlobalBindings3D;
		Handle<BindGroup> GlobalPresentBindings;
	};

	struct UploadContext
	{
		VkFence UploadFence;
		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffer;
	};

	class VulkanRenderer final : public Renderer
	{
	public:
		virtual ~VulkanRenderer() = default;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;
		virtual void Present() override;
		virtual void Clean() override;

		virtual CommandBuffer* BeginCommandRecording(CommandBufferType type, RenderPassStage stage) override;

		virtual void* GetDepthAttachment() override { return nullptr; }
		virtual void* GetColorAttachment() override { return m_ColorAttachmentID; }

		virtual void Draw(Handle<Mesh> mesh) override {}
		virtual void DrawIndexed(Handle<Mesh> mesh) override {}

		virtual Handle<BindGroup> GetGlobalBindings2D() override { return m_Frames[m_FrameNumber % FRAME_OVERLAP].GlobalBindings2D; }
		virtual Handle<BindGroup> GetGlobalBindings3D() override { return m_Frames[m_FrameNumber % FRAME_OVERLAP].GlobalBindings3D; }
		virtual Handle<BindGroup> GetGlobalPresentBindings() override { return m_Frames[m_FrameNumber % FRAME_OVERLAP].GlobalPresentBindings; }

		virtual Handle<FrameBuffer> GetMainFrameBuffer() override { return m_FrameBuffers[m_SwapchainImageIndex]; }

		const VmaAllocator& GetAllocator() const { return m_Allocator; } // TODO: Move to VulkanResourceManager

		const uint32_t GetFrameIndex() const { return m_FrameNumber % FRAME_OVERLAP; }
		const FrameData& GetCurrentFrame() const { return m_Frames[m_FrameNumber % FRAME_OVERLAP]; }
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
		void CreateSwapchain();
		void CreateImageViews();
		void CreateCommands();
		void CreateRenderPass();
		void CreateFrameBuffers();
		void CreateSyncStructures();
		void CreateDescriptorPool();
		void CreateDescriptorSets();

		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		void StubRenderPass();

	private:
		VulkanDevice* m_Device;
		VulkanResourceManager* m_ResourceManager;
		VmaAllocator m_Allocator;
		DeletionQueue m_MainDeletionQueue;

		FrameData m_Frames[FRAME_OVERLAP];

		VulkanCommandBuffer m_ShadowCommandBuffers[FRAME_OVERLAP];
		VulkanCommandBuffer m_OpaqueCommandBuffers[FRAME_OVERLAP];
		VulkanCommandBuffer m_SkyboxCommandBuffers[FRAME_OVERLAP];
		VulkanCommandBuffer m_TransparentCommandBuffers[FRAME_OVERLAP];

		VulkanCommandBuffer m_OpaqueSpriteCommandBuffers[FRAME_OVERLAP];

		VulkanCommandBuffer m_PostProcessCommandBuffers[FRAME_OVERLAP];
		VulkanCommandBuffer m_PresentCommandBuffers[FRAME_OVERLAP];
		
		VulkanCommandBuffer m_ImGuiCommandBuffers[FRAME_OVERLAP];
		UploadContext m_UploadContext;

		std::vector<VulkanCommandBuffer> m_SecondaryShadowCommandBuffers[FRAME_OVERLAP];
		std::vector<VulkanCommandBuffer> m_SecondaryOpaqueCommandBuffers[FRAME_OVERLAP];
		std::vector<VulkanCommandBuffer> m_SecondarySkyBoxCommandBuffers[FRAME_OVERLAP];
		std::vector<VulkanCommandBuffer> m_SecondaryTransparentCommandBuffers[FRAME_OVERLAP];
		std::vector<VulkanCommandBuffer> m_SecondaryPostProcessCommandBuffers[FRAME_OVERLAP];

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