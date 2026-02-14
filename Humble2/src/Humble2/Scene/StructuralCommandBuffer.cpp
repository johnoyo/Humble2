#include "StructuralCommandBuffer.h"

namespace HBL2
{
	void StructuralCommandBuffer::Initialize()
	{
		uint32_t mainArenaByteSize = (sizeof(StructuralCommandBuffer::ChunkCommands) + sizeof(Arena)) * JobSystem::Get().GetThreadCount();

		m_Reservation = Allocator::Arena.Reserve(std::format("SCB-MPool"), mainArenaByteSize);
		m_Arena.Initialize(&Allocator::Arena, mainArenaByteSize, m_Reservation);

		m_ChunkCommands = MakeDArrayResized<StructuralCommandBuffer::ChunkCommands>(m_Arena, JobSystem::Get().GetThreadCount());

		constexpr uint32_t byteSize = 100_KB;

		for (int i = 0; i < JobSystem::Get().GetThreadCount(); i++)
		{
			m_ChunkCommands[i].Reservation = Allocator::Arena.Reserve(std::format("SCB-WPool-{}", i), byteSize);

			m_ChunkCommands[i].Arena = m_Arena.AllocConstruct<Arena>();
			m_ChunkCommands[i].Arena->Initialize(&Allocator::Arena, byteSize, m_ChunkCommands[i].Reservation);

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

			m_ChunkCommands[i].Arena->Reset(false);
		}
	}

	void StructuralCommandBuffer::Clear()
	{
		m_Arena.Reset(false);
	}

	void StructuralCommandBuffer::ClearAll()
	{
		for (int i = 0; i < m_ChunkCommands.size(); i++)
		{
			m_ChunkCommands[i].Arena->Reset(false);
		}

		m_Arena.Reset(false);
	}
}
