#include "OpenGLRenderPass.h"

#include "Platform/OpenGL/OpenGLResourceManager.h"

namespace HBL2
{
	OpenGLRenderPass::OpenGLRenderPass(const RenderPassDescriptor&& desc)
	{
		OpenGLResourceManager* rm = (OpenGLResourceManager*)ResourceManager::Instance;

		DebugName = desc.debugName;

		OpenGLRenderPassLayout* layout = rm->GetRenderPassLayout(desc.layout);

		ColorClearValues.reserve(desc.colorTargets.Size());
		for (const auto& colorTarget : desc.colorTargets)
		{
			ColorClearValues.push_back(colorTarget.loadOp == LoadOperation::CLEAR);
			ClearColor = colorTarget.clearColor;
		}
		ColorTargetCount = desc.colorTargets.Size();

		if (layout->Pass.depthTarget)
		{
			DepthClearValue = desc.depthTarget.loadOp == LoadOperation::CLEAR;
			ClearDepth = desc.depthTarget.clearZ;
		}
		else
		{
			DepthClearValue = false;
		}

		StencilClearValue = desc.depthTarget.stencilLoadOp == LoadOperation::CLEAR;
		ClearStencil = desc.depthTarget.clearStencil;
	}
}
