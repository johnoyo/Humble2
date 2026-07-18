#include "MetalCommon.h"

namespace HBL2
{
    MTL::LoadAction MtlUtils::LoadOperationToMTLLoadAction(LoadOperation loadOperation)
    {
        switch (loadOperation)
        {
        case HBL2::LoadOperation::CLEAR:
            return MTL::LoadActionClear;
        case HBL2::LoadOperation::LOAD:
            return MTL::LoadActionLoad;
        case HBL2::LoadOperation::DONT_CARE:
            return MTL::LoadActionDontCare;
        }

        return (MTL::LoadAction)NS::UInteger(-1);
    }

    MTL::StoreAction MtlUtils::StoreOperationMTLStoreAction(StoreOperation storeOperation)
    {
        switch (storeOperation)
        {
        case HBL2::StoreOperation::STORE:
            return MTL::StoreActionStore;
        case HBL2::StoreOperation::DONT_CARE:
            return MTL::StoreActionDontCare;
        }

        return (MTL::StoreAction)NS::UInteger(-1);
    }

    MTL::TextureType MtlUtils::TextureTypeToMTLTextureType(TextureType textureType)
    {
        switch (textureType)
        {
        case HBL2::TextureType::D1:
            return MTL::TextureType1D;
        case HBL2::TextureType::D2:
            return MTL::TextureType2D;
        case HBL2::TextureType::D3:
                return MTL::TextureType3D;
        case HBL2::TextureType::D2_ARRAY:
                return MTL::TextureType2DArray;
        case HBL2::TextureType::CUBE:
            return MTL::TextureTypeCube;
        }

        return (MTL::TextureType)NS::UInteger(-1);
    }

    MTL::PixelFormat MtlUtils::FormatToMTLPixelFormat(Format format)
    {
        switch (format)
        {
            case Format::D32_FLOAT:
                return MTL::PixelFormatDepth32Float;
            case Format::RGBA8_RGB:
                return MTL::PixelFormatRGBA8Unorm_sRGB;
            case Format::RGBA8_UNORM:
                return MTL::PixelFormatRGBA8Unorm;
            case Format::BGRA8_UNORM:
                return MTL::PixelFormatBGRA8Unorm;
            case Format::RG16_FLOAT:
                return MTL::PixelFormatRG16Float;
            case Format::RGBA16_FLOAT:
                return MTL::PixelFormatRGBA16Float;
            case Format::RGB32_FLOAT:
                return MTL::PixelFormatInvalid;
            case Format::RGBA32_FLOAT:
                return MTL::PixelFormatRGBA32Float;
            case Format::R10G10B10A2_UNORM:
                return MTL::PixelFormatRGB10A2Unorm;
            default:
                break;
        }

        return MTL::PixelFormatInvalid;
    }

    MTL::TextureUsage MtlUtils::TextureUsageFlagToMTLTextureUsage(TextureUsage usage)
    {
        switch (usage)
        {
        case HBL2::TextureUsage::TEXTURE_BINDING:
            return MTL::TextureUsageShaderRead;
        case HBL2::TextureUsage::STORAGE_BINDING:
            return MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite;
        case HBL2::TextureUsage::RENDER_ATTACHMENT:
            return MTL::TextureUsageRenderTarget;
        case HBL2::TextureUsage::DEPTH_STENCIL:
            return MTL::TextureUsageRenderTarget;
        default:
            break;
        }

        return MTL::TextureUsageUnknown;
    }

    MTL::TextureUsage MtlUtils::TextureUsageFlagsToMTLTextureUsage(BitFlags<TextureUsage> usageFlags)
    {
        MTL::TextureUsage mtlUsage = MTL::TextureUsageUnknown;

        if (usageFlags.IsSet(HBL2::TextureUsage::TEXTURE_BINDING))
        {
            mtlUsage |= MTL::TextureUsageShaderRead;
        }

        if (usageFlags.IsSet(HBL2::TextureUsage::STORAGE_BINDING))
        {
            mtlUsage |= MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite;
        }

        if (usageFlags.IsSet(HBL2::TextureUsage::RENDER_ATTACHMENT) || usageFlags.IsSet(HBL2::TextureUsage::DEPTH_STENCIL))
        {
            mtlUsage |= MTL::TextureUsageRenderTarget;
        }

        return mtlUsage;
    }

    MTL::SamplerMinMagFilter MtlUtils::FilterToMTLSamplerMinMagFilter(TextureFilter filter)
    {
        switch (filter)
        {
        case HBL2::TextureFilter::NEAREST:
            return MTL::SamplerMinMagFilterNearest;
        case HBL2::TextureFilter::LINEAR:
            return MTL::SamplerMinMagFilterLinear;
        case HBL2::TextureFilter::CUBIC:
            return MTL::SamplerMinMagFilterLinear;
        }
    
        return (MTL::SamplerMinMagFilter)NS::UInteger(-1);;
    }

    MTL::SamplerAddressMode MtlUtils::WrapToMTLSamplerAddressMode(Wrap wrap)
    {
        switch (wrap)
        {
        case HBL2::Wrap::REPEAT:
            return MTL::SamplerAddressModeRepeat;
        case HBL2::Wrap::REPEAT_MIRRORED:
            return MTL::SamplerAddressModeMirrorRepeat;
        case HBL2::Wrap::CLAMP_TO_EDGE:
            return MTL::SamplerAddressModeClampToEdge;
        case HBL2::Wrap::CLAMP_TO_BORDER:
            return MTL::SamplerAddressModeClampToBorderColor;
        case HBL2::Wrap::MIRROR_CLAMP_TO_EDGE:
            return MTL::SamplerAddressModeMirrorClampToEdge;
        }

        return (MTL::SamplerAddressMode)NS::UInteger(-1);
    }

    MTL::CompareFunction MtlUtils::CompareToMTLCompareFunction(Compare compare)
    {
        switch (compare)
        {
        case HBL2::Compare::ALAWAYS:
            return MTL::CompareFunctionAlways;
        case HBL2::Compare::NEVER:
            return MTL::CompareFunctionNever;
        case HBL2::Compare::LESS:
            return MTL::CompareFunctionLess;
        case HBL2::Compare::LESS_OR_EQUAL:
            return MTL::CompareFunctionLessEqual;
        case HBL2::Compare::GREATER:
            return MTL::CompareFunctionGreater;
        case HBL2::Compare::GREATER_OR_EQUAL:
            return MTL::CompareFunctionGreaterEqual;
        case HBL2::Compare::EQUAL:
            return MTL::CompareFunctionEqual;
        case HBL2::Compare::NOT_EQUAL:
            return MTL::CompareFunctionNotEqual;
        }

        return (MTL::CompareFunction)NS::UInteger(-1);
    }
}
