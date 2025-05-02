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
		Handle<FrameBuffer> m_FrameBuffer;

		OpenGLComputePassRenderer m_CurrentComputePassRenderer;
		Span<const Handle<Texture>> m_TexturesWrite;
		Span<const Handle<Buffer>> m_BuffersWrite;
	};
}