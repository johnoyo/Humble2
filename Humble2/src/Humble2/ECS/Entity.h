#pragma once

#include "Humble2API.h"

#include <type_traits>

namespace HBL2
{
    /**
     * @brief An opaque handle that represents an entity.
     */
	struct HBL2_API Entity
	{
		int32_t Idx;
		int32_t Gen;

        uint64_t Pack() const { return (static_cast<uint64_t>(static_cast<uint32_t>(Gen)) << 32) | static_cast<uint64_t>(static_cast<uint32_t>(Idx)); }
        static Entity UnPack(uint64_t packedEntity) { return Entity{ static_cast<int32_t>(packedEntity & 0xFFFFFFFF), static_cast<int32_t>((packedEntity >> 32) & 0xFFFFFFFF) }; }

        constexpr bool operator==(const Entity& o) const { return Idx == o.Idx && Gen == o.Gen; }
        constexpr bool operator!=(const Entity& o) const { return Idx != o.Idx || Gen != o.Gen; }

        static const Entity Null;
	};
}

template<>
struct std::hash<HBL2::Entity>
{
    std::size_t operator()(const HBL2::Entity& e) const noexcept
    {
        std::size_t h = static_cast<std::size_t>(e.Idx);
        h ^= static_cast<std::size_t>(e.Gen) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};