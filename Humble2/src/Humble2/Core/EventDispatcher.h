#pragma once

#include "Base.h"
#include "Allocators.h"
#include "EventDispatcher.h"
#include "Resources/Pool.h"
#include "Utilities/Random.h"
#include "Utilities/Allocators/Arena.h"
#include "Utilities/Collections/FixedArray.h"
#include "Utilities/Collections/FixedHashMap.h"
#include "Utilities/Collections/StaticFunction.h"

#include <typeindex>

namespace HBL2
{
    class HBL2_API Event
    {
    public:
        ~Event() = default;
    };

    template <typename T>
    class EventType : public Event {};

    struct HBL2_API EventPayload
    {
        EventPayload() = default;
        EventPayload(const EventPayload&) = delete;
        EventPayload& operator=(const EventPayload&) = delete;
        EventPayload(EventPayload&&) noexcept = default;
        EventPayload& operator=(EventPayload&&) noexcept = default;
        ~EventPayload() = default;

        UUID Id = 0;
        StaticFunction<void(const Event&), 64> Callback;
        bool OneShot = false;
        bool Dead = false;
    };

    class HBL2_API EventDispatcher
    {
        static constexpr uint32_t MaxEventTypes = 256;
        static constexpr uint32_t MaxEntriesPerEventType = 1024;

    public:
        EventDispatcher(const EventDispatcher&) = delete;
        static EventDispatcher& Get();

        static void Initialize();
        static void Shutdown();

        template <typename T, typename F>
        UUID Register(F&& callback)
        {
            static_assert(std::is_base_of_v<Event, T>, "T must derive from Event.");
            static_assert(std::is_invocable_v<F&, const T&>, "Callback must be callable with (const T&).");

            std::type_index type = std::type_index(typeid(T));
            UUID id = Random::UInt64();

            EventPayload payload;
            payload.Id = id;
            payload.OneShot = false;
            payload.Dead = false;

            auto wrapper = [cb = std::forward<F>(callback)](const Event& e) mutable
            {
                cb(static_cast<const T&>(e));
            };

            payload.Callback = std::move(wrapper);

            auto& slot = GetOrCreateSlot(type);
            HBL2_CORE_ASSERT(slot.size() < MaxEntriesPerEventType, "Too many listeners for this event type.");
            slot.emplace_back(std::move(payload));

            return id;
        }

        template <typename T, typename F>
        UUID RegisterOneShot(F&& callback)
        {
            static_assert(std::is_base_of_v<Event, T>, "T must derive from Event.");
            static_assert(std::is_invocable_v<F&, const T&>, "Callback must be callable with (const T&).");

            std::type_index type = std::type_index(typeid(T));
            UUID id = Random::UInt64();

            EventPayload payload;
            payload.Id = id;
            payload.OneShot = true;
            payload.Dead = false;

            auto wrapper = [cb = std::forward<F>(callback)](const Event& e) mutable
            {
                cb(static_cast<const T&>(e));
            };

            payload.Callback = std::move(wrapper);

            auto& slot = GetOrCreateSlot(type);
            HBL2_CORE_ASSERT(slot.size() < MaxEntriesPerEventType, "Too many listeners for this event type.");
            slot.emplace_back(std::move(payload));

            return id;
        }

        template <typename T>
        bool Deregister(UUID id)
        {
            static_assert(std::is_base_of_v<Event, T>, "T must derive from Event.");

            auto it = m_CallbackSlots.find(std::type_index(typeid(T)));
            if (it == m_CallbackSlots.end())
            {
                return false;
            }

            for (EventPayload& p : it->second)
            {
                if (p.Id == id && !p.Dead)
                {
                    p.Dead = true;
                    m_DeadCount++;

                    return true;
                }
            }

            return false;
        }

        template <typename T>
        void Post(const T& event)
        {
            static_assert(std::is_base_of_v<Event, T>, "T must derive from Event.");

            auto it = m_CallbackSlots.find(std::type_index(typeid(T)));
            if (it == m_CallbackSlots.end())
            {
                return;
            }

            auto& callbacks = it->second;
            
            // Listeners added by callbacks start next Post.
            const size_t count = callbacks.size();

            m_DispatchDepth++;

            for (size_t i = 0; i < count; i++)
            {
                EventPayload& listener = callbacks[i];

                if (listener.Dead)
                {
                    continue;
                }

                if (listener.OneShot)
                {
                    // Before invoking, so a recursive Post can not re-fire it.
                    listener.Dead = true;
                    m_DeadCount++;
                }

                listener.Callback(event);
            }

            m_DispatchDepth--;
        }

        void Flush();

    private:
        EventDispatcher() = default;
        FixedArray<EventPayload>& GetOrCreateSlot(std::type_index type);

    private:
        PoolReservation* m_Reservation = nullptr;
        Arena m_Arena;

        FixedHashMap<std::type_index, FixedArray<EventPayload>> m_CallbackSlots;

        uint32_t m_DispatchDepth = 0;
        uint32_t m_DeadCount = 0;

        static EventDispatcher* s_Instance;
    };
}
