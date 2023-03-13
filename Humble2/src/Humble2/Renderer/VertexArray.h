#pragma once

#include "IndexBuffer.h"
#include "VertexBuffer.h"

#include<vector>

namespace HBL
{
	class VertexArray
	{
	public:
		virtual ~VertexArray() = default;

		static VertexArray* Create();

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		virtual std::vector<VertexBuffer*>& GetVertexBuffers() = 0;
		virtual IndexBuffer* GetIndexBuffer() = 0;
		virtual void AddVertexBuffer(VertexBuffer* vertexBuffer) = 0;
		virtual void SetIndexBuffer(IndexBuffer* indexBuffer) = 0;
	};
}