#include "RuntimeAssetManager.h"

#include "Serialization/AssetPackSerializer.h"
#include "Project/Project.h"

namespace HBL2
{
    void RuntimeAssetManager::Initialize(const AssetManagerSpecification& spec)
    {
        m_Spec = spec;

        m_AssetPool.Initialize(m_Spec.Assets);

        size_t registeredAssetMapBytes = FixedHashMap<UUID, Handle<Asset>>::RequiredBytes(m_Spec.Assets);
        size_t assetPackAssetsMapBytes = FixedHashMap<UUID, AssetPack::AssetInfo>::RequiredBytes(m_Spec.Assets);

        uint64_t bytes = ArenaLayout::Create()
            .AddRaw(registeredAssetMapBytes, alignof(std::pair<UUID, Handle<Asset>>))
            .Add<Handle<Asset>>(m_Spec.Assets)
            .AddRaw(assetPackAssetsMapBytes, alignof(std::pair<UUID, AssetPack::AssetInfo>))
            .Total();

        m_Reservation = Allocator::Arena.Reserve("AssetManagerPool", bytes);
        m_PoolArena.Initialize(&Allocator::Arena, bytes, m_Reservation);

        m_RegisteredAssetMap = FixedHashMap<UUID, Handle<Asset>>(&m_PoolArena, m_Spec.Assets);
        m_RegisteredAssets = FixedArray<Handle<Asset>>(&m_PoolArena, m_Spec.Assets);

        m_AssetPack.Assets = FixedHashMap<UUID, AssetPack::AssetInfo>(&m_PoolArena, m_Spec.Assets);

        m_AssetPackPath = Project::GetAssetDirectory() / "AssetPack.hblap";
    }

    bool RuntimeAssetManager::IsAssetValid(Handle<Asset> handle)
    {
        return handle.IsValid() && GetAssetMetadata(handle) != nullptr;
    }

    bool RuntimeAssetManager::IsAssetLoaded(Handle<Asset> handle)
    {
        if (!IsAssetValid(handle))
        {
            return false;
        }

        Asset* asset = GetAssetMetadata(handle);
        return asset->Loaded;
    }

    void RuntimeAssetManager::RegisterAssets()
    {
        // Load asset pack file.
        AssetPackSerializer serializer(&m_AssetPack);
        serializer.Deserialize(m_AssetPackPath);

        // Register all the assets from the asset pack.
        for (const auto& [assetUUID, assetInfo] : m_AssetPack.Assets)
        {
            AssetType assetType = (AssetType)assetInfo.Type;

            switch (assetType)
            {
            case AssetType::Texture:
                CreateAsset(assetUUID, { .debugName = "texture-asset", .type = AssetType::Texture, });
                break;
            case AssetType::Shader:
                CreateAsset(assetUUID, { .debugName = "shader-asset", .type = AssetType::Shader, });
                break;
            case AssetType::Material:
                CreateAsset(assetUUID, { .debugName = "material-asset", .type = AssetType::Material, });
                break;
            case AssetType::Scene:
                CreateAsset(assetUUID, { .debugName = "scene-asset", .type = AssetType::Scene, });
                break;
            case AssetType::Mesh:
                CreateAsset(assetUUID, { .debugName = "mesh-asset", .type = AssetType::Mesh, });
                break;
            case AssetType::Script:
                CreateAsset(assetUUID, { .debugName = "script-asset", .type = AssetType::Script, });
                break;
            case AssetType::Sound:
                CreateAsset(assetUUID, { .debugName = "sound-asset", .type = AssetType::Sound, });
                break;
            case AssetType::Prefab:
                CreateAsset(assetUUID, { .debugName = "prefab-asset", .type = AssetType::Prefab, });
                break;
            case AssetType::None:
                HBL2_CORE_ERROR("Aborting asset creation, asset type is None.");
                break;
            }                
        }
    }

    void RuntimeAssetManager::DeregisterAssets()
    {
        WaitForAsyncJobs();

        // Delete assets.
        for (const auto handle : m_RegisteredAssets)
        {
            DeleteAssetImmediate(handle);
        }

        // Clear asset handle caches.
        m_RegisteredAssets.clear();
        m_RegisteredAssetMap.clear();
    }

