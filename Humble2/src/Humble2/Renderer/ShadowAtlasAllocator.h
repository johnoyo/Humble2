#pragma once

// Inspired by the technique used in doom: https://www.adriancourreges.com/blog/2016/09/09/doom-2016-graphics-study/

#include "Core\Allocators.h"
#include "Utilities\Collections\Collections.h"

#include <glm\glm.hpp>

namespace HBL2
{
    constexpr uint16_t g_ShadowAtlasSize = 8192;
    constexpr uint16_t g_TileSize = 2048;
    constexpr uint16_t g_TilesPerRow = g_ShadowAtlasSize / g_TileSize; // = 4
    constexpr uint16_t g_MaxTiles = g_TilesPerRow * g_TilesPerRow;     // = 16

    struct ShadowTile
    {
        uint16_t x;
        uint16_t y;

        glm::vec4 GetUVRange();

        static ShadowTile Invalid;

        bool operator==(const ShadowTile& other) const
        {
            return other.x == x && other.y == y;
        }
    };

    class ShadowAtlasAllocator
    {
    public:
        ShadowAtlasAllocator();

        ShadowTile AllocateTile();
        void FreeTile(ShadowTile tile);
        void Clear();

    private:
        DArray<ShadowTile> m_FreeTiles = MakeEmptyDArray<ShadowTile>();

        PoolReservation* m_Reservation = nullptr;
        Arena m_Arena;
    };
}