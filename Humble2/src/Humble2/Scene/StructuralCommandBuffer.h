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
					scene->GetRegistry().emplace_or_replace<T>(e, value);
				};

				op.remove = [](Scene* scene, Entity e)
				{
					scene->GetRegistry().remove<T>(e);
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
			PoolReservation* Reservation = nullptr;
			DArray<Command> Commands = MakeEmptyDArray<Command>();
		};

		void Initialize();

		template<class T>
		void Add(Entity e, T value = {})
		{
			uint32_t workerIndex = JobSystem::Get().GetWorkerIndex();

			auto& command = m_ChunkCommands[workerIndex];

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
			command.Commands.emplace_back(Command{
				.type = Command::Type::Remove,
				.operation = Operation::Construct<T>(),
				.entity = e,
			});
		}

		void Playback(Scene* scene);
		void Clear();
		void ClearAll();

	private:
		Arena m_Arena;
		PoolReservation* m_Reservation;
		DArray<ChunkCommands> m_ChunkCommands = MakeEmptyDArray<ChunkCommands>();
	};
}