#pragma once

#include <string>
#include <functional>
#include <unordered_map>

namespace HBL2
{
	class Application;

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

		void Initialize(Application* app);

		using Slot = std::function<void(const Event&)>;

		void Register(const std::string& descriptor, Slot&& callback);
		void Post(const Event& event) const;

	private:
		EventDispatcher() = default;

		Application* m_App = nullptr;
		std::unordered_map<std::string, std::vector<Slot>> m_Slots;
	};
}