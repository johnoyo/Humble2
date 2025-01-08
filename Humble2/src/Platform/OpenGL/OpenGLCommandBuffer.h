#pragma once

#include "Renderer\CommandBuffer.h"
#include "OpenGLRenderPassRenderer.h"

namespace HBL2
{
	class OpenGLCommandBuffer final : public CommandBuffer
	{
	public:
		virtual RenderPassRenderer* BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer) override;
		virtual void EndRenderPass(const RenderPassRenderer& renderPassRenderer) override;
		virtual void Submit() override;

	private:
		OpenGLRenderPasRenderer m_CurrentRenderPassRenderer;
		Handle<FrameBuffer> m_FrameBuffer;
	};
}