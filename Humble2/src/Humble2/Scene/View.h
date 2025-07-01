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

        constexpr auto begin() const noexcept { return m_View.cbegin(); }
        constexpr auto end()   const noexcept { return m_View.cend(); }

    private:
        ViewType m_View;
    };
}