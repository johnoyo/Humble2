#pragma once

#include <stdint.h>

namespace HBL2
{
	class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() = default;

		static IndexBuffer* Create(uint32_t size, bool generated = true);

		virtual void Bind() = 0;
		virtual void UnBind() = 0;
		virtual void SetData(uint32_t batchSize) = 0;
		virtual void Invalidate(uint32_t size) = 0;
		virtual uint32_t* GetHandle() = 0;
	};
}