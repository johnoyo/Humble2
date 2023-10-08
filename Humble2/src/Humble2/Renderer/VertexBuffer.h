#pragma once

#include "Base.h"
#include <stdint.h>

namespace HBL2
{
	enum class Type
	{
		FLOAT,
		UNSIGNED_INT,
		UNSIGNED_BYTE
	};

	struct VertexBufferElement
	{
		int Index;
		int Count;
		Type Type;
		bool Normalized;
	};

	class VertexBufferLayout
	{
	public:
		VertexBufferLayout(std::vector<VertexBufferElement> elements) : m_Elements(elements), m_Stride(0) 
		{
			for (auto& element : m_Elements)
			{
				uint32_t sizeOfType = 0;

				switch (element.Type)
				{
				case Type::FLOAT:
					sizeOfType = 4;
					break;
				case Type::UNSIGNED_INT:
					sizeOfType = 4;
					break;
				case Type::UNSIGNED_BYTE:
					sizeOfType = 1;
					break;
				}

				m_Stride += sizeOfType * element.Count;
			}
		}

		uint32_t GetStride() { return m_Stride; }
		std::vector<VertexBufferElement>& GetVertexBufferElements() { return m_Elements; }

	private:
		std::vector<VertexBufferElement> m_Elements;
		uint32_t m_Stride;
	};

	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() = default;

		static VertexBuffer* Create(uint32_t size, VertexBufferLayout& layout);
		static VertexBuffer* Create(Buffer* handle, uint32_t size, VertexBufferLayout& layout);

		virtual void Bind() = 0;
		virtual void UnBind() = 0;
		virtual void Clean() = 0;
		virtual void SetData() = 0;
		Buffer* GetHandle();
		uint32_t BatchSize = 0;
		uint32_t BatchIndex;
	protected:
		Buffer* m_Buffer = nullptr;
	};
}