#pragma once

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
			static EventDispatcher instance;
			return instance;
		}

		void Initialize();
		void Register(const std::string& descriptor, std::function<void(const Event&)>&& callback);
		void Post(const Event& event) const;

	private:
		EventDispatcher() = default;

		std::unordered_map<std::string, std::vector<std::function<void(const Event&)>>> m_Callbacks;
	};
}