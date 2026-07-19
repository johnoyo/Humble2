#include "MetalShader.h"

#include "Platform/Metal/MetalResourceManager.h"

namespace HBL2
{
    void MetalShaderHot::Destroy()
    {
        if (Pso != nullptr)
        {
            ((MTL::Allocation*)Pso)->release();
        }
        
        if (DepthStencilState != nullptr)
        {
            DepthStencilState->release();
        }
        
        ResourceManager::Instance->DeleteBindGroup(ShaderBindGroup);
    }

    void* MetalShaderCold::Find(ShaderDescriptor::RenderPipeline::PackedVariant key, uint32_t* pipelineIndex)
    {
        uint32_t n = m_Count.load(std::memory_order_acquire);

        for (uint32_t i = 0; i < n; i++)
        {
            if (m_Entries[i].Key == key)
            {
                *pipelineIndex = i;
                return m_Entries[i].Pipeline;
            }
        }

        return nullptr;
    }

    void* MetalShaderCold::GetOrCreatePipeline(const PipelineConfig& config, bool forceCreateNewAndRemoveOld)
    {
        void* p = nullptr;
        MTL::DepthStencilState* d = nullptr;
        uint32_t pipelineIndex = UINT32_MAX;

        // Fast path with a lock-free read.
        if (p = Find(config.variantDesc, &pipelineIndex); p != nullptr && !forceCreateNewAndRemoveOld)
        {
            return p;
        }

        // Slow path where only writers lock, if the pipeline is not in cache.
        std::lock_guard lock(m_WriteMutex);

        // Re-check after acquiring lock (another thread may have inserted).
        if (p == nullptr)
        {
            if (p = Find(config.variantDesc, &pipelineIndex); p != nullptr && !forceCreateNewAndRemoveOld)
            {
                return p;
            }
        }

        // If we got a hit in the cache and we want to force create new and remove old, we schedule the pipeline for deletion.
        if (forceCreateNewAndRemoveOld && p != nullptr)
        {
            // Get current number of entries.
            uint32_t n = m_Count.load(std::memory_order_relaxed);

            // Swap entry to remove with last element.
            const uint32_t last = n - 1;

            if (pipelineIndex != last)
            {
                m_Entries[pipelineIndex] = m_Entries[last];
            }

            // Invalidate key and pipeline.
            m_Entries[last].Key = g_NullVariant;
            m_Entries[last].Pipeline = nullptr;
            m_Entries[last].DepthStencilState = nullptr;

            // Update number of entries.
            m_Count.store(last, std::memory_order_release);

            // Schedule retired pipeline for cleanup.
            ResourceManager::Instance->GetDeletionQueue().Push(Renderer::Instance->GetFrameNumber(), [=]()
            {
                ((MTL::RenderPipelineState*)p)->release();
                d->release();
            });
        }

        // Create pipeline
        if (ComputeShaderModule == nullptr)
        {
            p = CreatePipeline(config);
            
            // Depth state.
            MTL::DepthStencilDescriptor* depthStencilDesc = MTL::DepthStencilDescriptor::alloc()->init();
            depthStencilDesc->setDepthCompareFunction(config.variantDesc.depthEnabled ? MtlUtils::CompareToMTLCompareFunction((Compare)config.variantDesc.depthCompare) : MTL::CompareFunctionAlways);
            depthStencilDesc->setDepthWriteEnabled(config.variantDesc.depthWrite ? true : false);
            d = ((MetalDevice*)Device::Instance)->Get()->newDepthStencilState(depthStencilDesc);
            depthStencilDesc->release();
        }
        else
        {
            p = CreateComputePipeline(config);
        }

        // Check if we exceeded max variant count.
        uint32_t idx = m_Count.load(std::memory_order_relaxed);

        if (idx >= MaxVariants)
        {
            return p;
        }

        // Store in array and update count.
        m_Entries[idx] = { config.variantDesc, p, d };
        m_Count.store(idx + 1, std::memory_order_release);

        return p;
    }

