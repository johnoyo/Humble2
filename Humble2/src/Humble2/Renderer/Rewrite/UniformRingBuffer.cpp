#include "UniformRingBuffer.h"

#include "Renderer.h"
#include "Resources\ResourceManager.h"

namespace HBL2
{
	UniformRingBuffer::UniformRingBuffer(uint32_t size, uint32_t uniformOffset) : m_BufferSize(size), m_UniformOffset(uniformOffset), m_CurrentOffset(0)
	{
		m_Buffer = ResourceManager::Instance->CreateBuffer({
			.debugName = "dynamic-uniform-buffer",
			.usage = BufferUsage::UNIFORM,
			.usageHint = BufferUsageHint::DYNAMIC,
			.byteSize = m_BufferSize,
			.initialData = nullptr,
		});

		m_BufferData = operator new(m_BufferSize);
		memset(m_BufferData, 0, m_BufferSize);

		Renderer::Instance->SetBufferData(m_Buffer, 0, m_BufferData);
	}

	void UniformRingBuffer::Invalidate()
	{
		m_CurrentOffset = 0;
		memset(m_BufferData, 0, m_BufferSize);
	}

	void UniformRingBuffer::ReAllocate()
	{
		// Reallocate GPU buffer.
		ResourceManager::Instance->ReAllocateBuffer(m_Buffer, m_CurrentOffset);

		// Reallocate CPU buffer.
		void* oldData = m_BufferData;

		m_BufferData = operator new(m_BufferSize * 2);
		memset(m_BufferData, 0, m_BufferSize * 2);
		memcpy(m_BufferData, oldData, m_CurrentOffset);

		operator delete(oldData);

		m_BufferSize = m_BufferSize * 2;

		// Update buffer.
		Renderer::Instance->SetBufferData(m_Buffer, 0, m_BufferData);
	}

	uint32_t UniformRingBuffer::CeilToNextMultiple(uint32_t value, uint32_t step)
	{
		uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
		return step * divide_and_ceil;
	}
}