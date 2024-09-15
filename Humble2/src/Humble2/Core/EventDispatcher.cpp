#include "EventDispatcher.h"

namespace HBL2
{
	Event::~Event()
	{
	}

	void EventDispatcher::Initialize(Application* app)
	{
		m_App = app;
	}

	void EventDispatcher::Register(const std::string& descriptor, Slot&& callback)
	{
		m_Slots[descriptor].push_back(callback);
	}

	void EventDispatcher::Post(const Event& event) const
	{
		auto desc = event.GetDescription();
		if (m_Slots.find(desc) == m_Slots.end())
		{
			return;
		}

		auto&& observers = m_Slots.at(desc);

		for (auto&& observer : observers)
		{
			observer(event);
		}
	}
}
