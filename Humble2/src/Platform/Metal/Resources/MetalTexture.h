#pragma once

#include "Base.h"
#include "Resources/TypeDescriptors.h"

#include "Platform/Metal/MetalCommon.h"
#include "Platform/Metal/MetalCommandBuffer.h"

namespace HBL2
{
    class MetalRenderer;

    struct MetalTexture
    {
        MetalTexture() = default;
        MetalTexture(const TextureDescriptor&& desc);
        
        void Update(const Span<const std::byte>& bytes);
        void ChangeTextureView(const TextureViewDescriptor&& desc);
        void Destroy();
        
        const char* DebugName = "";
        MTL::Texture* Texture = nullptr;
        MTL::SamplerState* Sampler = nullptr;
        MTL::Size Extent;
        TextureType ImageType = TextureType::D2;
        uint32_t LayerCount = 0;
        
    private:
        void CreateStagingBuffer(MetalRenderer* renderer, size_t imageSize, MTL::Buffer** outStagingBuffer);
        void CopyBufferToTexture(MetalRenderer* renderer, MTL::Buffer* stagingBuffer);
        
    private:
        MTL::Texture* m_StorageTexture = nullptr;
        uint32_t m_PixelByteSize = 0;
    };
}

