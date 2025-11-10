#pragma once

#include <entt.hpp>

#include <utility>

namespace HBL2
{
    template <typename TView>
    class View
    {
    public:
        using ViewType = std::decay_t<TView>;

        constexpr explicit View(TView view) noexcept : m_View{ std::move(view) } {}

        template <typename F>
        constexpr void Each(F&& f) const
        {
            m_View.each(std::forward<F>(f));
        }

        inline auto Each() const
        {
            return m_View.each();
        }

        constexpr auto begin() noexcept { return m_View.begin(); }
        constexpr auto end() noexcept { return m_View.end(); }

        constexpr const auto begin() const noexcept { return m_View.begin(); }
        constexpr const auto end() const noexcept { return m_View.end(); }

    private:
        ViewType m_View;
    };
}