    MTL::RenderPipelineState* MetalShaderCold::CreatePipeline(const PipelineConfig& config)
    {
        // Primitive topology state.
        MTL::PrimitiveTopologyClass inputPrimitiveTopology = MtlUtils::TopologyToMTLPrimitiveTopologyClass((Topology)config.variantDesc.topology);
        
        // Alpha state.
        MTL4::AlphaToOneState alphaToOneState = MTL4::AlphaToOneStateDisabled;
        MTL4::AlphaToCoverageState alphaToCoverageState = MTL4::AlphaToCoverageStateDisabled;
        
        // Vertex input state.
        MTL::VertexDescriptor* vertexDesc = nullptr;

        for (uint32_t i = 0; i < config.vertexBufferBindings.Size(); i++)
        {
            const auto& binding = config.vertexBufferBindings[i];
            
            vertexDesc = MTL::VertexDescriptor::alloc()->init();
            
            for (uint32_t j = 0; j < binding.attributes.size(); j++)
            {
                auto& attribute = binding.attributes[j];
                
                vertexDesc->attributes()->object(j)->setFormat(MtlUtils::VertexFormatToMTLVertexFormat(attribute.format));
                vertexDesc->attributes()->object(j)->setOffset(attribute.byteOffset);
                vertexDesc->attributes()->object(j)->setBufferIndex(VERTEX_BUFFER_BINDING_IDX);
            }
            
            vertexDesc->layouts()->object(VERTEX_BUFFER_BINDING_IDX)->setStride(binding.byteStride);
        }
        
        // Build pso.
        MTL4::RenderPipelineDescriptor* pipelineDesc = MTL4::RenderPipelineDescriptor::alloc()->init();
        pipelineDesc->setLabel(NS::String::string(DebugName, NS::UTF8StringEncoding));
        pipelineDesc->setVertexFunctionDescriptor(config.shaderModules[0]);
        pipelineDesc->setFragmentFunctionDescriptor(config.shaderModules[1]);
        pipelineDesc->setInputPrimitiveTopology(inputPrimitiveTopology);
        pipelineDesc->setAlphaToOneState(alphaToOneState);
        pipelineDesc->setAlphaToCoverageState(alphaToCoverageState);
        pipelineDesc->setRasterSampleCount(1);
        pipelineDesc->setRasterizationEnabled(true);
        for (uint32_t i = 0; i < ColorAttachmentCount; i++)
        {
            pipelineDesc->colorAttachments()->object(i)->setPixelFormat(ColorAttachmentFormats[i]);
        }
        pipelineDesc->setVertexDescriptor(vertexDesc);

        NS::Error* psoError = nullptr;
        MTL::RenderPipelineState* pipeline = Compiler->newRenderPipelineState(pipelineDesc, nullptr, &psoError);
        
        if (!pipeline)
        {
            std::cerr << "PSO compile error: " << psoError->localizedDescription()->utf8String() << "\n";
            return nullptr;
        }
        
        pipelineDesc->release();

        return pipeline;
    }

    MTL::ComputePipelineState* MetalShaderCold::CreateComputePipeline(const PipelineConfig& config)
    {
        // Build pso.
        MTL4::ComputePipelineDescriptor* pipelineDesc = MTL4::ComputePipelineDescriptor::alloc()->init();
        pipelineDesc->setLabel(NS::String::string(DebugName, NS::UTF8StringEncoding));
        pipelineDesc->setComputeFunctionDescriptor(config.shaderModules[0]);
        
        NS::Error* psoError = nullptr;
        MTL::ComputePipelineState* computePipeline = Compiler->newComputePipelineState(pipelineDesc, nullptr, &psoError);
        
        if (!computePipeline)
        {
            std::cerr << "PSO compile error: " << psoError->localizedDescription()->utf8String() << "\n";
            return nullptr;
        }
        
        pipelineDesc->release();

        return computePipeline;
    }

    void MetalShaderCold::Destroy()
    {
        if (Compiler != nullptr)
        {
            Compiler->release();
        }
        
        if (VertexShaderModule != nullptr)
        {
            VertexShaderModule->release();
        }
        if (FragmentShaderModule != nullptr)
        {
            FragmentShaderModule->release();
        }
        if (ComputeShaderModule != nullptr)
        {
            ComputeShaderModule->release();
        }
        
        for (const auto& variantEntry : m_Entries)
        {
            if (variantEntry.Pipeline != nullptr)
            {
                ((MTL::Allocation*)variantEntry.Pipeline)->release();
            }
            
            if (variantEntry.DepthStencilState != nullptr)
            {
                variantEntry.DepthStencilState->release();
            }
        }
    }

