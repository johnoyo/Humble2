#include "MetalTexture.h"

#include "Platform/Metal/MetalRenderer.h"

namespace HBL2
{
    static BlockFormatInfo GetBlockFormatInfo(Format format)
    {
        switch (format)
        {
        case Format::D32_FLOAT:
        case Format::RGBA8_RGB:
        case Format::RGBA8_UNORM:
        case Format::BGRA8_UNORM:
        case Format::RG16_FLOAT:
            return { 1, 1, 4 };
        case Format::RGBA32_FLOAT:
            return { 1, 1, 16 };
        case Format::RGBA16_FLOAT:
        case Format::RGB32_FLOAT:
            return { 1, 1, 8 };
        case Format::R10G10B10A2_UNORM:
            return { 1, 1, 8 }; // TODO: same open question as the Vulkan version.
        case Format::ASTC_4x4_SRGB:
        case Format::ASTC_4x4_UNORM:
            return { 4, 4, 16 };
        case Format::ASTC_6x6_SRGB:
        case Format::ASTC_6x6_UNORM:
            return { 6, 6, 16 };
        case Format::ASTC_8x8_SRGB:
        case Format::ASTC_8x8_UNORM:
            return { 8, 8, 16 };
        case Format::ASTC_10x10_SRGB:
        case Format::ASTC_10x10_UNORM:
            return { 10, 10, 16 };
        case Format::ASTC_12x12_SRGB:
        case Format::ASTC_12x12_UNORM:
            return { 12, 12, 16 };

        // case Format::BC1_RGBA_UNORM: return { 4, 4, 8 };  // BC1/BC4
        // case Format::BC3_RGBA_UNORM: return { 4, 4, 16 }; // BC2/3/5/6H/7

        default:
            return { 1, 1, 4 };
        }
    }

    static inline uint32_t DivRoundUp(uint32_t value, uint32_t divisor)
    {
        return (value + divisor - 1) / divisor;
    }

    static inline size_t RowPitch(uint32_t width, const BlockFormatInfo& info)
    {
        return (size_t)DivRoundUp(width, info.blockWidth) * info.bytesPerBlock;
    }

    static inline size_t ImageSize(uint32_t width, uint32_t height, const BlockFormatInfo& info)
    {
        return RowPitch(width, info) * DivRoundUp(height, info.blockHeight);
    }

