#pragma once

#include <entt.hpp>

#include <utility>

namespace HBL2
{
    template <typename TGroup>
    class Group
    {
    public:
        using GroupType = std::decay_t<TGroup>;

        explicit constexpr Group(TGroup group) noexcept : m_Group{ std::move(group) } {}

        template <typename F>
        constexpr void Each(F&& f) const
        {
            m_Group.each(std::forward<F>(f));
        }

        constexpr auto begin() noexcept { return m_Group.begin(); }
        constexpr auto end() noexcept { return m_Group.end(); }

        constexpr const auto begin() const noexcept { return m_Group.begin(); }
        constexpr const auto end() const noexcept { return m_Group.end(); }

        constexpr std::size_t Size() const noexcept { return m_Group.size(); }

    private:
        GroupType m_Group;
    };
}