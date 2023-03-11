#pragma once

#include <stdint.h>

namespace HBL
{
	class VertexBuffer
	{
	public:
		virtual void Initialize(uint32_t maxSize) = 0;
		virtual void SetData() = 0;
		virtual void Bind() = 0;
		virtual void UnBind() = 0;
	};
}