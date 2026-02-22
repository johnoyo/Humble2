#include "StructuralCommandBuffer.h"

namespace HBL2
{
	void StructuralCommandBuffer::Initialize(PoolReservation* reservation)
	{
		uint32_t workerThreadCount = JobSystem::Get().GetThreadCount();
		uint32_t mainArenaByteSize = (uint32_t)Allocator::CalculateInterleavedByteSize<StructuralCommandBuffer::ChunkCommands, Arena>(workerThreadCount);
		m_Arena.Initialize(&Allocator::Arena, mainArenaByteSize, reservation);

		m_ChunkCommands = MakeDArrayResized<StructuralCommandBuffer::ChunkCommands>(m_Arena, workerThreadCount);

		constexpr uint32_t byteSize = 100_KB;

		for (int i = 0; i < workerThreadCount; i++)
		{
			m_ChunkCommands[i].Arena = m_Arena.AllocConstruct<Arena>();
			m_ChunkCommands[i].Arena->Initialize(&Allocator::Arena, byteSize, reservation);

			m_ChunkCommands[i].Commands = MakeDArray<StructuralCommandBuffer::Command>(*m_ChunkCommands[i].Arena, 64);
		}
	}

	void StructuralCommandBuffer::Playback(Scene* scene)
	{
		for (int i = 0; i < m_ChunkCommands.size(); i++)
		{
			for (int j = 0; j < m_ChunkCommands[i].Commands.size(); j++)
			{
				const auto& command = m_ChunkCommands[i].Commands[j];

				switch (command.type)
				{
				case StructuralCommandBuffer::Command::Type::Add:
					command.operation.add(scene, command.entity, command.payload);
					break;
				case StructuralCommandBuffer::Command::Type::Remove:
					command.operation.remove(scene, command.entity);
					break;
				}
			}

			m_ChunkCommands[i].Arena->Reset();
		}
	}

	void StructuralCommandBuffer::Reset()
	{
		m_Arena.Reset();
	}

	void StructuralCommandBuffer::Clear()
	{
		for (int i = 0; i < m_ChunkCommands.size(); i++)
		{
			m_Arena.Destruct(&m_ChunkCommands[i].Commands);
			m_Arena.Destruct(m_ChunkCommands[i].Arena);
		}
	}
}
