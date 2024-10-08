#pragma once

#include "ImGui\ImGuiRenderer.h"
#include "Core\Window.h"

#include "VulkanDevice.h"

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

		virtual void Clean() override;

	private:
		VkDescriptorPool m_ImGuiPool;
	};
}