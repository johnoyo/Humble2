#pragma once

#include "Renderer2D.h"

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
	};
}