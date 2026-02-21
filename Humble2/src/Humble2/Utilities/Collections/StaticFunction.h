#pragma once

#include <type_traits>
#include <utility>
#include <new>
#include <cassert>

namespace HBL2
{
    template<typename Signature, size_t InlineSize>
    class StaticFunction;

    template<typename R, typename... Args, size_t InlineSize>
    class StaticFunction<R(Args...), InlineSize>
    {
    public:
        StaticFunction() noexcept = default;
        StaticFunction(std::nullptr_t) noexcept {}

        ~StaticFunction()
        {
            Reset();
        }

        StaticFunction(StaticFunction&& other) noexcept
        {
            MoveFrom(std::move(other));
        }

        StaticFunction& operator=(StaticFunction&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                MoveFrom(std::move(other));
            }
            return *this;
        }

        StaticFunction(const StaticFunction&) = delete;
        StaticFunction& operator=(const StaticFunction&) = delete;

        template<typename F> requires (!std::is_same_v<std::decay_t<F>, StaticFunction>)
        StaticFunction(F&& f)
        {
            Emplace(std::forward<F>(f));
        }

        template<typename F>
        void Emplace(F&& f)
        {
            using Fn = std::decay_t<F>;

            static_assert(sizeof(Fn) <= InlineSize, "Callable too large for StaticFunction inline storage");
            static_assert(alignof(Fn) <= alignof(Storage), "Callable alignment too strict for StaticFunction");

            new (&m_Storage) Fn(std::forward<F>(f));

            m_Invoke = [](void* storage, Args&&... args) -> R
            {
                return (*reinterpret_cast<Fn*>(storage))(std::forward<Args>(args)...);
            };

            m_Destroy = [](void* storage)
            {
                reinterpret_cast<Fn*>(storage)->~Fn();
            };

            m_Move = [](void* src, void* dst)
            {
                new (dst) Fn(std::move(*reinterpret_cast<Fn*>(src)));
                reinterpret_cast<Fn*>(src)->~Fn();
            };
        }

        void Reset()
        {
            if (m_Destroy)
            {
                m_Destroy(&m_Storage);
                m_Destroy = nullptr;
                m_Invoke = nullptr;
                m_Move = nullptr;
            }
        }

        explicit operator bool() const noexcept
        {
            return m_Invoke != nullptr;
        }

        R operator()(Args... args)
        {
            assert(m_Invoke && "Calling empty StaticFunction");
            return m_Invoke(&m_Storage, std::forward<Args>(args)...);
        }

    private:
        using Storage = std::aligned_storage_t<InlineSize>;

        void MoveFrom(StaticFunction&& other) noexcept
        {
            if (!other.m_Invoke)
            {
                return;
            }

            other.m_Move(&other.m_Storage, &m_Storage);

            m_Invoke = other.m_Invoke;
            m_Destroy = other.m_Destroy;
            m_Move = other.m_Move;

            other.m_Invoke = nullptr;
            other.m_Destroy = nullptr;
            other.m_Move = nullptr;
        }

    private:
        Storage m_Storage;

        R(*m_Invoke)(void*, Args&&...) = nullptr;
        void (*m_Destroy)(void*) = nullptr;
        void (*m_Move)(void*, void*) = nullptr;
    };
}