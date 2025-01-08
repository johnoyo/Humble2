#pragma once

#include "vulkan\vulkan.h"
#include "Core\Window.h"

namespace HBL2
{
	class VulkanWindow final : public Window
	{
	public:
		~VulkanWindow() = default;

		virtual void Create() override;
	};
}