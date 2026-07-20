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

    MTL::VertexFormat MtlUtils::VertexFormatToMTLVertexFormat(VertexFormat vertexFormat)
    {
        switch (vertexFormat)
        {
        case HBL2::VertexFormat::FLOAT32:
            return MTL::VertexFormatFloat;
        case HBL2::VertexFormat::FLOAT32x2:
            return MTL::VertexFormatFloat2;
        case HBL2::VertexFormat::FLOAT32x3:
            return MTL::VertexFormatFloat3;
        case HBL2::VertexFormat::FLOAT32x4:
            return MTL::VertexFormatFloat4;
        case HBL2::VertexFormat::INT32:
            return MTL::VertexFormatInt;
        case HBL2::VertexFormat::INT32x2:
            return MTL::VertexFormatInt2;
        case HBL2::VertexFormat::INT32x3:
            return MTL::VertexFormatInt3;
        case HBL2::VertexFormat::INT32x4:
            return MTL::VertexFormatInt4;
        case HBL2::VertexFormat::UINT32:
            return MTL::VertexFormatUInt;
        case HBL2::VertexFormat::UINT32x2:
            return MTL::VertexFormatUInt2;
        case HBL2::VertexFormat::UINT32x3:
            return MTL::VertexFormatUInt3;
        case HBL2::VertexFormat::UINT32x4:
            return MTL::VertexFormatUInt4;
        case HBL2::VertexFormat::NONE:
            break;
        }

        return MTL::VertexFormatInvalid;
    }

    MTL::PrimitiveTopologyClass MtlUtils::TopologyToMTLPrimitiveTopologyClass(Topology topology)
    {
        switch (topology)
        {
        case HBL2::Topology::POINT_LIST:
            return MTL::PrimitiveTopologyClassPoint;
        case HBL2::Topology::LINE_LIST:
            return MTL::PrimitiveTopologyClassLine;
        case HBL2::Topology::TRIANGLE_LIST:
            return MTL::PrimitiveTopologyClassTriangle;
        case HBL2::Topology::LINE_STRIP:
        case HBL2::Topology::TRIANGLE_STRIP:
        case HBL2::Topology::TRIANGLE_FAN:
        case HBL2::Topology::PATCH_LIST:
            return MTL::PrimitiveTopologyClassUnspecified;
        }

        return (MTL::PrimitiveTopologyClass)NS::UInteger(-1);
    }

    MTL::PrimitiveType MtlUtils::TopologyToMTLPrimitiveType(Topology topology)
    {
        switch (topology)
        {
        case HBL2::Topology::POINT_LIST:
            return MTL::PrimitiveTypePoint;
        case HBL2::Topology::LINE_LIST:
            return MTL::PrimitiveTypeLine;
        case HBL2::Topology::TRIANGLE_LIST:
            return MTL::PrimitiveTypeTriangle;
        case HBL2::Topology::LINE_STRIP:
            return MTL::PrimitiveTypeLineStrip;
        case HBL2::Topology::TRIANGLE_STRIP:
            return MTL::PrimitiveTypeTriangleStrip;
        case HBL2::Topology::TRIANGLE_FAN:
        case HBL2::Topology::PATCH_LIST:
            break;
        }

        return (MTL::PrimitiveType)NS::UInteger(-1);
    }

    MTL::Winding MtlUtils::FrontFaceToMTLWinding(FrontFace frontFace)
    {
        // NOTE: Front face inverted to match vulkan setup.
        
        switch (frontFace)
        {
        case HBL2::FrontFace::COUNTER_CLOCKWISE:
            return MTL::WindingClockwise;
        case HBL2::FrontFace::CLOCKWISE:
            return MTL::WindingCounterClockwise;
        }

        return (MTL::Winding)NS::UInteger(-1);
    }

    MTL::CullMode MtlUtils::CullModeToMTLCullMode(CullMode cullMode)
    {
        switch (cullMode)
        {
        case HBL2::CullMode::NONE:
            return MTL::CullModeNone;
        case HBL2::CullMode::FRONT:
            return MTL::CullModeFront;
        case HBL2::CullMode::BACK:
            return MTL::CullModeBack;
        case HBL2::CullMode::FRONT_AND_BACK:
            return MTL::CullModeBack; // NOTE: FrontAndBack is missing!
        }

        return (MTL::CullMode)NS::UInteger(-1);
    }

    MTL::TriangleFillMode MtlUtils::PolygonModeToMTLTriangleFillMode(PolygonMode polygonMode)
    {
        switch (polygonMode)
        {
        case HBL2::PolygonMode::FILL:
            return MTL::TriangleFillModeFill;
        case HBL2::PolygonMode::LINE:
            return MTL::TriangleFillModeLines;
        case HBL2::PolygonMode::POINT:
            return MTL::TriangleFillModeFill; // NOTE: Points is missing!
        }

        return (MTL::TriangleFillMode)NS::UInteger(-1);
    }

    MTL::ResourceOptions MtlUtils::MemoryUsageToMTLResourceOptions(MemoryUsage memoryUsage)
    {
        switch (memoryUsage)
        {
        case HBL2::MemoryUsage::CPU_ONLY:
            // Host-visible, coherent, normally cached — general-purpose staging/CPU-owned data.
            return MTL::ResourceStorageModeShared | MTL::ResourceCPUCacheModeDefaultCache;
        case HBL2::MemoryUsage::GPU_ONLY:
            // GPU-exclusive, not CPU-visible at all — fastest for static VBs/IBs, render targets.
            // return MTL::ResourceStorageModePrivate; // TODO: fix when the buffer is fixed.
            return MTL::ResourceStorageModeShared;
        case HBL2::MemoryUsage::GPU_CPU:
            // GPU writes, CPU reads back (e.g. readback/query buffers).
            // Default cache mode matters here — write-combined is uncached for CPU *reads* and would be brutal for readback.
            return MTL::ResourceStorageModeShared | MTL::ResourceCPUCacheModeDefaultCache;
        case HBL2::MemoryUsage::CPU_GPU:
            // CPU writes every frame, GPU reads (uniforms, streamed vertex data).
            // Write-combined optimizes sequential CPU writes; fine since CPU never reads this back.
            return MTL::ResourceStorageModeShared | MTL::ResourceCPUCacheModeWriteCombined;
        }

        return MTL::ResourceStorageModeShared;
    }

    MTL::BlendOperation MtlUtils::BlendOperationToMTLBlendOperation(BlendOperation blendOperation)
    {
        switch (blendOperation)
        {
        case HBL2::BlendOperation::ADD:
            return MTL::BlendOperationAdd;
        case HBL2::BlendOperation::MUL:
            return MTL::BlendOperationUnspecialized; // Multiply not available!
        case HBL2::BlendOperation::SUB:
                return MTL::BlendOperationSubtract;
        case HBL2::BlendOperation::MIN:
                return MTL::BlendOperationMin;
        case HBL2::BlendOperation::MAX:
            return MTL::BlendOperationMax;
        }

        return MTL::BlendOperationUnspecialized;
    }

    MTL::BlendFactor MtlUtils::BlendFactorToMTLBlendFactor(BlendFactor blendFactor)
    {
        switch (blendFactor)
        {
        case HBL2::BlendFactor::SRC_ALPHA:
            return MTL::BlendFactorSourceAlpha;
        case HBL2::BlendFactor::ONE_MINUS_SRC_ALPHA:
            return MTL::BlendFactorOneMinusSourceAlpha;
        case HBL2::BlendFactor::ONE:
            return MTL::BlendFactorOne;
        case HBL2::BlendFactor::ZERO:
            return MTL::BlendFactorZero;
        }

        return MTL::BlendFactorUnspecialized;
    }

    MTL::Stages MtlUtils::TextureLayoutToMTLStage(TextureLayout layout)
    {
        switch (layout)
        {
        case TextureLayout::RENDER_ATTACHMENT:
        case TextureLayout::DEPTH_STENCIL:
            return MTL::StageFragment;
        case TextureLayout::SHADER_READ_ONLY:
            return MTL::StageFragment | MTL::StageVertex;
        default:
            return MTL::StageFragment;
        }
    }
}
