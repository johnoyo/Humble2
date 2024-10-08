#pragma once

#include "Renderer\Renderer.h"
#include "Renderer\RenderPassRenderer.h"

#include "Resources\ResourceManager.h"
#include "VulkanResourceManager.h"

#include "Resources\VulkanBuffer.h"
#include "Resources\VulkanBindGroup.h"

namespace HBL2
{
	class VulkanRenderPasRenderer final : public RenderPassRenderer
	{
	public:
		virtual void DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws) override;
	};
}