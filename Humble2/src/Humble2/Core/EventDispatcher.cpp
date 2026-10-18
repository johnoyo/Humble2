#include "EventDispatcher.h"

namespace HBL2
{
	EventDispatcher* EventDispatcher::s_Instance = nullptr;

	EventDispatcher& EventDispatcher::Get()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "EventDispatcher s_Instance is null! Call EventDispatcher::Initialize before use.");
		return *s_Instance;
	}

	void EventDispatcher::Initialize()
	{
		HBL2_CORE_ASSERT(s_Instance == nullptr, "EventDispatcher s_Instance is not null! EventDispatcher::Initialize has been called twice.");
		s_Instance = new EventDispatcher;

		uint32_t entiyMapBytes = FixedHashMap<std::type_index, FixedArray<EventPayload>>::RequiredBytes(MaxEventTypes);

		uint64_t totalBytes = ArenaLayout::Create()
			.Add<EventPayload>(MaxEventTypes * MaxListenersPerEventType)
			.AddRaw(entiyMapBytes, alignof(std::pair<std::type_index, FixedArray<EventPayload>>))
			.Total();

		Get().m_Reservation = Allocator::Arena.Reserve("EventDispatcherPool", totalBytes);
		Get().m_Arena.Initialize(&Allocator::Arena, totalBytes, Get().m_Reservation);

		Get().m_CallbackSlots = FixedHashMap<std::type_index, FixedArray<EventPayload>>(&Get().m_Arena, MaxEventTypes);
	}

	void EventDispatcher::Shutdown()
	{
		for (auto& [type, callbacks] : s_Instance->m_CallbackSlots)
		{
			while (callbacks.size() > 0)
			{
				callbacks.pop_back();
			}
		}

		delete s_Instance;
		s_Instance = nullptr;
	}

	void EventDispatcher::Flush()
	{
		HBL2_CORE_ASSERT(m_DispatchDepth == 0, "Flush must not run inside a dispatch.");

		if (m_DeadCount == 0)
		{
			return;
		}

		for (auto& [type, callbacks] : m_CallbackSlots)
		{
			// Stable compaction that keeps registration order.
			size_t write = 0;
			const size_t count = callbacks.size();

			for (size_t read = 0; read < count; read++)
			{
				if (callbacks[read].Dead)
				{
					continue;
				}

				if (write != read)
				{
					callbacks[write] = std::move(callbacks[read]);
				}
				write++;
			}

			// Destroy the tail so captured state is released.
			while (callbacks.size() > write)
			{
				callbacks.pop_back();
			}
		}

		m_DeadCount = 0;
	}

	FixedArray<EventPayload>& EventDispatcher::GetOrCreateSlot(std::type_index type)
	{
		auto it = m_CallbackSlots.find(type);
		if (it != m_CallbackSlots.end())
		{
			return it->second;
		}

		m_CallbackSlots[type] = FixedArray<EventPayload>(&m_Arena, MaxListenersPerEventType);
		return m_CallbackSlots[type];
	}
}
