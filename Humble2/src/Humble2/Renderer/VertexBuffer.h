#pragma once

#include "Renderer2D.h"

#include <stdint.h>

namespace HBL
{
	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() = default;

		static VertexBuffer* Create(uint32_t size);

		virtual void SetData() = 0;
		virtual void Bind() = 0;
		virtual void UnBind() = 0;
	};
}