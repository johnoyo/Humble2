#pragma once

#include "Base.h"
#include "Renderer/Enums.h"

#include "Utilities/Collections/BitFlags.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

namespace HBL2
{
    namespace MtlUtils
    {
        MTL::LoadAction LoadOperationToMTLLoadAction(LoadOperation loadOperation);
        MTL::StoreAction StoreOperationMTLStoreAction(StoreOperation storeOperation);
        MTL::TextureType TextureTypeToMTLTextureType(TextureType textureType);
        MTL::PixelFormat FormatToMTLPixelFormat(Format format);
        MTL::TextureUsage TextureUsageFlagToMTLTextureUsage(TextureUsage usage);
        MTL::TextureUsage TextureUsageFlagsToMTLTextureUsage(BitFlags<TextureUsage> usageFlags);
        MTL::SamplerMinMagFilter FilterToMTLSamplerMinMagFilter(TextureFilter filter);
        MTL::SamplerAddressMode WrapToMTLSamplerAddressMode(Wrap wrap);
        MTL::CompareFunction CompareToMTLCompareFunction(Compare compare);
    }
}
