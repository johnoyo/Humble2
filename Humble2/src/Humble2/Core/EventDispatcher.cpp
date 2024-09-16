#include "EventDispatcher.h"

namespace HBL2
{
	Event::~Event()
	{
	}

	void EventDispatcher::Initialize()
	{
	}

	void EventDispatcher::Register(const std::string& descriptor, std::function<void(const Event&)>&& callback)
	{
		m_Callbacks[descriptor].push_back(callback);
	}

	void EventDispatcher::Post(const Event& event) const
	{
		auto desc = event.GetDescription();
		if (m_Callbacks.find(desc) == m_Callbacks.end())
		{
			return;
		}

		auto&& observers = m_Callbacks.at(desc);

		for (auto&& observer : observers)
		{
			observer(event);
		}
	}
}
