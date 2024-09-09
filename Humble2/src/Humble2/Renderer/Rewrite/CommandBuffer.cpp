#include "CommandBuffer.h"

namespace HBL2
{
	RenderPassRenderer* CommandBuffer::BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer)
	{
		// TODO: FIXME
		return &m_CurrentRenderPassRenderer;
	}

	void CommandBuffer::EndRenderPass(const RenderPassRenderer& renderPassRenderer)
	{
	}

	void CommandBuffer::Submit()
	{
	}
}
