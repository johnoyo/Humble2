#include "EventDispatcher.h"

namespace HBL2
{
	Event::~Event()
	{
	}

	EventDispatcher& EventDispatcher::Get()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "EventDispatcher s_Instance is null! Call EventDispatcher::Initialize before use.");
		return *s_Instance;
	}

	void EventDispatcher::Initialize()
	{
		HBL2_CORE_ASSERT(s_Instance == nullptr, "EventDispatcher s_Instance is not null! EventDispatcher::Initialize has been called twice.");
		s_Instance = new EventDispatcher;
	}

	void EventDispatcher::Shutdown()
	{
		delete s_Instance;
		s_Instance = nullptr;
	}

	void EventDispatcher::Register(const std::string& descriptor, std::function<void(const Event&)>&& callback)
	{
		m_CallbackSlots[descriptor].push_back(callback);
	}

	void EventDispatcher::Post(const Event& event) const
	{
		auto desc = event.GetDescription();
		if (m_CallbackSlots.find(desc) == m_CallbackSlots.end())
		{
			return;
		}

		auto&& callbacks = m_CallbackSlots.at(desc);

		for (auto&& callback : callbacks)
		{
			callback(event);
		}
	}
}
