#pragma once

#include "RenderPassRenderer.h"

namespace HBL2
{
	class CommandBuffer
	{
	public:
		RenderPassRenderer* BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer);
		void EndRenderPass(const RenderPassRenderer& renderPassRenderer);
		void Submit();
	private:
		RenderPassRenderer m_CurrentRenderPassRenderer;
	};
}