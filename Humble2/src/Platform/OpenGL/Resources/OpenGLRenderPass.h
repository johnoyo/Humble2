#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

namespace HBL2
{
	struct OpenGLRenderPass
	{
		OpenGLRenderPass() = default;
		OpenGLRenderPass(const RenderPassDescriptor&& desc);

		const char* DebugName = "";

		std::vector<bool> ColorClearValues;
		glm::vec4 ClearColor = {};
		uint32_t ColorTargetCount = 0;

		bool DepthClearValue = false;
		float ClearDepth = 0.0f;

		bool StencilClearValue = false;
		uint32_t ClearStencil = 0;
	};
}