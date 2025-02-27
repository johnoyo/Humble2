#pragma once

#include "Base.h"

#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <typeindex>

namespace HBL2
{
    class HBL2_API Event
    {
    public:
        virtual ~Event() = default;
    };

    template <typename T>
    class EventType : public Event
    {
    public:
        static std::type_index GetType() { return typeid(T); }
    };

    class HBL2_API EventDispatcher
    {
    public:
        EventDispatcher(const EventDispatcher&) = delete;
        static EventDispatcher& Get();

        static void Initialize();
        static void Shutdown();

        template <typename T>
        void Register(std::function<void(const T&)>&& callback, bool oneShot = false)
        {
            static_assert(std::is_base_of_v<Event, T>, "T must derive from Event.");

            std::type_index type = std::type_index(typeid(T));

            auto wrapper = [cb = std::forward<std::function<void(const T&)>>(callback)](const Event& e)
            {
                cb(static_cast<const T&>(e));
            };

            if (oneShot)
            {
                m_OneShotCallbacks[type].push_back(std::move(wrapper));
            }
            else
            {
                m_CallbackSlots[type].push_back(std::move(wrapper));
            }
        }

        template <typename T>
        void Post(const T& event)
        {
            static_assert(std::is_base_of_v<Event, T>, "T must derive from Event.");

            auto it = m_CallbackSlots.find(std::type_index(typeid(T)));
            if (it != m_CallbackSlots.end())
            {
                for (auto&& callback : it->second)
                {
                    callback(event);
                }
            }

            auto oneShotIt = m_OneShotCallbacks.find(std::type_index(typeid(T)));
            if (oneShotIt != m_OneShotCallbacks.end())
            {
                for (auto&& callback : oneShotIt->second)
                {
                    callback(event);
                }
                m_OneShotCallbacks.erase(oneShotIt); // Remove one-shot callbacks
            }
        }

    private:
        EventDispatcher() = default;

        std::unordered_map<std::type_index, std::vector<std::function<void(const Event&)>>> m_CallbackSlots;
        std::unordered_map<std::type_index, std::vector<std::function<void(const Event&)>>> m_OneShotCallbacks;

        static EventDispatcher* s_Instance;
    };
}