    MetalTexture::MetalTexture(const TextureDescriptor&& desc)
    {
        MetalDevice* device = (MetalDevice*)Device::Instance;
        MetalRenderer* renderer = (MetalRenderer*)Renderer::Instance;
 
        DebugName = desc.debugName;
        ImageType = desc.type;
        LayerCount = desc.layerCount;
        Extent = MTL::Size((NS::UInteger)desc.dimensions.x, (NS::UInteger)desc.dimensions.y, (NS::UInteger)desc.dimensions.z);
        BindSampler = desc.bindSampler;
        
        m_BlockInfo = GetBlockFormatInfo(desc.format);
 
        const uint32_t faceCount = (desc.type == TextureType::CUBE ? 6 : LayerCount);
    
        MTL::TextureUsage usage = MtlUtils::TextureUsageFlagsToMTLTextureUsage(desc.usage);

        // Required for ChangeTextureView to work at all: MTLTextureUsagePixelFormatView must
        // be set at creation time for a texture to later support a format- or type-
        // reinterpreted view (e.g. the D2_ARRAY -> CUBE view the skybox pass creates after
        // its compute dispatch).
        if (desc.dynamicTextureView)
        {
            usage |= MTL::TextureUsagePixelFormatView;
        }
 
        // Allocate texture (GPU-private, CPU never touches this again after the initial upload below).
        MTL::TextureDescriptor* texDesc = MTL::TextureDescriptor::alloc()->init();
        texDesc->setTextureType(MtlUtils::TextureTypeToMTLTextureType(desc.type));
        texDesc->setPixelFormat(MtlUtils::FormatToMTLPixelFormat(desc.format));
        texDesc->setWidth(Extent.width);
        texDesc->setHeight(Extent.height);
        texDesc->setArrayLength(desc.type == TextureType::D2_ARRAY ? LayerCount : 1);
        texDesc->setUsage(usage);
        texDesc->setStorageMode(MTL::StorageModePrivate);
 
        m_StorageTexture = device->Get()->newTexture(texDesc);
        texDesc->release();

        if (m_StorageTexture == nullptr)
        {
            HBL2_CORE_ERROR("MetalTexture: newTexture failed for '{}'", DebugName);
            return;
        }

        m_StorageTexture->setLabel(NS::String::string(DebugName, NS::UTF8StringEncoding));

        Texture = m_StorageTexture;

        // Texture stays resident for its whole lifetime once created.
        renderer->MakeResident({ m_StorageTexture });
 
        if (desc.initialData == nullptr && Extent.width == 1 && Extent.height == 1)
        {
            const size_t imageSize = ImageSize((uint32_t)Extent.width, (uint32_t)Extent.height, m_BlockInfo);
 
            MTL::Buffer* stagingBuffer = nullptr;
            CreateStagingBuffer(renderer, imageSize, &stagingBuffer);
 
            uint32_t whitePixel = 0xffffffff;
            std::memcpy(stagingBuffer->contents(), &whitePixel, imageSize);
 
            CopyBufferToTexture(renderer, stagingBuffer);
 
            renderer->RemoveResident(stagingBuffer);
            stagingBuffer->release();
        }
        else if (desc.initialData != nullptr)
        {
            const size_t faceSize = ImageSize((uint32_t)Extent.width, (uint32_t)Extent.height, m_BlockInfo);
            const size_t imageSize = faceSize * faceCount;
 
            MTL::Buffer* stagingBuffer = nullptr;
            CreateStagingBuffer(renderer, imageSize, &stagingBuffer);
 
            std::memcpy(stagingBuffer->contents(), desc.initialData, imageSize);
            std::free(desc.initialData);
 
            CopyBufferToTexture(renderer, stagingBuffer);
 
            renderer->RemoveResident(stagingBuffer);
            stagingBuffer->release();
        }
 
        if (desc.createSampler)
        {
            MTL::SamplerDescriptor* samplerDesc = MTL::SamplerDescriptor::alloc()->init();
            samplerDesc->setMinFilter(MtlUtils::FilterToMTLSamplerMinMagFilter(desc.sampler.filter));
            samplerDesc->setMagFilter(MtlUtils::FilterToMTLSamplerMinMagFilter(desc.sampler.filter));
            samplerDesc->setMipFilter(MTL::SamplerMipFilterNotMipmapped);
            samplerDesc->setSAddressMode(MtlUtils::WrapToMTLSamplerAddressMode(desc.sampler.wrap));
            samplerDesc->setTAddressMode(MtlUtils::WrapToMTLSamplerAddressMode(desc.sampler.wrap));
            samplerDesc->setRAddressMode(MtlUtils::WrapToMTLSamplerAddressMode(desc.sampler.wrap));
 
            if (desc.sampler.compareEnable)
            {
                samplerDesc->setCompareFunction(MtlUtils::CompareToMTLCompareFunction(desc.sampler.compare));
            }
 
            if (desc.sampler.wrap == Wrap::CLAMP_TO_BORDER)
            {
                samplerDesc->setBorderColor(MTL::SamplerBorderColorOpaqueWhite);
            }
 
            Sampler = device->Get()->newSamplerState(samplerDesc);
            samplerDesc->release();
        }
    }

    void MetalTexture::Update(const Span<const std::byte>& bytes)
    {
        MetalRenderer* renderer = (MetalRenderer*)Renderer::Instance;
 
        const uint32_t faceCount = (ImageType == TextureType::CUBE ? 6 : LayerCount);
        const size_t faceSize = ImageSize((uint32_t)Extent.width, (uint32_t)Extent.height, m_BlockInfo);
        const size_t imageSize = faceSize * faceCount;
 
        MTL::Buffer* stagingBuffer = nullptr;
        CreateStagingBuffer(renderer, imageSize, &stagingBuffer);
 
        std::memcpy(stagingBuffer->contents(), bytes.Data(), imageSize);
 
        CopyBufferToTexture(renderer, stagingBuffer);
 
        renderer->RemoveResident(stagingBuffer);
        stagingBuffer->release();
    }
 
