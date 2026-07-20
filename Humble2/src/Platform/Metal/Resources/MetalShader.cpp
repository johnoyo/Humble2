#include "MetalShader.h"

#include "Platform/Metal/MetalResourceManager.h"

namespace HBL2
{
    void MetalShaderHot::Destroy()
    {
        // The Pso and DepthStencilState are released in the ShaderColdCold::Destroy with all the variants.
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

    uint64_t MetalShaderCold::GetOrCreatePipeline(const PipelineConfig& config, bool forceCreateNewAndRemoveOld)
    {
        void* p = nullptr;
        MTL::DepthStencilState* d = nullptr;
        uint32_t pipelineIndex = UINT32_MAX;

        // Fast path with a lock-free read.
        if (p = Find(config.variantDesc, &pipelineIndex); p != nullptr && !forceCreateNewAndRemoveOld)
        {
            if (config.shaderHotData != nullptr)
            {
                config.shaderHotData->Pso = p;
                config.shaderHotData->DepthStencilState = m_Entries[pipelineIndex].DepthStencilState;
            }
            return m_Entries[pipelineIndex].Key.Key();
        }

        // Slow path where only writers lock, if the pipeline is not in cache.
        std::lock_guard lock(m_WriteMutex);

        // Re-check after acquiring lock (another thread may have inserted).
        if (p == nullptr)
        {
            if (p = Find(config.variantDesc, &pipelineIndex); p != nullptr && !forceCreateNewAndRemoveOld)
            {
                if (config.shaderHotData != nullptr)
                {
                    config.shaderHotData->DepthStencilState = m_Entries[pipelineIndex].DepthStencilState;
                }
                return m_Entries[pipelineIndex].Key.Key();
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
            
            config.shaderHotData->DepthStencilState = d;
        }
        else
        {
            p = CreateComputePipeline(config);
        }

        // Check if we exceeded max variant count.
        uint32_t idx = m_Count.load(std::memory_order_relaxed);

        if (idx >= MaxVariants)
        {
            return 0;
        }

        // Store in array and update count.
        m_Entries[idx] = { config.variantDesc, p, d };
        m_Count.store(idx + 1, std::memory_order_release);

        return m_Entries[idx].Key.Key();
    }

    MTL::RenderPipelineState* MetalShaderCold::CreatePipeline(const PipelineConfig& config)
    {
        // Primitive topology state.
        MTL::PrimitiveTopologyClass topology = MtlUtils::TopologyToMTLPrimitiveTopologyClass((Topology)config.variantDesc.topology);
        
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
        pipelineDesc->setInputPrimitiveTopology(topology);
        pipelineDesc->setAlphaToOneState(alphaToOneState);
        pipelineDesc->setAlphaToCoverageState(alphaToCoverageState);
        pipelineDesc->setRasterSampleCount(1);
        pipelineDesc->setRasterizationEnabled(true);
        
        const auto& var = config.variantDesc;
        
        for (uint32_t i = 0; i < ColorAttachmentCount; i++)
        {
            MTL4::RenderPipelineColorAttachmentDescriptor* colorAttachment = pipelineDesc->colorAttachments()->object(i);

            colorAttachment->setBlendingState(config.variantDesc.blendEnabled ? MTL4::BlendStateEnabled : MTL4::BlendStateDisabled);
            
            colorAttachment->setSourceRGBBlendFactor(MtlUtils::BlendFactorToMTLBlendFactor((BlendFactor)var.srcColorFactor));
            colorAttachment->setDestinationRGBBlendFactor(MtlUtils::BlendFactorToMTLBlendFactor((BlendFactor)var.dstColorFactor));
            colorAttachment->setRgbBlendOperation(MtlUtils::BlendOperationToMTLBlendOperation((BlendOperation)var.colorOp));
            
            colorAttachment->setSourceAlphaBlendFactor(MtlUtils::BlendFactorToMTLBlendFactor((BlendFactor)var.srcAlphaFactor));
            colorAttachment->setDestinationAlphaBlendFactor(MtlUtils::BlendFactorToMTLBlendFactor((BlendFactor)var.dstAlphaFactor));
            colorAttachment->setAlphaBlendOperation(MtlUtils::BlendOperationToMTLBlendOperation((BlendOperation)var.alphaOp));
            
            colorAttachment->setWriteMask(config.variantDesc.colorOutput ? MTL::ColorWriteMaskAll : MTL::ColorWriteMaskNone);
            
            colorAttachment->setPixelFormat(ColorAttachmentFormats[i]);
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
            for (auto& format : rp->ColorAttachmentFormats)
            {
                Cold->ColorAttachmentFormats.push_back(format);
            }
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

        MTL::Function* vertexFn = nullptr;
        MTL::Function* fragmentFn = nullptr;
        MTL::Function* computeFn = nullptr;
        
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
            vertexFn = vertexLibrary->newFunction(NS::String::string(desc.VS.entryPoint, NS::UTF8StringEncoding));

            // Fragment shader module.
            MTL::Library* fragmentLibrary;
            {
                NS::Error* error = nullptr;
                auto& bytes = desc.FS.code;
                
                auto data = dispatch_data_create(bytes.Data(), bytes.Size(), nullptr, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
                fragmentLibrary = device->Get()->newLibrary(data, &error);
                dispatch_release(data);

                if (!fragmentLibrary)
                {
                    std::cerr << "Failed to load vertex metallib from memory: " << error->localizedDescription()->utf8String() << "\n";
                    return;
                }
            }
            Cold->FragmentShaderModule = MTL4::LibraryFunctionDescriptor::alloc()->init();
            Cold->FragmentShaderModule->setLibrary(fragmentLibrary);
            Cold->FragmentShaderModule->setName(NS::String::string(desc.FS.entryPoint, NS::UTF8StringEncoding));
            fragmentFn = fragmentLibrary->newFunction(NS::String::string(desc.FS.entryPoint, NS::UTF8StringEncoding));

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
            computeFn = computeLibrary->newFunction(NS::String::string(desc.CS.entryPoint, NS::UTF8StringEncoding));
            
            entryPoints[0] = desc.CS.entryPoint;
            shaderModules[0] = Cold->ComputeShaderModule;
            entryPoints[1] = "";
            shaderModules[1] = nullptr;
            shaderModuleCount = 1;
        }
        
        uint32_t bindGroupIndex = 0;
        std::fill(Hot->BuffersInBindGroups.begin(), Hot->BuffersInBindGroups.end(), 0);
        std::fill(Hot->TexturesInBindGroups.begin(), Hot->TexturesInBindGroups.end(), 0);
        
        for (auto bindGroupLayoutHandle : desc.bindGroups)
        {
            MetalBindGroupLayout* bindGroupLayout = rm->GetBindGroupLayout(bindGroupLayoutHandle);
            
            Hot->BuffersInBindGroups[bindGroupIndex] = bindGroupLayout->BufferBindings.size();
            Hot->TexturesInBindGroups[bindGroupIndex] = bindGroupLayout->TextureBindings.size();
            
            bindGroupIndex++;
        }

//        // Reflection
//        if (vertexFn != nullptr && fragmentFn != nullptr)
//        {
//            const auto& binding = Cold->VertexBufferBinding;
//
//            MTL::VertexDescriptor* vertexDesc = MTL::VertexDescriptor::alloc()->init();
//
//            for (uint32_t j = 0; j < binding.attributes.size(); j++)
//            {
//                auto& attribute = binding.attributes[j];
//
//                vertexDesc->attributes()->object(j)->setFormat(MtlUtils::VertexFormatToMTLVertexFormat(attribute.format));
//                vertexDesc->attributes()->object(j)->setOffset(attribute.byteOffset);
//                vertexDesc->attributes()->object(j)->setBufferIndex(VERTEX_BUFFER_BINDING_IDX);
//            }
//
//            vertexDesc->layouts()->object(VERTEX_BUFFER_BINDING_IDX)->setStride(binding.byteStride);
//
//            MTL::RenderPipelineDescriptor* reflectionDesc = MTL::RenderPipelineDescriptor::alloc()->init();
//            reflectionDesc->setVertexFunction(vertexFn);
//            reflectionDesc->setFragmentFunction(fragmentFn);
//            reflectionDesc->setVertexDescriptor(vertexDesc);
//
//            MTL::AutoreleasedRenderPipelineReflection reflection;
//
//            device->Get()->newRenderPipelineState(reflectionDesc, MTL::PipelineOptionArgumentInfo, &reflection, nullptr);
//
//            HBL2_CORE_INFO("Shader: {0}", Cold->DebugName);
//            HBL2_CORE_INFO("Vertex Stage");
//            for (int i = 0; i < (int)reflection->vertexArguments()->count(); i++)
//            {
//                MTL::Argument* arg = (MTL::Argument*)reflection->vertexArguments()->object(i);
//                HBL2_CORE_INFO("name: {0}, index: {1}", std::string(arg->name()->cString(NS::UTF8StringEncoding)), arg->index());
//
//                if (arg->type() == MTL::ArgumentTypeBuffer)
//                {
//                    if (arg->index() != VERTEX_BUFFER_BINDING_IDX)
//                    {
//                        Hot->BufferIndexes.push_back(arg->index());
//                    }
//                }
//                else if (arg->type() == MTL::ArgumentTypeTexture)
//                {
//                    Hot->TextureIndexes.push_back(arg->index());
//                }
//            }
//            HBL2_CORE_INFO("Fragment Stage");
//            for (int i = 0; i < (int)reflection->fragmentArguments()->count(); i++)
//            {
//                MTL::Argument* arg = (MTL::Argument*)reflection->fragmentArguments()->object(i);
//                HBL2_CORE_INFO("name: {0}, index: {1}", std::string(arg->name()->cString(NS::UTF8StringEncoding)), arg->index());
//
//                if (arg->type() == MTL::ArgumentTypeBuffer)
//                {
//                    Hot->BufferIndexes.push_back(arg->index());
//                }
//                else if (arg->type() == MTL::ArgumentTypeTexture)
//                {
//                    Hot->TextureIndexes.push_back(arg->index());
//                }
//            }
//
//            std::sort(Hot->BufferIndexes.begin(), Hot->BufferIndexes.end(), [](const uint16_t& a, const uint16_t& b)
//            {
//                return a < b;
//            });
//
//            std::sort(Hot->TextureIndexes.begin(), Hot->TextureIndexes.end(), [](const uint16_t& a, const uint16_t& b)
//            {
//                return a < b;
//            });
//
//            HBL2_CORE_INFO("sorting done");
//      }

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
                .shaderHotData = Hot,
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

    uint64_t MetalShader::GetOrCreateVariant(ShaderDescriptor::RenderPipeline::PackedVariant key)
    {
        if (!IsValid())
        {
            return 0;
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
                .shaderHotData = Hot,
            });
        }

        return GetOrCreateComputeVariant(key);
    }

    uint64_t MetalShader::GetOrCreateComputeVariant(ShaderDescriptor::RenderPipeline::PackedVariant key)
    {
        if (!IsValid())
        {
            return 0;
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
