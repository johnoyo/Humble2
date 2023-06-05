#pragma once

#include <stdint.h>

namespace HBL2
{
	struct FrameBufferSpecification
	{
		uint32_t Width;
		uint32_t Height;
	};

	class FrameBuffer
	{
	public:
		virtual ~FrameBuffer() = default;

		static FrameBuffer* Create(FrameBufferSpecification& spec);

		virtual void Invalidate() = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		virtual void Clean() = 0;

		virtual uint32_t GetColorAttachmentID() const = 0;
		virtual uint32_t GetDepthAttachmentID() const = 0;

		virtual FrameBufferSpecification& GetSpecification() = 0;
	};
}