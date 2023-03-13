#pragma once

#include <stdint.h>

namespace HBL
{
	class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() = default;

		static IndexBuffer* Create(uint32_t size);

		virtual void Bind() = 0;
		virtual void UnBind() = 0;
		virtual void SetData(uint32_t batchSize) = 0;
	};
}