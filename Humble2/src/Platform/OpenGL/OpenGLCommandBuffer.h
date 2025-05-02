#pragma once

#include "Renderer\CommandBuffer.h"
#include "OpenGLRenderPassRenderer.h"
#include "OpenGLComputePassRenderer.h"

namespace HBL2
{
	class OpenGLCommandBuffer final : public CommandBuffer
	{
	public:
		virtual RenderPassRenderer* BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer) override;
		virtual void EndRenderPass(const RenderPassRenderer& renderPassRenderer) override;

		virtual ComputePassRenderer* BeginComputePass(const Span<const Handle<Texture>>& texturesWrite, const Span<const Handle<Buffer>>& buffersWrite) override;
		virtual void EndComputePass(const ComputePassRenderer& computePassRenderer) override;

		virtual void EndCommandRecording() override;
		virtual void Submit() override;

	private:
		OpenGLRenderPasRenderer m_CurrentRenderPassRenderer;
		OpenGLComputePassRenderer m_CurrentComputePassRenderer;
		Handle<FrameBuffer> m_FrameBuffer;
	};
}