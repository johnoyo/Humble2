#pragma once

#include "Scene.h"
#include "Utilities\Allocators\Arena.h"
#include "Utilities\Collections\Collections.h"

#include <format>

namespace HBL2
{
	class StructuralCommandBuffer
	{
	public:
		struct Operation
		{
			using AddFn = void(*)(Scene*, Entity, const void*);
			using RemoveFn = void(*)(Scene*, Entity);

			AddFn add = nullptr;
			RemoveFn remove = nullptr;
			uint32_t size = 0;
			uint32_t align = 0;

			template<class T>
			static Operation Construct()
			{
				Operation op;

				op.size = (uint32_t)sizeof(T);
				op.align = (uint32_t)alignof(T);

				op.add = [](Scene* scene, Entity e, const void* payload)
				{
					const T& value = *reinterpret_cast<const T*>(payload);
					scene->GetRegistry().AddOrReplaceComponent<T>(e, value);
				};

				op.remove = [](Scene* scene, Entity e)
				{
					scene->GetRegistry().RemoveComponent<T>(e);
				};

				return op;
			}
		};

		struct Command
		{
			enum class Type
			{
				None,
				Add,
				Remove,
			};

			Type type = Type::None;
			Operation operation;
			Entity entity = Entity::Null;
			void* payload = nullptr;
		};

		struct ChunkCommands
		{
			Arena* Arena = nullptr;
			DArray<Command> Commands = MakeEmptyDArray<Command>();
		};

		void Initialize(PoolReservation* reservation, uint32_t mainArenaByteSize, uint32_t maxStructuralCommandsPerFramePerThread);

		template<class T>
		void Add(Entity e, T value = {})
		{
			uint32_t workerIndex = JobSystem::Get().GetWorkerIndex();

			auto& command = m_ChunkCommands[workerIndex];

			if (command.Commands.size() == m_MaxStructuralCommandsPerFramePerThread)
			{
				return;
			}

			void* payload = command.Arena->Alloc(sizeof(T), alignof(T));
			std::memcpy(payload, &value, sizeof(T));

			command.Commands.emplace_back(Command{
				.type = Command::Type::Add,
				.operation = Operation::Construct<T>(),
				.entity = e,
				.payload = payload,
			});
		}

		template<class T>
		void Remove(Entity e)
		{
			uint32_t workerIndex = JobSystem::Get().GetWorkerIndex();

			auto& command = m_ChunkCommands[workerIndex];

			if (command.Commands.size() == m_MaxStructuralCommandsPerFramePerThread)
			{
				return;
			}

			command.Commands.emplace_back(Command{
				.type = Command::Type::Remove,
				.operation = Operation::Construct<T>(),
				.entity = e,
			});
		}

		void Playback(Scene* scene);
		void Reset();
		void Clear();

	private:
		Arena m_Arena;
		DArray<ChunkCommands> m_ChunkCommands = MakeEmptyDArray<ChunkCommands>();
		uint32_t m_MaxStructuralCommandsPerFramePerThread = 0;
	};
}