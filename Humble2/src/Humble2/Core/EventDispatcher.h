#pragma once

#include "Base.h"

#include <string>
#include <functional>
#include <unordered_map>

namespace HBL2
{
	class Event {
	public:
		virtual ~Event();
		virtual std::string GetDescription() const = 0;
	};

	class EventDispatcher {
	public:
		EventDispatcher(const EventDispatcher&) = delete;

		static EventDispatcher& Get()
		{
			HBL2_CORE_ASSERT(s_Instance != nullptr, "EventDispatcher s_Instance is null! Call EventDispatcher::Initialize before use.");
			return *s_Instance;
		}

		static void Initialize();
		static void Shutdown();

		void Register(const std::string& descriptor, std::function<void(const Event&)>&& callback);
		void Post(const Event& event) const;

	private:
		EventDispatcher() = default;

		std::unordered_map<std::string, std::vector<std::function<void(const Event&)>>> m_CallbackSlots;

		inline static EventDispatcher* s_Instance = nullptr;
	};
}