#pragma once

#include "RenderPassRenderer.h"
#include "ComputePassRenderer.h"

#include "Utilities/Collections/Span.h"

namespace HBL2
{
	class HBL2_API CommandBuffer
	{
	public:
		virtual RenderPassRenderer* BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer) = 0;
		virtual void EndRenderPass(const RenderPassRenderer& renderPassRenderer) = 0;

		virtual ComputePassRenderer* BeginComputePass(Span<const Handle<Texture>> texturesWrite, Span<const Handle<Buffer>> buffersWrite) = 0;
		virtual void EndComputePass(const ComputePassRenderer& computePassRenderer) = 0;

		virtual void EndCommandRecording() = 0;
		virtual void Submit() = 0;
	};
}
