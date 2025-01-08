#pragma once

#include "Base.h"

#include <string>
#include <functional>
#include <unordered_map>

namespace HBL2
{
	class HBL2_API Event {
	public:
		virtual ~Event();
		virtual std::string GetDescription() const = 0;
	};

	class HBL2_API EventDispatcher {
	public:
		EventDispatcher(const EventDispatcher&) = delete;

		static EventDispatcher& Get();

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