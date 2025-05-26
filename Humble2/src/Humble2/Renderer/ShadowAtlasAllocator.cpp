#include "ShadowAtlasAllocator.h"
#include "Renderer.h"

namespace HBL2
{
    ShadowTile ShadowTile::Invalid = { UINT16_MAX, UINT16_MAX };

    glm::vec4 ShadowTile::GetUVRange()
    {
        float scaleY = (float)(g_TileSize) / g_ShadowAtlasSize;
        float offsetY = (float)(y * g_TileSize) / g_ShadowAtlasSize;

        switch (Renderer::Instance->GetAPI())
        {
        case GraphicsAPI::OPENGL:
            return
            {
                (float)(x * g_TileSize) / g_ShadowAtlasSize,    // offset X
                offsetY,                                        // offset Y
                (float)(g_TileSize) / g_ShadowAtlasSize,        // scale X
                scaleY                                          // scale Y
            };
        case GraphicsAPI::VULKAN:
            return
            {
                (float)(x * g_TileSize) / g_ShadowAtlasSize,    // offset X
                offsetY + scaleY,                               // offset Y
                (float)(g_TileSize) / g_ShadowAtlasSize,        // scale X
                -scaleY                                         // scale Y
            };
        }
    }

    ShadowAtlasAllocator::ShadowAtlasAllocator()
    {
        freeTiles.reserve(g_MaxTiles);

        for (uint16_t i = 0; i < g_TilesPerRow; ++i)
        {
            for (uint16_t j = 0; j < g_TilesPerRow; ++j)
            {
                freeTiles.push_back({ i, j });
            }
        }
    }

    ShadowTile ShadowAtlasAllocator::AllocateTile()
    {
        if (freeTiles.empty())
        {
            return ShadowTile::Invalid;
        }

        ShadowTile tile = freeTiles.back();
        freeTiles.pop_back();
        return tile;
    }

    void ShadowAtlasAllocator::FreeTile(ShadowTile tile)
    {
        freeTiles.push_back(tile);
    }

    void ShadowAtlasAllocator::Clear()
    {
        freeTiles.clear();

        for (uint16_t i = 0; i < g_TilesPerRow; ++i)
        {
            for (uint16_t j = 0; j < g_TilesPerRow; ++j)
            {
                freeTiles.push_back({ i, j });
            }
        }
    }
}