    Handle<Asset> RuntimeAssetManager::CreateAsset(UUID assetUUID, const AssetDescriptor&& desc)
    {
        auto handle = GetHandleFromUUID(assetUUID);

        if (IsAssetValid(handle))
        {
            return handle;
        }

        handle = m_AssetPool.Insert(Asset(std::forward<const AssetDescriptor>(desc)));

        Asset* asset = GetAssetMetadata(handle);
        asset->UUID = assetUUID;

        m_RegisteredAssets.push_back(handle);
        m_RegisteredAssetMap[asset->UUID] = handle;

        return handle;
    }

    uint32_t RuntimeAssetManager::LoadAsset(Handle<Asset> handle)
    {
        Asset* asset = GetAssetMetadata(handle);
        if (asset == nullptr)
        {
            return 0;
        }

        auto& assetInfo = m_AssetPack.Assets[asset->UUID];

        FileStreamReader stream(m_AssetPackPath);
        stream.SetStreamPosition(assetInfo.PackedOffset);

        switch (asset->Type)
        {
        case AssetType::Texture:
            asset->Indentifier = ImportTexture(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Shader:
            asset->Indentifier = ImportShader(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Material:
            asset->Indentifier = ImportMaterial(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Mesh:
            asset->Indentifier = ImportMesh(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Scene:
            asset->Indentifier = ImportScene(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Sound:
            asset->Indentifier = ImportSound(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Prefab:
            asset->Indentifier = ImportPrefab(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Script:
            asset->Indentifier = ImportScript(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::None:
            return 0;
        }

        return 0;
    }

    void RuntimeAssetManager::UnloadAsset(Handle<Asset> handle)
    {
        Asset* asset = GetAssetMetadata(handle);

        if (asset == nullptr)
        {
            return;
        }

        switch (asset->Type)
        {
        case AssetType::Texture:
            UnloadTexture(asset);
            break;
        case AssetType::Shader:
            UnloadShader(asset);
            break;
        case AssetType::Mesh:
            UnloadMesh(asset);
            break;
        case AssetType::Material:
            UnloadMaterial(asset);
            break;
        case AssetType::Scene:
            UnloadScene(asset);
            break;
        case AssetType::Sound:
            UnloadSound(asset);
            break;
        case AssetType::Prefab:
            UnloadPrefab(asset);
            break;
        case AssetType::Script:
            UnloadScript(asset);
            break;
        case AssetType::None:
            break;
        }
    }

    /// Import methods

    Handle<Texture> RuntimeAssetManager::ImportTexture(Asset* asset, FileStreamReader& stream)
    {
        int width, height;
        uint64_t format;

        stream.ReadRaw(width);
        stream.ReadRaw(height);
        stream.ReadRaw(format);

        ByteSpan textureData;
        stream.ReadBytes(textureData);

        // Create the texture.
        auto texture = ResourceManager::Instance->CreateTexture({
            .dimensions = { width, height, 1 },
            .format = (Format)format,
            .internalFormat = (Format)format,
            .usage = { TextureUsage::SAMPLED, TextureUsage::COPY_DST },
            .aspect = TextureAspect::COLOR,
            .sampler = { .filter = TextureFilter::LINEAR },
            .initialData = textureData.data(),
        });

        return texture;
    }

    Handle<Shader> RuntimeAssetManager::ImportShader(Asset* asset, FileStreamReader& stream)
    {
        AssetPackFormat::Shader shaderAssetPackFormat;

        stream.ReadRaw<AssetPackFormat::Shader::FileHeader>(shaderAssetPackFormat.Header);
        bool validHeader = memcmp(shaderAssetPackFormat.Header.HEADER, "HBLSD", 5) == 0;
        HBL2_CORE_ASSERT(validHeader, "Invalid asset pack shader header, aborting shader asset load!");
        if (!validHeader)
        {
            return Handle<Shader>();
        }

        if (shaderAssetPackFormat.Header.Version != AssetPack::CurrentVersion)
        {
            HBL2_CORE_ERROR("AssetPack shader file format version {} is not compatible with current version {}", shaderAssetPackFormat.Header.Version, AssetPackFormat::Shader::CurrentVersion);
            return Handle<Shader>();
        }

        // Retrieve shader variants.
        {
            uint32_t shaderVariantCount = 0;
            stream.ReadRaw<uint32_t>(shaderVariantCount);
            shaderAssetPackFormat.Data.ShaderVariants.resize(shaderVariantCount);
            stream.ReadSpan<ShaderDescriptor::RenderPipeline::PackedVariant>(shaderAssetPackFormat.Data.ShaderVariants, shaderVariantCount);
        }
        
        // Retrieve shader vertex buffer bindings.
        {
            uint32_t vertexBufferBindingSize = 0;
            stream.ReadRaw<uint32_t>(vertexBufferBindingSize);
            for (uint32_t i = 0; i < vertexBufferBindingSize; i++)
            {
                ShaderDescriptor::RenderPipeline::VertexBufferBinding vertexBufferBinding = {};

                stream.ReadRaw<uint32_t>(vertexBufferBinding.bufferBinding);
                stream.ReadRaw<uint32_t>(vertexBufferBinding.byteStride);
                uint32_t attributeSize = 0;
                stream.ReadRaw<uint32_t>(attributeSize);
                vertexBufferBinding.attributes.resize(attributeSize);
                stream.ReadSpan<ShaderDescriptor::RenderPipeline::VertexBufferBinding::Attribute>(vertexBufferBinding.attributes, attributeSize);

                shaderAssetPackFormat.Data.VertexBufferBindings.push_back(vertexBufferBinding);
            }
        }

        // Retrieve shader bind group layouts.
        StaticDArray<Handle<BindGroupLayout>, 3> bindGroupLayouts;
        {
            uint32_t bindGroupLayoutDescriptorSize = 0;
            stream.ReadRaw<uint32_t>(bindGroupLayoutDescriptorSize);
            shaderAssetPackFormat.Data.BindGroupLayoutDescriptors.resize(bindGroupLayoutDescriptorSize);
            for (uint32_t i = 0; i < bindGroupLayoutDescriptorSize; i++)
            {
                auto& textureBindings = shaderAssetPackFormat.Data.BindGroupLayoutDescriptors[i].textureBindings;

                uint32_t textureBindingCount = 0;
                stream.ReadRaw<uint32_t>(textureBindingCount);
                textureBindings.resize(textureBindingCount);
                stream.ReadSpan<AssetPackFormat::BindGroupLayoutDescriptor::TextureBinding>(textureBindings, textureBindingCount);

                auto& bufferBindings = shaderAssetPackFormat.Data.BindGroupLayoutDescriptors[i].bufferBindings;

                uint32_t bufferBindingCount = 0;
                stream.ReadRaw<uint32_t>(bufferBindingCount);
                bufferBindings.resize(bufferBindingCount);
                stream.ReadSpan<AssetPackFormat::BindGroupLayoutDescriptor::BufferBinding>(bufferBindings, bufferBindingCount);
            }

            for (auto& bindGroupLayoutDescriptor : shaderAssetPackFormat.Data.BindGroupLayoutDescriptors)
            {
                auto& textureBindings = bindGroupLayoutDescriptor.textureBindings;
                auto& bufferBindings = bindGroupLayoutDescriptor.bufferBindings;

                if (!textureBindings.empty() || !bufferBindings.empty())
                {
                    // Create the shader bind group layout resource.
                    Handle<BindGroupLayout> bindGroupLayout = ResourceManager::Instance->CreateBindGroupLayout({
                        .textureBindings = { (const BindGroupLayoutDescriptor::TextureBinding*)textureBindings.data(), textureBindings.size() },
                        .bufferBindings = { (const BindGroupLayoutDescriptor::BufferBinding*)bufferBindings.data(), bufferBindings.size() },
                    });

                    bindGroupLayouts.push_back(bindGroupLayout);
                }
                else
                {
                    bindGroupLayouts.push_back(Renderer::Instance->GetEmptyBindingsLayout());
                }
            }
        }

        // Retrieve shader bind group.
        Handle<BindGroup> shaderBindGroup;
        {
            // Deserialize shader bind group texture entries.
            auto& textureEntries = shaderAssetPackFormat.Data.ShaderBindGroupDescriptor.textures;

            uint32_t textureEntryCount = 0;
            stream.ReadRaw<uint32_t>(textureEntryCount);
            textureEntries.resize(textureEntryCount);
            stream.ReadSpan<AssetPackFormat::Shader::Metadata::BindGroupDescriptor::TextureEntry>(textureEntries, textureEntryCount);

            // Deserialize shader bind group buffer entries.
            auto& bufferEntries = shaderAssetPackFormat.Data.ShaderBindGroupDescriptor.buffers;

            uint32_t bufferEntryCount = 0;
            stream.ReadRaw<uint32_t>(bufferEntryCount);
            bufferEntries.resize(bufferEntryCount);
            for (uint32_t i = 0; i < bufferEntryCount; i++)
            {
                stream.ReadRaw<uint32_t>(bufferEntries[i].byteOffset);
                stream.ReadRaw<uint32_t>(bufferEntries[i].range);

                uint32_t bufferUsageFlags = 0;
                stream.ReadRaw<uint32_t>(bufferUsageFlags);
                bufferEntries[i].buffer.usage.SetRaw(bufferUsageFlags);

                uint32_t memoryUsage = 0;
                stream.ReadRaw<uint32_t>(memoryUsage);
                bufferEntries[i].buffer.memoryUsage = (MemoryUsage)memoryUsage;

                stream.ReadRaw<uint32_t>(bufferEntries[i].buffer.byteSize);

                ByteSpan bufferBytes(bufferEntries[i].buffer.initialData, bufferEntries[i].buffer.byteSize);
                stream.ReadBytes(bufferBytes, bufferEntries[i].buffer.byteSize);
            }

            // Populate actual texture entries that will be used to create the bind group.
            StaticDArray<BindGroupDescriptor::TextureEntry, 8> finalTextureEntries;
            for (auto& textureEntry : textureEntries)
            {
                finalTextureEntries.push_back({ AssetManager::Instance->GetAsset<Texture>(textureEntry.texture) });
            }

            // Populate actual buffer entries that will be used to create the bind group.
            StaticDArray<BindGroupDescriptor::BufferEntry, 8> finalBufferEntries;
            for (auto& bufferEntry : bufferEntries)
            {
                auto userBuffer = ResourceManager::Instance->CreateBuffer({
                    .debugName = "shader-uniform-buffer",
                    .usage = bufferEntry.buffer.usage,
                    .memoryUsage = bufferEntry.buffer.memoryUsage,
                    .byteSize = bufferEntry.buffer.byteSize,
                    .initialData = bufferEntry.buffer.initialData,
                });

                // TODO: Delete bufferEntry.buffer.initialData since it was allocated with malloc?

                finalBufferEntries.push_back({ .buffer = userBuffer });
            }

            // Create the shader bind group resource.
            shaderBindGroup = ResourceManager::Instance->CreateBindGroup({
                .debugName = "shader-bind-group",
                .layout = bindGroupLayouts[1],
                .textures = finalTextureEntries,
                .buffers = finalBufferEntries,
            });
        }        

        // Retrieve shader specialization constants per variant.
        StaticDArray<Span<const ShaderConstant>, 16> specializationConstantsPerVariant;
        {
            uint32_t specializationConstantsPerVariantCount = 0;
            stream.ReadRaw<uint32_t>(specializationConstantsPerVariantCount);
            shaderAssetPackFormat.Data.SpecializationConstantsPerVariant.resize(specializationConstantsPerVariantCount);
            for (uint32_t i = 0; i < specializationConstantsPerVariantCount; i++)
            {
                auto& variantSpecializationConstants = shaderAssetPackFormat.Data.SpecializationConstantsPerVariant[i];

                uint32_t variantSpecializationConstantCount = 0;
                stream.ReadRaw<uint32_t>(variantSpecializationConstantCount);
                variantSpecializationConstants.resize(variantSpecializationConstantCount);
                stream.ReadSpan<ShaderConstant>(variantSpecializationConstants, variantSpecializationConstantCount);
            }

            for (const auto& constants : shaderAssetPackFormat.Data.SpecializationConstantsPerVariant)
            {
                specializationConstantsPerVariant.push_back(Span<const ShaderConstant>(constants.data(), constants.size()));
            }
        }

        // Retrieve shader code.
        {
            stream.ReadBytes(shaderAssetPackFormat.Data.VertexShaderCode);
            stream.ReadBytes(shaderAssetPackFormat.Data.FragmentShaderCode);
        }

        // Create shader resource.
        auto shader = ResourceManager::Instance->CreateShader({
            .VS { .code = { (uint8_t*)shaderAssetPackFormat.Data.VertexShaderCode.data(), shaderAssetPackFormat.Data.VertexShaderCode.size() } },
            .FS { .code = { (uint8_t*)shaderAssetPackFormat.Data.FragmentShaderCode.data(), shaderAssetPackFormat.Data.FragmentShaderCode.size() } },
            .bindGroups {
                bindGroupLayouts[0],		                    // Global bind group			(0)
                bindGroupLayouts[1],		                    // Global user bind group		(1)
                bindGroupLayouts[2],		                    // Material / user bind group	(2)
                Renderer::Instance->GetDynamicBindingsLayout(),	// Draw bind group				(3)
            },
            .renderPipeline {
                .vertexBufferBindings = shaderAssetPackFormat.Data.VertexBufferBindings,
                .variants = shaderAssetPackFormat.Data.ShaderVariants,
                .specializationConstantsPerVariant = { specializationConstantsPerVariant.data(), specializationConstantsPerVariant.size() },
            },
            .renderPass = Renderer::Instance->GetRenderingRenderPass(),
            .shaderBindGroup = shaderBindGroup,
        });

        return shader;
    }

    Handle<Material> RuntimeAssetManager::ImportMaterial(Asset* asset, FileStreamReader& stream)
    {
        AssetPackFormat::Material materialAssetPackFormat;

        stream.ReadRaw<AssetPackFormat::Material::FileHeader>(materialAssetPackFormat.Header);
        bool validHeader = memcmp(materialAssetPackFormat.Header.HEADER, "HBLMT", 5) == 0;
        HBL2_CORE_ASSERT(validHeader, "Invalid asset pack material header, aborting material asset load!");
        if (!validHeader)
        {
            return Handle<Material>();
        }

        if (materialAssetPackFormat.Header.Version != AssetPack::CurrentVersion)
        {
            HBL2_CORE_ERROR("AssetPack material file format version {} is not compatible with current version {}", materialAssetPackFormat.Header.Version, AssetPackFormat::Material::CurrentVersion);
            return Handle<Material>();
        }

        // Retrieve material type.
        uint32_t materialType = 0;
        {
            stream.ReadRaw<uint32_t>(materialType);
        }

        // Retrieve shader handle.
        Handle<Shader> shaderHandle;
        {
            stream.ReadRaw<UUID>(materialAssetPackFormat.Data.ShaderUUID);
            shaderHandle = AssetManager::Instance->GetAsset<Shader>(materialAssetPackFormat.Data.ShaderUUID);
        }

        // Retrieve material bind group.
        Handle<BindGroup> materialBindGroup;
        {
            // Deserialize material bind group layout.
            Handle<BindGroupLayout> bindGroupLayout;
            {
                auto& textureBindings = materialAssetPackFormat.Data.MaterialBindGroupDescriptor.layout.textureBindings;

                uint32_t textureBindingCount = 0;
                stream.ReadRaw<uint32_t>(textureBindingCount);
                textureBindings.resize(textureBindingCount);
                stream.ReadSpan<AssetPackFormat::BindGroupLayoutDescriptor::TextureBinding>(textureBindings, textureBindingCount);

                auto& bufferBindings = materialAssetPackFormat.Data.MaterialBindGroupDescriptor.layout.bufferBindings;

                uint32_t bufferBindingCount = 0;
                stream.ReadRaw<uint32_t>(bufferBindingCount);
                bufferBindings.resize(bufferBindingCount);
                stream.ReadSpan<AssetPackFormat::BindGroupLayoutDescriptor::BufferBinding>(bufferBindings, bufferBindingCount);

                // Create the material bind group layout resource.
                bindGroupLayout = ResourceManager::Instance->CreateBindGroupLayout({
                    .textureBindings = { (const BindGroupLayoutDescriptor::TextureBinding*)textureBindings.data(), textureBindings.size() },
                    .bufferBindings = { (const BindGroupLayoutDescriptor::BufferBinding*)bufferBindings.data(), bufferBindings.size() },
                });
            }

            // Deserialize material bind group texture entries.
            auto& textureEntries = materialAssetPackFormat.Data.MaterialBindGroupDescriptor.textures;

            uint32_t textureEntryCount = 0;
            stream.ReadRaw<uint32_t>(textureEntryCount);
            textureEntries.resize(textureEntryCount);
            stream.ReadSpan<AssetPackFormat::Material::Metadata::BindGroupDescriptor::TextureEntry>(textureEntries, textureEntryCount);

            // Deserialize material bind group buffer entries.
            auto& bufferEntries = materialAssetPackFormat.Data.MaterialBindGroupDescriptor.buffers;

            uint32_t bufferEntryCount = 0;
            stream.ReadRaw<uint32_t>(bufferEntryCount);
            bufferEntries.resize(bufferEntryCount);
            for (uint32_t i = 0; i < bufferEntryCount; i++)
            {
                stream.ReadRaw<uint32_t>(bufferEntries[i].byteOffset);
                stream.ReadRaw<uint32_t>(bufferEntries[i].range);

                uint32_t bufferUsageFlags = 0;
                stream.ReadRaw<uint32_t>(bufferUsageFlags);
                bufferEntries[i].buffer.usage.SetRaw(bufferUsageFlags);

                uint32_t memoryUsage = 0;
                stream.ReadRaw<uint32_t>(memoryUsage);
                bufferEntries[i].buffer.memoryUsage = (MemoryUsage)memoryUsage;

                stream.ReadRaw<uint32_t>(bufferEntries[i].buffer.byteSize);

                ByteSpan bufferBytes(bufferEntries[i].buffer.initialData, bufferEntries[i].buffer.byteSize);
                stream.ReadBytes(bufferBytes, bufferEntries[i].buffer.byteSize);
            }

            // Populate actual texture entries that will be used to create the bind group.
            StaticDArray<BindGroupDescriptor::TextureEntry, 8> finalTextureEntries;
            for (auto& textureEntry : textureEntries)
            {
                finalTextureEntries.push_back({ AssetManager::Instance->GetAsset<Texture>(textureEntry.texture) });
            }

            // Populate actual buffer entries that will be used to create the bind group.
            StaticDArray<BindGroupDescriptor::BufferEntry, 8> finalBufferEntries;
            for (auto& bufferEntry : bufferEntries)
            {
                auto userBuffer = ResourceManager::Instance->CreateBuffer({
                    .debugName = "shader-uniform-buffer",
                    .usage = bufferEntry.buffer.usage,
                    .memoryUsage = bufferEntry.buffer.memoryUsage,
                    .byteSize = bufferEntry.buffer.byteSize,
                    .initialData = bufferEntry.buffer.initialData,
                });
                
                // TODO: Delete bufferEntry.buffer.initialData since it was allocated with malloc?

                finalBufferEntries.push_back({ .buffer = userBuffer });
            }

            // Create the material bind group resource.
            materialBindGroup = ResourceManager::Instance->CreateBindGroup({
                .debugName = "shader-bind-group",
                .layout = bindGroupLayout,
                .textures = finalTextureEntries,
                .buffers = finalBufferEntries,
            });
        }

        // Retrieve material variant hash.
        ShaderDescriptor::RenderPipeline::PackedVariant variantDesc;
        {
            stream.ReadRaw<packed_size>(materialAssetPackFormat.Data.MaterialVariantHash);
            variantDesc = ShaderDescriptor::RenderPipeline::PackedVariant::FromKey(materialAssetPackFormat.Data.MaterialVariantHash);
        }

        uint32_t dynamicUniformBufferRange = (materialType == 0 ? sizeof(PerDrawDataSprite) : sizeof(PerDrawData));

        auto drawBindings = ResourceManager::Instance->CreateBindGroup({
            .layout = Renderer::Instance->GetDynamicBindingsLayout(),
            .buffers = { { .buffer = Renderer::Instance->TempUniformRingBuffer->GetBuffer(), .range = dynamicUniformBufferRange }, }
        });

        auto material = ResourceManager::Instance->CreateMaterial({
            .shader = shaderHandle,
            .drawBindGroup = drawBindings,
            .materialBindGroup = materialBindGroup,
        });

        Material* mat = ResourceManager::Instance->GetMaterial(material);
        mat->VariantHash = variantDesc;

        return material;
    }

    Handle<Mesh> RuntimeAssetManager::ImportMesh(Asset* asset, FileStreamReader& stream)
    {
        return Handle<Mesh>();
    }

    Handle<Scene> RuntimeAssetManager::ImportScene(Asset* asset, FileStreamReader& stream)
    {
        return Handle<Scene>();
    }

    Handle<Sound> RuntimeAssetManager::ImportSound(Asset* asset, FileStreamReader& stream)
    {
        return Handle<Sound>();
    }

    Handle<Prefab> RuntimeAssetManager::ImportPrefab(Asset* asset, FileStreamReader& stream)
    {
        return Handle<Prefab>();
    }

    Handle<Script> RuntimeAssetManager::ImportScript(Asset* asset, FileStreamReader& stream)
    {
        return Handle<Script>();
    }

    /// Unload methods

    void RuntimeAssetManager::UnloadTexture(Asset* asset)
    {
        Handle<Texture> textureAssetHandle = Handle<Texture>::UnPack(asset->Indentifier);

        if (!textureAssetHandle.IsValid())
        {
            HBL2_CORE_WARN("Asset \"{0}\" has an invalid handle, skipping unload operation.", asset->DebugName);
            return;
        }

        if (!asset->Loaded)
        {
            HBL2_CORE_WARN("Asset \"{0}\" is already unloaded, skipping unload operation.", asset->DebugName);
            return;
        }

        ResourceManager::Instance->DeleteTexture(textureAssetHandle);

        asset->Loaded = false;
        asset->Indentifier = 0;
    }

    void RuntimeAssetManager::UnloadShader(Asset * asset)
    {
        Handle<Shader> shaderAssetHandle = Handle<Shader>::UnPack(asset->Indentifier);

        if (!shaderAssetHandle.IsValid())
        {
            HBL2_CORE_WARN("Asset \"{0}\" has an invalid handle, skipping unload operation.", asset->DebugName);
            return;
        }

        if (!asset->Loaded)
        {
            HBL2_CORE_WARN("Asset \"{0}\" is already unloaded, skipping unload operation.", asset->DebugName);
            return;
        }

        ResourceManager::Instance->DeleteShader(shaderAssetHandle);

        asset->Loaded = false;
        asset->Indentifier = 0;
    }

    void RuntimeAssetManager::UnloadMesh(Asset * asset)
    {

    }

    void RuntimeAssetManager::UnloadMaterial(Asset * asset)
    {

    }

    void RuntimeAssetManager::UnloadScene(Asset * asset)
    {

    }

    void RuntimeAssetManager::UnloadSound(Asset * asset)
    {

    }

    void RuntimeAssetManager::UnloadPrefab(Asset * asset)
    {

    }

    void RuntimeAssetManager::UnloadScript(Asset* asset)
    {

    }
}
