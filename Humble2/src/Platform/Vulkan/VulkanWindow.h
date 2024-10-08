#pragma once

#include "vulkan\vulkan.h"
#include "Core\Window.h"

namespace HBL2
{
	class VulkanWindow final : public Window
	{
		~VulkanWindow() = default;

		virtual void Create() override;
		virtual void Present() override;
	};
}