#pragma once

#include "Humble2API.h"

#include <entt.hpp>

namespace HBL2
{
	/**
	 * @brief A lightweight wrapper over an entity.
	 */
	struct HBL2_API Entity
	{
		entt::entity Handle{ entt::null };

		constexpr Entity() noexcept = default;
		constexpr Entity(const Entity&) noexcept = default;
		constexpr Entity(Entity&&) noexcept = default;
		Entity& operator=(const Entity&) noexcept = default;
		Entity& operator=(Entity&&) noexcept = default;

		constexpr Entity(uint32_t e) noexcept : Handle((entt::entity)e) {}
		constexpr Entity(uint64_t e) noexcept : Handle((entt::entity)e) {}
		constexpr Entity(entt::entity e) noexcept : Handle(e) {}
		constexpr Entity(entt::null_t) noexcept : Handle(entt::null) {}

		constexpr operator entt::entity() const noexcept { return Handle; }
		constexpr operator entt::null_t() const noexcept { return entt::null; }
		constexpr operator uint64_t() const noexcept { return (uint64_t)Handle; }
		constexpr operator uint32_t() const noexcept { return (uint32_t)Handle; }

		constexpr bool operator==(Entity rhs) const noexcept { return Handle == rhs.Handle; }
		constexpr bool operator!=(Entity rhs) const noexcept { return Handle != rhs.Handle; }
		constexpr bool operator==(entt::entity rhs) const noexcept { return Handle == rhs; }
		constexpr bool operator!=(entt::entity rhs) const noexcept { return Handle != rhs; }
		constexpr bool operator==(entt::null_t) const noexcept { return Handle == entt::null; }
		constexpr bool operator!=(entt::null_t) const noexcept { return Handle != entt::null; }

		static const Entity Null;
	};
}

namespace std
{
	template<>
	struct hash<HBL2::Entity>
	{
		size_t operator()(HBL2::Entity e) const noexcept
		{
			return hash<entt::entity>()(static_cast<entt::entity>(e));
		}
	};
}