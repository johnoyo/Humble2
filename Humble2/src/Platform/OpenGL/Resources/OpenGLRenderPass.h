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

			for (const auto& colorTarget : desc.colorTargets)
			{
				ColorClearValues.push_back(colorTarget.loadOp == LoadOperation::CLEAR);
			}

			DepthClearValue = desc.depthTarget.loadOp == LoadOperation::CLEAR;
		}

		const char* DebugName = "";
		std::vector<bool> ColorClearValues;
		bool DepthClearValue = false;
	};
}