#pragma once

#include "Base.h"
#include "Renderer\Device.h"
#include "Platform\Vulkan\VulkanWindow.h"

#include "VulkanCommon.h"

#include <set>
#include <vector>

namespace HBL2
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete()
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities{};
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	class VulkanDevice : public Device
	{
	public:
		~VulkanDevice() = default;

		virtual void Initialize() override;
		virtual void Destroy() override;

		const VkDevice Get() const { return m_Device; }
		const VkInstance GetInstance() const { return m_Instance; }
		const VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		const VkSurfaceKHR GetSurface() const { return m_Surface; }
		const SwapChainSupportDetails& GetSwapChainSupportDetails() const { return m_SwapChainSupportDetails; }
		const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }

	private:
		void CreateInstance();
		void SetupDebugMessenger();
		void CreateSurface();
		void SelectPhysicalDevice();
		void CreateLogicalDevice();

	private:
		bool CheckValidationLayerSupport();
		std::vector<const char*> GetRequiredExtensions();
		bool CheckInstanceExtensionSupport(const std::vector<const char*>& glfwExtensionNames);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
		bool IsDeviceSuitable(VkPhysicalDevice device);

	private:
		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkPhysicalDevice m_PhysicalDevice;
		VkPhysicalDeviceProperties m_VkGPUProperties;
		VkDevice m_Device;
		VkSurfaceKHR m_Surface;

		QueueFamilyIndices m_QueueFamilyIndices{};
		SwapChainSupportDetails m_SwapChainSupportDetails{};

		const std::vector<const char*> m_DeviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		};

		const std::vector<const char*> m_ValidationLayers =
		{
			"VK_LAYER_KHRONOS_validation"
		};

#ifdef DEBUG
		const bool m_EnableValidationLayers = true;
#elif RELEASE
		const bool m_EnableValidationLayers = false;
#endif
	};
}