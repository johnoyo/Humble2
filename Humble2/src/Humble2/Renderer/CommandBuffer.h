#pragma once

#include "RenderPassRenderer.h"
#include "ComputePassRenderer.h"

#include "Utilities/Collections/Span.h"

namespace HBL2
{
	struct Viewport
	{
		uint32_t x = std::numeric_limits<uint32_t>::max();
		uint32_t y = std::numeric_limits<uint32_t>::max();
		uint32_t width = std::numeric_limits<uint32_t>::max();
		uint32_t height = std::numeric_limits<uint32_t>::max();

		bool IsValid() const
		{
			return x != std::numeric_limits<uint32_t>::max() &&
				   y != std::numeric_limits<uint32_t>::max() &&
				   width != std::numeric_limits<uint32_t>::max() &&
				   height != std::numeric_limits<uint32_t>::max();
		}
	};

	class HBL2_API CommandBuffer
	{
	public:
		virtual RenderPassRenderer* BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer, Viewport&& drawArea = {}) = 0;
		virtual void EndRenderPass(const RenderPassRenderer& renderPassRenderer) = 0;

		virtual ComputePassRenderer* BeginComputePass(const Span<const Handle<Texture>>& texturesWrite, const Span<const Handle<Buffer>>& buffersWrite) = 0;
		virtual void EndComputePass(const ComputePassRenderer& computePassRenderer) = 0;

		virtual void EndCommandRecording() = 0;
		virtual void Submit() = 0;
	};
}
