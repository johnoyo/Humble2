#pragma once

#include "Resources\Types.h"
#include "Resources\Handle.h"

namespace HBL2
{
	template<typename T>
	struct Allocation
	{
		T* Data;
		uint32_t Offset;
	};

	class HBL2_API UniformRingBuffer
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
		uint32_t GetBufferSize() const { return m_BufferSize; }

		const uint32_t GetCurrentOffset() const { return m_CurrentOffset; }
		const uint32_t GetAlignedSize(uint32_t byteSize) { return CeilToNextMultiple(byteSize, m_UniformOffset); }

		void Invalidate(uint32_t startOffset = 0);

		void Free();

		static uint32_t CeilToNextMultiple(uint32_t value, uint32_t step);

	private:
		void ReAllocate();

	private:
		Handle<Buffer> m_Buffer;
		void* m_BufferData;
		uint32_t m_BufferSize;
		uint32_t m_CurrentOffset;
		uint32_t m_UniformOffset;
	};
}