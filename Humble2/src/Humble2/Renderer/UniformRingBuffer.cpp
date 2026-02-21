#include "UniformRingBuffer.h"

#include "Renderer.h"
#include "Resources\ResourceManager.h"

namespace HBL2
{
	UniformRingBuffer::UniformRingBuffer(uint32_t size, uint32_t uniformOffset)
		: m_BufferSize(size), m_UniformOffset(uniformOffset), m_CurrentOffset(0)
	{
		m_Reservation = Allocator::Arena.Reserve("UniformRingBufferPool", m_BufferSize);
		m_Arena.Initialize(&Allocator::Arena, m_BufferSize, m_Reservation);

		m_Buffer = ResourceManager::Instance->CreateBuffer({
			.debugName = "dynamic-uniform-buffer",
			.usage = BufferUsage::UNIFORM,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memoryUsage = MemoryUsage::CPU_GPU,
			.byteSize = m_BufferSize,
			.initialData = nullptr,
		});

		m_BufferData = m_Arena.Alloc(m_BufferSize);
		memset(m_BufferData, 0, m_BufferSize);

		ResourceManager::Instance->SetBufferData(m_Buffer, 0, m_BufferData);
	}

	void UniformRingBuffer::Invalidate(uint32_t startOffset)
	{
		m_CurrentOffset = startOffset;
	}

	void UniformRingBuffer::Free()
	{
		ResourceManager::Instance->DeleteBuffer(m_Buffer);
	}

	uint32_t UniformRingBuffer::CeilToNextMultiple(uint32_t value, uint32_t step)
	{
		uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
		return step * divide_and_ceil;
	}
}