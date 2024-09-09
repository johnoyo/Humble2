#pragma once

#include "Types.h"
#include "Handle.h"

namespace HBL2
{
	template<typename T>
	struct Allocation
	{
		T* Data;
		uint32_t Offset;
	};

	class UniformRingBuffer
	{
	public:
		UniformRingBuffer(uint32_t size, uint32_t uniformOffset);

		template<typename T>
		Allocation<T> BumpAllocate()
		{
			uint32_t alignedStride = CeilToNextMultiple(sizeof(T), m_UniformOffset);

			if (m_CurrentOffset + alignedStride > m_BufferSize)
			{
				ReAllocate();
			}

			uint32_t blockIndex = m_CurrentOffset;
			m_CurrentOffset += alignedStride;

			return { .Data = (T*)((char*)m_BufferData + blockIndex), .Offset = blockIndex, };
		}

		Handle<Buffer> GetBuffer() const { return m_Buffer; }

		void Invalidate();

	private:
		void ReAllocate();

		uint32_t CeilToNextMultiple(uint32_t value, uint32_t step);

	private:
		Handle<Buffer> m_Buffer;
		void* m_BufferData;
		uint32_t m_BufferSize;
		uint32_t m_CurrentOffset;
		uint32_t m_UniformOffset;
	};
}