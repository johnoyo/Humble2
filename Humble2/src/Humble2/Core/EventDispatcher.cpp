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
	}

	void EventDispatcher::Shutdown()
	{
		delete s_Instance;
		s_Instance = nullptr;
	}
}