    void MetalShaderCold::DestroyOldShaderModules()
    {
        if (OldVertexShaderModule != nullptr)
        {
            OldVertexShaderModule->release();
        }
        if (OldFragmentShaderModule != nullptr)
        {
            OldFragmentShaderModule->release();
        }
        if (OldComputeShaderModule != nullptr)
        {
            OldComputeShaderModule->release();
        }
    }

    void MetalShader::Initialize(const ShaderDescriptor&& desc)
    {
        Recompile(std::forward<const ShaderDescriptor>(desc));
    }

    void MetalShader::Recompile(const ShaderDescriptor&& desc, bool removeVariants)
    {
        if (!IsValid())
        {
            return;
        }

        MetalDevice* device = (MetalDevice*)Device::Instance;
        MetalResourceManager* rm = (MetalResourceManager*)ResourceManager::Instance;
        
        Cold->OldVertexShaderModule = Cold->VertexShaderModule;
        Cold->OldFragmentShaderModule = Cold->FragmentShaderModule;
        Cold->OldComputeShaderModule = Cold->ComputeShaderModule;

        Cold->VertexShaderModule = nullptr;
        Cold->FragmentShaderModule = nullptr;
        Cold->ComputeShaderModule = nullptr;
        
        Cold->DebugName = desc.debugName;
        
        if (desc.renderPipeline.vertexBufferBindings.Size() > 0)
        {
            Cold->VertexBufferBinding = desc.renderPipeline.vertexBufferBindings[0];
        }
        
        MetalRenderPass* rp = rm->GetRenderPass(desc.renderPass);
        if (rp != nullptr)
        {
            Cold->ColorAttachmentFormats = rp->ColorAttachmentFormats;
            Cold->ColorAttachmentCount = rp->ColorAttachmentCount;
        }
        
        // BindGroups use a reference counting system, so if there are other objects
        // referencing the bindgroup, it will not be deleted, just the ref count will be decreased.
        if (Hot->ShaderBindGroup.IsValid())
        {
            ResourceManager::Instance->DeleteBindGroup(Hot->ShaderBindGroup);
        }
        Hot->ShaderBindGroup = desc.shaderBindGroup;
        
        // Create compiler.
        {
            auto* compilerDesc = MTL4::CompilerDescriptor::alloc()->init();
            Cold->Compiler = device->Get()->newCompiler(compilerDesc, nullptr);
            compilerDesc->release();
        }
        
        std::array<MTL4::LibraryFunctionDescriptor*, 2> shaderModules{};
        std::array<const char*, 2> entryPoints{};
        uint32_t shaderModuleCount = 0;

        if (desc.type == ShaderType::RASTERIZATION)
        {
            // Vertex shader module.
            MTL::Library* vertexLibrary;
            {
                NS::Error* error = nullptr;
                auto& bytes = desc.VS.code;
                
                auto data = dispatch_data_create(bytes.Data(), bytes.Size(), nullptr, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
                vertexLibrary = device->Get()->newLibrary(data, &error);
                dispatch_release(data);

                if (!vertexLibrary)
                {
                    std::cerr << "Failed to load vertex metallib from memory: " << error->localizedDescription()->utf8String() << "\n";
                    return;
                }
            }
            Cold->VertexShaderModule = MTL4::LibraryFunctionDescriptor::alloc()->init();
            Cold->VertexShaderModule->setLibrary(vertexLibrary);
            Cold->VertexShaderModule->setName(NS::String::string(desc.VS.entryPoint, NS::UTF8StringEncoding));

            // Fragment shader module.
            MTL::Library* fragmentLibrary;
            {
                NS::Error* error = nullptr;
                auto& bytes = desc.VS.code;
                
                auto data = dispatch_data_create(bytes.Data(), bytes.Size(), nullptr, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
                fragmentLibrary = device->Get()->newLibrary(data, &error);
                dispatch_release(data);

                if (!vertexLibrary)
                {
                    std::cerr << "Failed to load vertex metallib from memory: " << error->localizedDescription()->utf8String() << "\n";
                    return;
                }
            }
            Cold->FragmentShaderModule = MTL4::LibraryFunctionDescriptor::alloc()->init();
            Cold->FragmentShaderModule->setLibrary(fragmentLibrary);
            Cold->FragmentShaderModule->setName(NS::String::string(desc.FS.entryPoint, NS::UTF8StringEncoding));

            entryPoints[0] = desc.VS.entryPoint;
            shaderModules[0] = Cold->VertexShaderModule;
            entryPoints[1] = desc.FS.entryPoint;
            shaderModules[1] = Cold->FragmentShaderModule;
            shaderModuleCount = 2;
        }
        else
        {
            // Compute shader module.
            MTL::Library* computeLibrary;
            {
                NS::Error* error = nullptr;
                auto& bytes = desc.CS.code;
                
                auto data = dispatch_data_create(bytes.Data(), bytes.Size(), nullptr, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
                computeLibrary = device->Get()->newLibrary(data, &error);
                dispatch_release(data);

                if (!computeLibrary)
                {
                    std::cerr << "Failed to load vertex metallib from memory: " << error->localizedDescription()->utf8String() << "\n";
                    return;
                }
            }
            Cold->ComputeShaderModule = MTL4::LibraryFunctionDescriptor::alloc()->init();
            Cold->ComputeShaderModule->setLibrary(computeLibrary);
            Cold->ComputeShaderModule->setName(NS::String::string(desc.CS.entryPoint, NS::UTF8StringEncoding));

            entryPoints[0] = desc.CS.entryPoint;
            shaderModules[0] = Cold->ComputeShaderModule;
            entryPoints[1] = "";
            shaderModules[1] = nullptr;
            shaderModuleCount = 1;
        }

        // Create shader variants.
        for (int i = 0; i < desc.renderPipeline.variants.Size(); i++)
        {
            const auto& variant = desc.renderPipeline.variants[i];

            const MetalShaderCold::PipelineConfig pipelineConfig =
            {
                .shaderModules = shaderModules,
                .entryPoints = entryPoints,
                .shaderModuleCount = shaderModuleCount,
                .variantDesc = variant,
                .vertexBufferBindings = { &Cold->VertexBufferBinding, 1 },
                .specializationConstantStages = {},
            };

            Cold->GetOrCreatePipeline(pipelineConfig, removeVariants);
        }
    }

    void MetalShader::Destroy()
    {
        if (!IsValid())
        {
            return;
        }

        Hot->Destroy();
        Cold->Destroy();
    }

    void* MetalShader::GetOrCreateVariant(ShaderDescriptor::RenderPipeline::PackedVariant key)
    {
        if (!IsValid())
        {
            return nullptr;
        }
        
        if (Cold->ComputeShaderModule == nullptr)
        {
            return Cold->GetOrCreatePipeline({
                .shaderModules = { Cold->VertexShaderModule, Cold->FragmentShaderModule },
                .entryPoints = { "mainVS", "mainPS" }, // TODO: fix me!
                .shaderModuleCount = 2,
                .variantDesc = key,
                .vertexBufferBindings = { &Cold->VertexBufferBinding, 1 },
                .specializationConstantStages = {},
            });
        }

        return GetOrCreateComputeVariant(key);
    }

    void* MetalShader::GetOrCreateComputeVariant(ShaderDescriptor::RenderPipeline::PackedVariant key)
    {
        if (!IsValid())
        {
            return nullptr;
        }
        
        return Cold->GetOrCreatePipeline({
            .shaderModules = { Cold->ComputeShaderModule, nullptr },
            .entryPoints = { "mainCS", "" }, // TODO: fix me!
            .shaderModuleCount = 1,
            .variantDesc = key,
            .vertexBufferBindings = { &Cold->VertexBufferBinding, 1 },
            .specializationConstantStages = {},
        });
    }

    bool MetalShader::IsValid() const
    {
        return Cold != nullptr && Hot != nullptr;
    }
}
