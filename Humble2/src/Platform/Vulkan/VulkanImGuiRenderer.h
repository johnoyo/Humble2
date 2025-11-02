#pragma once

#include "ImGui\ImGuiRenderer.h"
#include "Core\Window.h"

#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanResourceManager.h"

#include "VulkanCommon.h"

#include <imgui.h>
#include <ImGuizmo.h>

#include <string>
#include <format>

namespace HBL2
{
	class VulkanImGuiRenderer final : public ImGuiRenderer
	{
	public:
		virtual void Initialize() override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;
		virtual void Render() override;

		virtual void Clean() override;

	private:
		void CreateRenderPass();

	private:
		Handle<RenderPass> m_ImGuiRenderPass;
		VkDescriptorPool m_ImGuiPool;

		VulkanDevice* m_Device = nullptr;
		VulkanRenderer* m_Renderer = nullptr;
		VulkanResourceManager* m_ResourceManager = nullptr;
	};
}