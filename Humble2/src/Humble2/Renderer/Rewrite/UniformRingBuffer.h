#pragma once

#include "Types.h"
#include "Handle.h"
#include "Renderer.h"
#include "ResourceManager.h"

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
		UniformRingBuffer(uint32_t size, uint32_t uniformOffset) : m_BufferSize(size), m_UniformOffset(uniformOffset), m_CurrentOffset(0)
		{
			m_Renderer = Renderer::Instance;
			m_ResourceManager = ResourceManager::Instance;

			m_Buffer = m_ResourceManager->CreateBuffer({
				.debugName = "dynamic-uniform-buffer",
				.usage = BufferUsage::UNIFORM,
				.usageHint = BufferUsageHint::DYNAMIC,
				.byteSize = m_BufferSize,
				.initialData = nullptr,
			});

			m_BufferData = operator new(m_BufferSize);
			memset(m_BufferData, 0, m_BufferSize);

			m_Renderer->SetBufferData(m_Buffer, 0, m_BufferData);
		}

		template<typename T>
		Allocation<T> BumpAllocate()
		{
			uint32_t alignedStride = CeilToNextMultiple(sizeof(T), m_UniformOffset);

			if (m_CurrentOffset + alignedStride > m_BufferSize)
			{
				m_ResourceManager->ReAllocateBuffer(m_Buffer, m_CurrentOffset);
				ReAllocate();
				m_Renderer->SetBufferData(m_Buffer, 0, m_BufferData);
			}

			uint32_t blockIndex = m_CurrentOffset;
			m_CurrentOffset += alignedStride;

			return { .Data = (T*)((char*)m_BufferData + blockIndex), .Offset = blockIndex, };
		}

		Handle<Buffer> GetBuffer()
		{
			return m_Buffer;
		}

		void Invalidate()
		{
			m_CurrentOffset = 0;
			memset(m_BufferData, 0, m_BufferSize);
		}

	private:
		void ReAllocate()
		{
			void* oldData = m_BufferData;

			m_BufferData = operator new(m_BufferSize * 2);
			memset(m_BufferData, 0, m_BufferSize * 2);
			memcpy(m_BufferData, oldData, m_CurrentOffset);

			operator delete(oldData);

			m_BufferSize = m_BufferSize * 2;
		}

		uint32_t CeilToNextMultiple(uint32_t value, uint32_t step)
		{
			uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
			return step * divide_and_ceil;
		}

		Renderer* m_Renderer;
		ResourceManager* m_ResourceManager;
		Handle<Buffer> m_Buffer;
		void* m_BufferData;
		uint32_t m_BufferSize;
		uint32_t m_CurrentOffset;
		uint32_t m_UniformOffset;
	};
}