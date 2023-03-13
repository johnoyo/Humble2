#pragma once

#include "../Base.h"
#include <stdint.h>

namespace HBL
{
	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() = default;

		static VertexBuffer* Create(uint32_t size);

		virtual void Bind() = 0;
		virtual void UnBind() = 0;
		virtual void SetData() = 0;
		Buffer* GetHandle();
		uint32_t BatchSize = 0;
		uint32_t BatchIndex;
	protected:
		Buffer* m_Buffer = nullptr;
	};
}