    void MetalTexture::ChangeTextureView(const TextureViewDescriptor&& desc)
    {
        MetalRenderer* renderer = (MetalRenderer*)Renderer::Instance;

        MTL::Texture* previouslyBound = Texture;

        MTL::Texture* newView = m_StorageTexture->newTextureView(MtlUtils::FormatToMTLPixelFormat(desc.format), MtlUtils::TextureTypeToMTLTextureType(desc.type), NS::Range::Make(0, 1), NS::Range::Make(0, desc.layerCount));

        if (newView == nullptr)
        {
            HBL2_CORE_ERROR("MetalTexture: newTextureView failed for '{}' - check MTLTextureUsagePixelFormatView and type/format compatibility", DebugName);
            
            return;
        }
        
        Texture = newView;
        BindSampler = desc.bindSampler;

        if (previouslyBound != m_StorageTexture)
        {
            auto& deletionQueue = ResourceManager::Instance->GetDeletionQueue();
            deletionQueue.Push(Renderer::Instance->GetFrameNumber(), [previouslyBound, renderer]()
            {
                previouslyBound->release();
            });
        }
    }

    void MetalTexture::SynchronizeUsage(MetalCommandBuffer* commandBuffer, ResourceState currentState, ResourceState newState)
    {
        commandBuffer->AddPendingBarrier(currentState, newState);
    }
 
    void MetalTexture::Destroy()
    {
        MetalRenderer* renderer = (MetalRenderer*)Renderer::Instance;
 
        if (Sampler != nullptr)
        {
            Sampler->release();
        }
 
        if (Texture != nullptr && Texture != m_StorageTexture)
        {
            Texture->release(); // release the currently-bound view, if any
        }

        if (m_StorageTexture != nullptr)
        {
            renderer->RemoveResident(m_StorageTexture);
            m_StorageTexture->release(); // release root storage last, views may reference it until this point
        }
    }
 
    void MetalTexture::CreateStagingBuffer(MetalRenderer* renderer, size_t imageSize, MTL::Buffer** outStagingBuffer)
    {
        MetalDevice* device = (MetalDevice*)Device::Instance;

        *outStagingBuffer = device->Get()->newBuffer(imageSize, MTL::ResourceStorageModeShared);

        if (*outStagingBuffer == nullptr)
        {
            HBL2_CORE_ERROR("MetalTexture: failed to allocate {} byte staging buffer for '{}'", imageSize, DebugName);
        }
    }

    void MetalTexture::CopyBufferToTexture(MetalRenderer* renderer, MTL::Buffer* stagingBuffer)
    {
        const uint32_t faceCount = (ImageType == TextureType::CUBE ? 6 : LayerCount);
        const size_t faceSize = ImageSize((uint32_t)Extent.width, (uint32_t)Extent.height, m_BlockInfo);
        const size_t rowPitch = RowPitch((uint32_t)Extent.width, m_BlockInfo);

        renderer->MakeResident({ stagingBuffer });

        MTL::Size faceExtent = Extent;
        faceExtent.depth = 1;

        // Always write through m_StorageTexture, never through `Texture`, `Texture` may
        // currently be a reinterpreted view (different type/format), and not every
        // reinterpretation is guaranteed to be a safe write target.
        MTL::Texture* writeTarget = m_StorageTexture;

        // One-off buffer -> texture copy. Safe to call from any worker thread: renderer
        // hands each calling thread its own MTL4::CommandAllocator/CommandBuffer pair
        // internally and blocks only the calling thread until the GPU signals completion,
        // same synchronous semantics as the Vulkan ImmediateSubmit path.
        renderer->ImmediateSubmit([=, this](MTL4::ComputeCommandEncoder* encoder)
        {
            for (uint32_t face = 0; face < faceCount; ++face)
            {
                encoder->copyFromBuffer(
                    stagingBuffer,
                    faceSize * face,
                    rowPitch,
                    faceSize,
                    faceExtent,
                    writeTarget,
                    face,
                    0,
                    MTL::Origin(0, 0, 0));
            }
        });
    }
}
