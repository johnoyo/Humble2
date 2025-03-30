#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

namespace HBL2
{
	struct OpenGLRenderPass
	{
		OpenGLRenderPass() = default;
		OpenGLRenderPass(const RenderPassDescriptor&& desc)
		{
			DebugName = desc.debugName;

			ColorClearValues.reserve(desc.colorTargets.Size());
			for (const auto& colorTarget : desc.colorTargets)
			{
				ColorClearValues.push_back(colorTarget.loadOp == LoadOperation::CLEAR);
				ClearColor = colorTarget.clearColor;
			}
			ColorTargetCount = desc.colorTargets.Size();

			DepthClearValue = desc.depthTarget.loadOp == LoadOperation::CLEAR;
			ClearDepth = desc.depthTarget.clearZ;

			StencilClearValue = desc.depthTarget.stencilLoadOp == LoadOperation::CLEAR;
			ClearStencil = desc.depthTarget.clearStencil;
		}

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