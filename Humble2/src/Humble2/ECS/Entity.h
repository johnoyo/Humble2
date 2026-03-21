#pragma once

#include "Utilities\Allocators\Arena.h"

namespace HBL2
{
	struct EntityRef
	{
		int32_t Idx;
		int32_t Gen;

        constexpr bool operator==(const EntityRef& o) const { return Idx == o.Idx && Gen == o.Gen; }
        constexpr bool operator!=(const EntityRef& o) const { return Idx != o.Idx || Gen != o.Gen; }

        static const EntityRef Null;
	};
}

template<>
struct std::hash<HBL2::EntityRef>
{
    std::size_t operator()(const HBL2::EntityRef& e) const noexcept
    {
        std::size_t h = static_cast<std::size_t>(e.Idx);
        h ^= static_cast<std::size_t>(e.Gen) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};