#include "UFbxLoader.h"

#include "Project/Project.h"
#include "Asset/EditorAssetManager.h"
#include "Resources/ResourceManager.h"

#include "Utilities/MeshUtilities.h"
#include "Utilities/ShaderUtilities.h"
#include "Utilities/TextureUtilities.h"
#include "Utilities/FileDialogs.h"

namespace HBL2
{
    thread_local std::vector<Vertex> UFbxLoader::s_Vertices;
    thread_local std::vector<uint32_t> UFbxLoader::s_Indeces;
    thread_local std::vector<SkinVertex> UFbxLoader::s_SkinVertices;
    thread_local std::unordered_map<std::string, Handle<Asset>> UFbxLoader::s_MaterialNameToAssetHandle;

    static glm::mat4 ToGlm(const ufbx_matrix& m)
    {
        return glm::mat4(
            (float)m.cols[0].x, (float)m.cols[0].y, (float)m.cols[0].z, 0.0f,
            (float)m.cols[1].x, (float)m.cols[1].y, (float)m.cols[1].z, 0.0f,
            (float)m.cols[2].x, (float)m.cols[2].y, (float)m.cols[2].z, 0.0f,
            (float)m.cols[3].x, (float)m.cols[3].y, (float)m.cols[3].z, 1.0f);
    }

    static JointTransform ToJointTransform(const ufbx_transform& t)
    {
        JointTransform out;
        out.T = glm::vec3((float)t.translation.x, (float)t.translation.y, (float)t.translation.z);
        out.R = glm::normalize(glm::quat((float)t.rotation.w, (float)t.rotation.x, (float)t.rotation.y, (float)t.rotation.z));
        out.S = glm::vec3((float)t.scale.x, (float)t.scale.y, (float)t.scale.z);
        return out;
    }

    static FbxMeshSkin BuildMeshSkin(const ufbx_mesh& mesh, const SkeletonImport& skeleton)
    {
        FbxMeshSkin out;

        if (!skeleton.Valid || mesh.skin_deformers.count == 0)
        {
            return out;
        }

        if (mesh.skin_deformers.count > 1)
        {
            HBL2_CORE_WARN("Mesh has {} skin deformers, only the first one is imported.", mesh.skin_deformers.count);
        }

        const ufbx_skin_deformer* skin = mesh.skin_deformers[0];
        if (skin->clusters.count == 0)
        {
            return out;
        }

        if (skin->max_weights_per_vertex > 4)
        {
            HBL2_CORE_WARN("Skin has up to {} influences per vertex, only the strongest 4 are kept.", skin->max_weights_per_vertex);
        }

        bool anyJoint = false;
        out.ClusterToJoint.resize(skin->clusters.count);

        for (size_t i = 0; i < skin->clusters.count; ++i)
        {
            const ufbx_skin_cluster* cluster = skin->clusters[i];
            const int32_t joint = cluster->bone_node ? skeleton.NodeToJoint[cluster->bone_node->typed_id] : -1;

            out.ClusterToJoint[i] = joint >= 0 ? (uint16_t)joint : uint16_t(0xFFFF);
            anyJoint |= joint >= 0;
        }

        if (!anyJoint)
        {
            return out;
        }

        // All clusters of one mesh agree on this, take the first.
        const ufbx_skin_cluster* first = skin->clusters[0];
        out.GeometryToWorldBind = ufbx_matrix_mul(&first->bind_to_world, &first->geometry_to_bone);
        out.NormalMatrix = ufbx_matrix_for_normals(&out.GeometryToWorldBind);

        out.Deformer = skin;
        out.Valid = true;
        return out;
    }

    static SkinVertex BuildSkinVertex(const FbxMeshSkin& skin, uint32_t vertexIndex)
    {
        SkinVertex out{};   // default: joint 0, weight 1

        const ufbx_skin_vertex sv = skin.Deformer->vertices[vertexIndex];

        struct Influence { uint16_t Joint = 0; float Weight = 0.f; };
        Influence top[4];
        uint32_t count = 0;

        for (uint32_t k = 0; k < sv.num_weights; ++k)
        {
            const ufbx_skin_weight w = skin.Deformer->weights[sv.weight_begin + k];
            const uint16_t joint = skin.ClusterToJoint[w.cluster_index];

            if (joint == 0xFFFF || w.weight <= 0.0) continue;

            if (count < 4)
            {
                top[count++] = { joint, (float)w.weight };
            }
            else
            {
                uint32_t weakest = 0;
                for (uint32_t i = 1; i < 4; ++i) { if (top[i].Weight < top[weakest].Weight) weakest = i; }
                if ((float)w.weight > top[weakest].Weight) top[weakest] = { joint, (float)w.weight };
            }
        }

        float sum = 0.f;
        for (uint32_t i = 0; i < count; ++i) { sum += top[i].Weight; }

        if (count == 0 || sum <= 1e-6f)
        {
            return out;
        }

        out.Weights[0] = out.Weights[1] = out.Weights[2] = out.Weights[3] = 0.f;
        for (uint32_t i = 0; i < count; ++i)
        {
            out.Joints[i] = top[i].Joint;
            out.Weights[i] = top[i].Weight / sum;
        }

        return out;
    }

    Handle<Mesh> UFbxLoader::Load(const std::filesystem::path& path)
	{
        HBL2_FUNC_PROFILE();

        ufbx_load_opts loadOptions{};
        loadOptions.load_external_files = true;
        loadOptions.ignore_missing_external_files = true;
        loadOptions.generate_missing_normals = true;
        loadOptions.target_axes =
        {
            .right = UFBX_COORDINATE_AXIS_POSITIVE_X,
            .up = UFBX_COORDINATE_AXIS_POSITIVE_Y,
            .front = UFBX_COORDINATE_AXIS_POSITIVE_Z,
        };
        loadOptions.target_unit_meters = 1.0f;

        ufbx_error error;
        ufbx_scene* ufbxScene = ufbx_load_file(path.string().c_str(), &loadOptions, &error);
        if (!ufbxScene)
        {
            HBL2_CORE_ERROR("Failed to load: {0}\n", error.description.data);
            ufbx_free_scene(ufbxScene);
            return Handle<Mesh>();
        }

        LoadMaterials(ufbxScene, path);

        SkeletonImport skeletonImport = BuildSkeleton(ufbxScene, path);

        std::vector<MeshPartDescriptor> meshes;
        uint32_t meshIndex = 0;

        for (size_t i = 0; i < ufbxScene->nodes.count; i++)
        {
            ufbx_node* node = ufbxScene->nodes.data[i];
            if (node->is_root)
            {
                continue;
            } 

            HBL2_CORE_TRACE("Object: {0}\n", node->name.data);
            if (node->mesh)
            {
                auto result = LoadMeshData(node, meshIndex++, skeletonImport);

                if (result.IsOk())
                {
                    meshes.push_back(result.Unwrap());
                }
            }
        }

        if (meshes.size() == 0)
        {
            ufbx_free_scene(ufbxScene);
            return Handle<Mesh>();
        }

        Handle<Skeleton> skeleton;
        std::vector<Handle<AnimationClip>> animationClips;
        CreateSkeletonAndClips(ufbxScene, skeletonImport, skeleton, animationClips);

        ufbx_free_scene(ufbxScene);

        Handle<Mesh> handle = HBL2::ResourceManager::Instance->CreateMesh({
            .debugName = strdup(path.filename().stem().string().c_str()),
            .meshes = std::move(meshes),
            .skeleton = skeleton,
            .animationClips = std::move(animationClips),
        });

        return handle;
	}

    void UFbxLoader::Reload(Asset* asset)
    {
        HBL2_FUNC_PROFILE();

        ufbx_load_opts loadOptions{};
        loadOptions.load_external_files = true;
        loadOptions.ignore_missing_external_files = true;
        loadOptions.generate_missing_normals = true;
        loadOptions.target_axes =
        {
            .right = UFBX_COORDINATE_AXIS_POSITIVE_X,
            .up = UFBX_COORDINATE_AXIS_POSITIVE_Y,
            .front = UFBX_COORDINATE_AXIS_POSITIVE_Z,
        };
        loadOptions.target_unit_meters = 1.0f;

        const auto& fileSystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
        const std::filesystem::path& meshPath = std::filesystem::exists(fileSystemPath) ? fileSystemPath : asset->FilePath;

        ufbx_error error;
        ufbx_scene* ufbxScene = ufbx_load_file(meshPath.string().c_str(), &loadOptions, &error);
        if (!ufbxScene)
        {
            HBL2_CORE_ERROR("Failed to load: {0}\n", error.description.data);
            ufbx_free_scene(ufbxScene);
            return;
        }

        ReloadMaterials(ufbxScene, meshPath, asset);

        SkeletonImport skeletonImport = BuildSkeleton(ufbxScene, meshPath);

        std::vector<MeshPartDescriptor> meshes;
        uint32_t meshIndex = 0;

        for (size_t i = 0; i < ufbxScene->nodes.count; i++)
        {
            ufbx_node* node = ufbxScene->nodes.data[i];
            if (node->is_root)
            {
                continue;
            }

            HBL2_CORE_TRACE("Object: {0}\n", node->name.data);
            if (node->mesh)
            {
                auto result = LoadMeshData(node, meshIndex++, skeletonImport);

                if (result.IsOk())
                {
                    meshes.push_back(result.Unwrap());
                }
            }
        }

        if (meshes.size() == 0)
        {
            ufbx_free_scene(ufbxScene);
            return;
        }

        Handle<Mesh> handle = Handle<Mesh>::UnPack(asset->Indentifier);
        Mesh* mesh = ResourceManager::Instance->GetMesh(handle);

        if (mesh == nullptr)
        {
            ufbx_free_scene(ufbxScene);
            return;
        }

        mesh->MarkAsEmpty();

        std::vector<Handle<Buffer>> previousBuffers;

        // Collect previous buffers.
        for (const MeshPart& meshPart : mesh->Meshes)
        {
            previousBuffers.push_back(meshPart.IndexBuffer);
            previousBuffers.push_back(meshPart.SkinBuffer);

            for (Handle<Buffer> vb : meshPart.VertexBuffers)
            {
                previousBuffers.push_back(vb);
            }
        }

        const Handle<Skeleton> previousSkeleton = mesh->Skeleton;
        const std::vector<Handle<AnimationClip>> previousClips = mesh->AnimationClips;

        Handle<Skeleton> newSkeleton;
        std::vector<Handle<AnimationClip>> newClips;
        CreateSkeletonAndClips(ufbxScene, skeletonImport, newSkeleton, newClips);

        ufbx_free_scene(ufbxScene);

        ResourceManager::Instance->ReimportMesh(handle, {
            .debugName = strdup(meshPath.filename().stem().string().c_str()),
            .meshes = std::move(meshes),
            .skeleton = newSkeleton,
            .animationClips = std::move(newClips),
        });

        // Delete previous buffers.
        for (Handle<Buffer> ib : previousBuffers)
        {
            ResourceManager::Instance->DeleteBuffer(ib);
        }

        ResourceManager::Instance->DeleteSkeleton(previousSkeleton);
    }

    void UFbxLoader::LoadMaterials(ufbx_scene* ufbxScene, const std::filesystem::path& path)
    {
        auto* editorAssetManager = (EditorAssetManager*)AssetManager::Instance;

        s_MaterialNameToAssetHandle.clear();

        uint32_t numMaterials = (uint32_t)ufbxScene->materials.count;

        for (uint32_t materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
        {
            const ufbx_material* fbxMaterial = ufbxScene->materials[materialIndex];

            Handle<Asset> materialAssetHandle;
            Handle<Material> materialHandle;

            const auto& relativePath = std::filesystem::path("AutoImported") / path.filename().stem() / "Materials" / (std::string(fbxMaterial->name.data) + ".mat");

            if (!std::filesystem::exists(Project::GetAssetFileSystemPath(relativePath)))
            {
                materialAssetHandle = editorAssetManager->CreateAsset({
                    .debugName = "material-asset",
                    .filePath = relativePath,
                    .type = AssetType::Material,
                });

                JobContext ctx;

                glm::vec4 albedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };
                Handle<Asset> albedoMapAssetHandle = LoadMaterial(path, fbxMaterial, UFBX_MATERIAL_PBR_BASE_COLOR, ctx, false, &albedoColor);

                double roughness = 1.0;
                Handle<Asset> roughnessMapAssetHandle = LoadMaterial(path, fbxMaterial, UFBX_MATERIAL_PBR_ROUGHNESS, ctx, false, &roughness);

                double metallicness = 1.0;
                Handle<Asset> metallicMapAssetHandle = LoadMaterial(path, fbxMaterial, UFBX_MATERIAL_PBR_METALNESS, ctx, false, &metallicness);

                Handle<Asset> normalMapAssetHandle = LoadMaterial(path, fbxMaterial, UFBX_MATERIAL_PBR_NORMAL_MAP, ctx, false);

                AssetManager::Instance->WaitForAsyncJobs(&ctx);

                // Get material uniform buffer data.
                struct MaterialParams { glm::vec4 Color; float Roughness; float Metallic; };
                MaterialParams params{ albedoColor, (float)roughness, (float)metallicness };

                std::vector<uint8_t> uniformBufferBytes(sizeof(MaterialParams));
                memcpy(uniformBufferBytes.data(), &params, sizeof(params));

                Handle<Asset> shaderAssetHandle;
                ShaderReflectionData shaderReflectionData;
                bool isPBR = normalMapAssetHandle.IsValid() && roughnessMapAssetHandle.IsValid() && metallicMapAssetHandle.IsValid();

                MeshUtilities::Get().GetShaderAssetHandleAndReflectionData(shaderAssetHandle, &shaderReflectionData, isPBR);

                ShaderUtilities::Get().CreateMaterialAssetFile(materialAssetHandle, {
                    .ShaderAssetHandle = shaderAssetHandle,
                    .VariantHash =
                    {
                        .blendEnabled = false,
                        .depthWrite = false,
                        .depthCompare = (ShaderDescriptor::RenderPipeline::packed_size)Compare::LESS_OR_EQUAL,
                    },
                    .ReflectionData = &shaderReflectionData,
                    .Buffers = { uniformBufferBytes },
                    .TextureAssets = { albedoMapAssetHandle.Pack(), normalMapAssetHandle.Pack(), metallicMapAssetHandle.Pack(), roughnessMapAssetHandle.Pack() },
                });

                if (materialAssetHandle.IsValid())
                {
                    uint32_t type = UINT32_MAX;

                    switch (fbxMaterial->shader_type)
                    {
                    case UFBX_SHADER_WAVEFRONT_MTL:
                    case UFBX_SHADER_FBX_LAMBERT:
                    case UFBX_SHADER_FBX_PHONG:
                        type = 1;
                        break;
                    case UFBX_SHADER_ARNOLD_STANDARD_SURFACE:
                    case UFBX_SHADER_OSL_STANDARD_SURFACE:
                        type = 2;
                        break;
                    default:
                        HBL2_CORE_ASSERT(false, "Error while loading FBX model, fbxMaterial->shader_type not supported. Add supprt if applicable.");
                    }

                    ShaderUtilities::Get().CreateMaterialMetadataFile(materialAssetHandle, type, true);
                }

                materialHandle = AssetManager::Instance->GetAsset<Material>(materialAssetHandle);
            }
            else
            {
                UUID materialAssetUUID = editorAssetManager->GetUUIDFromPath(relativePath);
                materialAssetHandle = editorAssetManager->GetHandleFromUUID(materialAssetUUID);
                materialHandle = AssetManager::Instance->GetAsset<Material>(materialAssetHandle);
            }

            s_MaterialNameToAssetHandle[fbxMaterial->name.data] = materialAssetHandle;
        }
    }

    void UFbxLoader::ReloadMaterials(ufbx_scene* ufbxScene, const std::filesystem::path& path, Asset* sourceAsset)
    {
        auto* editorAssetManager = (EditorAssetManager*)AssetManager::Instance;

        s_MaterialNameToAssetHandle.clear();

        MeshSettings meshSettings = MeshUtilities::Get().DeserializeAssetMetadataFile(sourceAsset);

        uint32_t numMaterials = (uint32_t)ufbxScene->materials.count;

        for (uint32_t materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
        {
            const ufbx_material* fbxMaterial = ufbxScene->materials[materialIndex];

            Handle<Asset> materialAssetHandle;
            Handle<Material> materialHandle;

            const auto& relativePath = std::filesystem::path("AutoImported") / path.filename().stem() / "Materials" / (std::string(fbxMaterial->name.data) + ".mat");

            JobContext ctx;

            glm::vec4 albedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            Handle<Asset> albedoMapAssetHandle = LoadMaterial(path, fbxMaterial, UFBX_MATERIAL_PBR_BASE_COLOR, ctx, meshSettings.ReimportTexturesOnReload, &albedoColor);

            double roughness = 1.0;
            Handle<Asset> roughnessMapAssetHandle = LoadMaterial(path, fbxMaterial, UFBX_MATERIAL_PBR_ROUGHNESS, ctx, meshSettings.ReimportTexturesOnReload, &roughness);

            double metallicness = 1.0;
            Handle<Asset> metallicMapAssetHandle = LoadMaterial(path, fbxMaterial, UFBX_MATERIAL_PBR_METALNESS, ctx, meshSettings.ReimportTexturesOnReload, &metallicness);

            Handle<Asset> normalMapAssetHandle = LoadMaterial(path, fbxMaterial, UFBX_MATERIAL_PBR_NORMAL_MAP, ctx, meshSettings.ReimportTexturesOnReload);

            AssetManager::Instance->WaitForAsyncJobs(&ctx);

            // Get material uniform buffer data.
            struct MaterialParams { glm::vec4 Color; float Roughness; float Metallic; };
            MaterialParams params{ albedoColor, (float)roughness, (float)metallicness };

            std::vector<uint8_t> uniformBufferBytes(sizeof(MaterialParams));
            memcpy(uniformBufferBytes.data(), &params, sizeof(params));

            Handle<Asset> shaderAssetHandle;
            ShaderReflectionData shaderReflectionData;
            bool isPBR = normalMapAssetHandle.IsValid() && roughnessMapAssetHandle.IsValid() && metallicMapAssetHandle.IsValid();

            MeshUtilities::Get().GetShaderAssetHandleAndReflectionData(shaderAssetHandle, &shaderReflectionData, isPBR);

            if (!std::filesystem::exists(Project::GetAssetFileSystemPath(relativePath)))
            {
                materialAssetHandle = editorAssetManager->CreateAsset({
                    .debugName = "material-asset",
                    .filePath = relativePath,
                    .type = AssetType::Material,
                });

                ShaderUtilities::Get().CreateMaterialAssetFile(materialAssetHandle, {
                    .ShaderAssetHandle = shaderAssetHandle,
                    .VariantHash =
                    {
                        .blendEnabled = false,
                        .depthWrite = false,
                        .depthCompare = (ShaderDescriptor::RenderPipeline::packed_size)Compare::LESS_OR_EQUAL,
                    },
                    .ReflectionData = &shaderReflectionData,
                    .Buffers = { uniformBufferBytes },
                    .TextureAssets = { albedoMapAssetHandle.Pack(), normalMapAssetHandle.Pack(), metallicMapAssetHandle.Pack(), roughnessMapAssetHandle.Pack() },
                });

                if (materialAssetHandle.IsValid())
                {
                    uint32_t type = UINT32_MAX;

                    switch (fbxMaterial->shader_type)
                    {
                    case UFBX_SHADER_WAVEFRONT_MTL:
                    case UFBX_SHADER_FBX_LAMBERT:
                    case UFBX_SHADER_FBX_PHONG:
                        type = 1;
                        break;
                    case UFBX_SHADER_ARNOLD_STANDARD_SURFACE:
                        type = 2;
                        break;
                    default:
                        HBL2_CORE_ASSERT(false, "Error while loading FBX model, fbxMaterial->shader_type not supported. Add supprt if applicable.");
                    }

                    ShaderUtilities::Get().CreateMaterialMetadataFile(materialAssetHandle, type, true);
                }

                materialHandle = AssetManager::Instance->GetAsset<Material>(materialAssetHandle);
            }
            else
            {
                UUID materialAssetUUID = editorAssetManager->GetUUIDFromPath(relativePath);
                materialAssetHandle = AssetManager::Instance->GetHandleFromUUID(materialAssetUUID);

                // Recreate material asset file with new data.
                ShaderUtilities::Get().CreateMaterialAssetFile(materialAssetHandle, {
                    .ShaderAssetHandle = shaderAssetHandle,
                    .VariantHash =
                    {
                        .blendEnabled = false,
                        .depthWrite = false,
                        .depthCompare = (ShaderDescriptor::RenderPipeline::packed_size)Compare::LESS_OR_EQUAL,
                    },
                    .ReflectionData = &shaderReflectionData,
                    .Buffers = { uniformBufferBytes },
                    .TextureAssets = { albedoMapAssetHandle.Pack(), normalMapAssetHandle.Pack(), metallicMapAssetHandle.Pack(), roughnessMapAssetHandle.Pack() },
                });

                // Reload material now that we have set the new resourses.
                materialHandle = editorAssetManager->ReloadAsset<Material>(materialAssetUUID);
            }

            s_MaterialNameToAssetHandle[fbxMaterial->name.data] = materialAssetHandle;
        }
    }

    Handle<Asset> UFbxLoader::LoadMaterial(const std::filesystem::path& path, const ufbx_material* fbxMaterial, ufbx_material_pbr_map materialProperty, JobContext& ctx, bool reload, void* internalData)
    {
        Handle<Asset> textureAssetHandle;

        switch (materialProperty)
        {
        case UFBX_MATERIAL_PBR_BASE_COLOR: // aka albedo aka diffuse color
            {
                const ufbx_material_map& materialMap = fbxMaterial->pbr.base_color;
                if (materialMap.has_value)
                {
                    const ufbx_material_map& baseFactorMaterialMap = fbxMaterial->pbr.base_factor;
                    float baseFactor = baseFactorMaterialMap.has_value ? baseFactorMaterialMap.value_real : 1.0f;
                    if (materialMap.texture)
                    {
                        if (reload)
                        {
                            textureAssetHandle = ReloadTexture(materialMap.texture, ctx);
                        }
                        else
                        {
                            textureAssetHandle = LoadTexture(materialMap.texture, ctx);
                        }

                        (*((glm::vec4*)internalData)).r = baseFactor;
                        (*((glm::vec4*)internalData)).g = baseFactor;
                        (*((glm::vec4*)internalData)).b = baseFactor;
                        (*((glm::vec4*)internalData)).a = baseFactor;
                    }
                    else // constant material property
                    {
                        (*((glm::vec4*)internalData)).r = materialMap.value_vec4.x * baseFactor;
                        (*((glm::vec4*)internalData)).g = materialMap.value_vec4.y * baseFactor;
                        (*((glm::vec4*)internalData)).b = materialMap.value_vec4.z * baseFactor;
                        (*((glm::vec4*)internalData)).a = materialMap.value_vec4.w * baseFactor;
                    }
                }
                break;
            }
        case UFBX_MATERIAL_PBR_ROUGHNESS:
            {
                ufbx_material_map const& materialMap = fbxMaterial->pbr.roughness;
                if (materialMap.has_value)
                {
                    if (materialMap.texture)
                    {
                        if (reload)
                        {
                            textureAssetHandle = ReloadTexture(materialMap.texture, ctx);
                        }
                        else
                        {
                            textureAssetHandle = LoadTexture(materialMap.texture, ctx);
                        }

                        (*((double*)internalData)) = 1.0;
                    }
                    else // constant material property
                    {
                        (*((double*)internalData)) = materialMap.value_real;
                    }
                }
                break;
            }
        case UFBX_MATERIAL_PBR_METALNESS:
            {
                ufbx_material_map const& materialMap = fbxMaterial->pbr.metalness;
                if (materialMap.has_value)
                {
                    if (materialMap.texture)
                    {
                        if (reload)
                        {
                            textureAssetHandle = ReloadTexture(materialMap.texture, ctx);
                        }
                        else
                        {
                            textureAssetHandle = LoadTexture(materialMap.texture, ctx);
                        }

                        (*((double*)internalData)) = 1.0;
                    }
                    else // constant material property
                    {
                        (*((double*)internalData)) = materialMap.value_real;
                    }
                }
                break;
            }
        case UFBX_MATERIAL_PBR_NORMAL_MAP:
            {
                ufbx_material_map const& materialMap = fbxMaterial->pbr.normal_map;
                if (materialMap.texture)
                {
                    if (reload)
                    {
                        textureAssetHandle = ReloadTexture(materialMap.texture, ctx);
                    }
                    else
                    {
                        textureAssetHandle = LoadTexture(materialMap.texture, ctx);
                    }
                }
                break;
            }
        default:
            {
                HBL2_CORE_ASSERT(false, "Material property not recognized");
                break;
            }
        }

        return textureAssetHandle;
    }

    Handle<Asset> UFbxLoader::LoadTexture(const ufbx_texture* texture, JobContext& ctx)
    {
        auto* editorAssetManager = (EditorAssetManager*)AssetManager::Instance;

        Handle<Asset> textureAssetHandle;

        const auto& texturePath = std::filesystem::path(texture->filename.data);

        if (std::filesystem::exists(texturePath))
        {
            HBL2_CORE_INFO("UFbxLoader::LoadTexture::AlbedoMap located at: \"{}\".", texturePath);

            const auto& relativeTexturePath = FileUtils::RelativePath(texturePath, Project::GetAssetDirectory());

            UUID textureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeTexturePath);
            textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

            if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
            {
                textureAssetHandle = editorAssetManager->CreateAsset({
                    .debugName = "texture-asset",
                    .filePath = relativeTexturePath,
                    .type = AssetType::Texture,
                });
            }

            if (textureAssetHandle.IsValid())
            {
                TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
            }

            AssetManager::Instance->GetAssetAsync<Texture>(textureAssetHandle, &ctx);
            return textureAssetHandle;
        }

        if (texture->absolute_filename.length != 0)
        {
            const auto& texturePathAlt = Project::GetAssetFileSystemPath(std::filesystem::path(texture->absolute_filename.data));

            if (std::filesystem::exists(texturePathAlt))
            {
                HBL2_CORE_INFO("UFbxLoader::LoadTexture::AlbedoMap located at: \"{}\".", texturePathAlt);

                const auto& relativeTexturePath = FileUtils::RelativePath(texturePathAlt, Project::GetAssetDirectory());

                UUID textureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeTexturePath);
                textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

                if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
                {
                    textureAssetHandle = editorAssetManager->CreateAsset({
                        .debugName = "texture-asset",
                        .filePath = relativeTexturePath,
                        .type = AssetType::Texture,
                    });
                }

                if (textureAssetHandle.IsValid())
                {
                    TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
                }

                AssetManager::Instance->GetAssetAsync<Texture>(textureAssetHandle, &ctx);
                return textureAssetHandle;
            }
        }

        HBL2_CORE_ERROR("UFbxLoader::LoadTexture::AlbedoMap located at: \"{}\", not found!.", texture->filename.data);

        return textureAssetHandle;
    }

    Handle<Asset> UFbxLoader::ReloadTexture(const ufbx_texture* texture, JobContext& ctx)
    {
        auto* editorAssetManager = (EditorAssetManager*)AssetManager::Instance;

        Handle<Asset> textureAssetHandle;

        const auto& texturePath = std::filesystem::path(texture->filename.data);

        if (std::filesystem::exists(texturePath))
        {
            HBL2_CORE_INFO("UFbxLoader::LoadTexture::AlbedoMap located at: \"{}\".", texturePath);

            const auto& relativeTexturePath = FileUtils::RelativePath(texturePath, Project::GetAssetDirectory());

            UUID textureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeTexturePath);
            textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

            if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
            {
                textureAssetHandle = editorAssetManager->CreateAsset({
                    .debugName = "texture-asset",
                    .filePath = relativeTexturePath,
                    .type = AssetType::Texture,
                });
            }

            if (textureAssetHandle.IsValid())
            {
                TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
            }

            editorAssetManager->ReloadAssetAsync<Texture>(textureAssetHandle, &ctx);
            return textureAssetHandle;
        }

        if (texture->absolute_filename.length != 0)
        {
            const auto& texturePathAlt = Project::GetAssetFileSystemPath(std::filesystem::path(texture->absolute_filename.data));

            if (std::filesystem::exists(texturePathAlt))
            {
                HBL2_CORE_INFO("UFbxLoader::LoadTexture::AlbedoMap located at: \"{}\".", texturePathAlt);

                const auto& relativeTexturePath = FileUtils::RelativePath(texturePathAlt, Project::GetAssetDirectory());

                UUID textureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeTexturePath);
                textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

                if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
                {
                    textureAssetHandle = editorAssetManager->CreateAsset({
                        .debugName = "texture-asset",
                        .filePath = relativeTexturePath,
                        .type = AssetType::Texture,
                    });
                }

                if (textureAssetHandle.IsValid())
                {
                    TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
                }

                editorAssetManager->ReloadAssetAsync<Texture>(textureAssetHandle, &ctx);
                return textureAssetHandle;
            }
        }

        HBL2_CORE_ERROR("UFbxLoader::LoadTexture::AlbedoMap located at: \"{}\", not found!.", texture->filename.data);

        return textureAssetHandle;
    }

    SkeletonImport UFbxLoader::BuildSkeleton(const ufbx_scene* scene, const std::filesystem::path& path)
    {
        SkeletonImport out;

        if (scene->skin_clusters.count == 0)
        {
            return out;
        }

        const size_t numNodes = scene->nodes.count;

        // 1. Joint set: every bone used by a skin cluster, plus all of its ancestors (stopping at the scene root).
        std::vector<uint8_t> isJoint(numNodes, 0);
        for (size_t c = 0; c < scene->skin_clusters.count; ++c)
        {
            for (const ufbx_node* n = scene->skin_clusters[c]->bone_node; n && !n->is_root; n = n->parent)
            {
                isJoint[n->typed_id] = 1;
            }
        }

        // Depth of every node (the scene root's children have depth 0).
        std::vector<uint32_t> depth(numNodes, 0);
        for (size_t i = 0; i < numNodes; ++i)
        {
            uint32_t d = 0;
            for (const ufbx_node* p = scene->nodes[i]->parent; p && !p->is_root; p = p->parent) { ++d; }
            depth[scene->nodes[i]->typed_id] = d;
        }

        std::vector<const ufbx_node*> joints;
        for (size_t i = 0; i < numNodes; ++i)
        {
            const ufbx_node* node = scene->nodes[i];
            if (isJoint[node->typed_id]) { joints.push_back(node); }
        }

        const size_t numJoints = joints.size();
        if (numJoints == 0 || numJoints > INT16_MAX)
        {
            HBL2_CORE_ERROR("{}: unsupported joint count {}.", path.filename().string(), numJoints);
            return out;
        }

        // Parents before children.
        std::stable_sort(joints.begin(), joints.end(), [&](const ufbx_node* a, const ufbx_node* b)
            {
                return depth[a->typed_id] < depth[b->typed_id];
            });

        out.NodeToJoint.assign(numNodes, -1);
        for (size_t i = 0; i < numJoints; ++i)
        {
            out.NodeToJoint[joints[i]->typed_id] = (int32_t)i;
        }

        // 2. Bind world matrix of every joint: from the skin cluster when there is one, else the node's default pose.
        std::vector<ufbx_matrix> bindWorld(numJoints);
        for (size_t i = 0; i < numJoints; ++i)
        {
            bindWorld[i] = joints[i]->node_to_world;
        }

        for (size_t c = 0; c < scene->skin_clusters.count; ++c)
        {
            const ufbx_skin_cluster* cluster = scene->skin_clusters[c];
            if (!cluster->bone_node) continue;

            const int32_t joint = out.NodeToJoint[cluster->bone_node->typed_id];
            if (joint >= 0)
            {
                bindWorld[joint] = cluster->bind_to_world;
            }
        }

        // 3. Build the descriptor. Unit/axis conversion lives on the scene root, which becomes Skeleton::RootTransform.
        const ufbx_matrix rootWorld = scene->root_node->node_to_world;
        const ufbx_matrix rootWorldInverse = ufbx_matrix_invert(&rootWorld);

        SkeletonDescriptor& d = out.Descriptor;
        d.debugName = strdup(path.filename().stem().string().c_str());
        d.rootTransform = ToGlm(rootWorld);
        d.parents.resize(numJoints);
        d.names.resize(numJoints);
        d.nameHashes.resize(numJoints);
        d.inverseBind.resize(numJoints);
        d.bindPose.resize(numJoints);

        for (size_t i = 0; i < numJoints; ++i)
        {
            const ufbx_node* node = joints[i];

            const bool hasParentJoint = node->parent && !node->parent->is_root;
            const int32_t parent = hasParentJoint ? out.NodeToJoint[node->parent->typed_id] : -1;
            d.parents[i] = (int16_t)parent;

            d.names[i] = node->name.length ? std::string(node->name.data, node->name.length) : ("joint_" + std::to_string(i));
            d.nameHashes[i] = HashJointName(d.names[i]);

            const ufbx_matrix ibm = ufbx_matrix_invert(&bindWorld[i]);
            d.inverseBind[i] = ToGlm(ibm);

            const ufbx_matrix parentInverse = parent < 0 ? rootWorldInverse : ufbx_matrix_invert(&bindWorld[parent]);
            const ufbx_matrix local = ufbx_matrix_mul(&parentInverse, &bindWorld[i]);
            d.bindPose[i] = ToJointTransform(ufbx_matrix_to_transform(&local));
        }

        out.SkinIndex = 0;
        out.Valid = true;
        return out;
    }

    void UFbxLoader::BuildAnimationClips(const ufbx_scene* scene, const SkeletonImport& skeleton, std::vector<AnimationClipDescriptor>& out)
    {
        if (!skeleton.Valid)
        {
            if (scene->anim_stacks.count > 0)
            {
                HBL2_CORE_WARN("File has animations but no skin, skipping them (node animation isn't supported).");
            }
            return;
        }

        ufbx_bake_opts bakeOpts{};
        bakeOpts.resample_rate = 30.0;
        bakeOpts.key_reduction_enabled = true;

        for (size_t stackIndex = 0; stackIndex < scene->anim_stacks.count; ++stackIndex)
        {
            const ufbx_anim_stack* stack = scene->anim_stacks[stackIndex];

            ufbx_error error;
            ufbx_baked_anim* baked = ufbx_bake_anim(scene, stack->anim, &bakeOpts, &error);
            if (!baked)
            {
                HBL2_CORE_WARN("Failed to bake animation '{}': {}", stack->name.data, error.description.data);
                continue;
            }

            std::vector<AnimationSampler> samplers;
            std::vector<AnimationChannel> channels;
            uint32_t skipped = 0;

            auto addChannel = [&](AnimationSampler&& sampler, AnimationChannel::PathType path, uint16_t joint)
                {
                    if (sampler.inputs.empty()) return;

                    AnimationChannel c;
                    c.Path = path;
                    c.JointIndex = joint;
                    c.SamplerIndex = (uint32_t)samplers.size();

                    channels.push_back(c);
                    samplers.push_back(std::move(sampler));
                };

            for (size_t n = 0; n < baked->nodes.count; ++n)
            {
                const ufbx_baked_node& bn = baked->nodes[n];

                const int32_t joint = bn.typed_id < skeleton.NodeToJoint.size() ? skeleton.NodeToJoint[bn.typed_id] : -1;
                if (joint < 0)
                {
                    ++skipped;   // animates something that isn't part of the skeleton
                    continue;
                }

                // Keys with a non-increasing time are dropped, SampleClip divides by the key interval.
                AnimationSampler t, r, s;
                t.interpolation = r.interpolation = s.interpolation = AnimationSampler::LINEAR;

                for (size_t k = 0; k < bn.translation_keys.count; ++k)
                {
                    const auto& key = bn.translation_keys[k];
                    if (!t.inputs.empty() && (float)key.time <= t.inputs.back()) continue;
                    t.inputs.push_back((float)key.time);
                    t.values.emplace_back((float)key.value.x, (float)key.value.y, (float)key.value.z, 0.f);
                }

                for (size_t k = 0; k < bn.rotation_keys.count; ++k)
                {
                    const auto& key = bn.rotation_keys[k];
                    if (!r.inputs.empty() && (float)key.time <= r.inputs.back()) continue;

                    glm::vec4 q((float)key.value.x, (float)key.value.y, (float)key.value.z, (float)key.value.w);   // xyzw, like glTF
                    const float len = glm::length(q);
                    q = len > 1e-6f ? q / len : glm::vec4(0.f, 0.f, 0.f, 1.f);

                    r.inputs.push_back((float)key.time);
                    r.values.push_back(q);
                }

                for (size_t k = 0; k < bn.scale_keys.count; ++k)
                {
                    const auto& key = bn.scale_keys[k];
                    if (!s.inputs.empty() && (float)key.time <= s.inputs.back()) continue;
                    s.inputs.push_back((float)key.time);
                    s.values.emplace_back((float)key.value.x, (float)key.value.y, (float)key.value.z, 0.f);
                }

                addChannel(std::move(t), AnimationChannel::TRANSLATION, (uint16_t)joint);
                addChannel(std::move(r), AnimationChannel::ROTATION, (uint16_t)joint);
                addChannel(std::move(s), AnimationChannel::SCALE, (uint16_t)joint);
            }

            ufbx_free_baked_anim(baked);

            if (skipped > 0)
            {
                HBL2_CORE_WARN("Animation '{}': skipped {} animated node(s) that aren't part of the skeleton.", stack->name.data, skipped);
            }

            if (channels.empty())
            {
                HBL2_CORE_WARN("Animation '{}' has no usable channels, skipping.", stack->name.data);
                continue;
            }

            // Shift so the clip starts at 0 (one shared offset, so channels stay in sync).
            float minT = std::numeric_limits<float>::max();
            float maxT = std::numeric_limits<float>::lowest();

            for (const AnimationSampler& sampler : samplers)
            {
                minT = std::min(minT, sampler.inputs.front());
                maxT = std::max(maxT, sampler.inputs.back());
            }

            for (AnimationSampler& sampler : samplers)
            {
                for (float& time : sampler.inputs) { time -= minT; }
            }

            AnimationClipDescriptor desc;
            desc.name = stack->name.length ? std::string(stack->name.data, stack->name.length) : ("animation_" + std::to_string(stackIndex));
            desc.samplers = std::move(samplers);
            desc.channels = std::move(channels);
            desc.duration = maxT - minT;
            out.push_back(std::move(desc));
        }
    }

    void UFbxLoader::CreateSkeletonAndClips(const ufbx_scene * scene, SkeletonImport & skeletonImport, Handle<Skeleton>&skeleton, std::vector<Handle<AnimationClip>>&clips)
    {
        if (!skeletonImport.Valid)
        {
            return;
        }

        skeleton = ResourceManager::Instance->CreateSkeleton(std::move(skeletonImport.Descriptor));

        std::vector<AnimationClipDescriptor> clipDescriptors;
        BuildAnimationClips(scene, skeletonImport, clipDescriptors);

        for (AnimationClipDescriptor& d : clipDescriptors)
        {
            d.skeleton = skeleton;
            clips.push_back(ResourceManager::Instance->CreateAnimationClip(std::move(d)));
        }
    }

    Result<MeshPartDescriptor> UFbxLoader::LoadMeshData(const ufbx_node* node, uint32_t meshIndex, const SkeletonImport& skeleton)
    {
        s_Vertices.clear();
        s_Indeces.clear();
        s_SkinVertices.clear();

        MeshPartDescriptor meshPartDescriptor{};

        ufbx_mesh& fbxMesh = *node->mesh; // mesh for this node, contains submeshes

        const FbxMeshSkin skin = BuildMeshSkin(fbxMesh, skeleton);
        const bool skinned = skin.Valid;

        uint32_t numSubMeshes = (uint32_t)fbxMesh.material_parts.count;
        if (numSubMeshes)
        {
            meshPartDescriptor.debugName = strdup(node->name.data);
            meshPartDescriptor.subMeshes.reserve(numSubMeshes);

            for (uint32_t subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
            {
                auto result = LoadSubMeshVertexData(node, meshIndex, subMeshIndex, skin);

                if (result.IsOk())
                {
                    meshPartDescriptor.subMeshes.emplace_back(std::move(result.Unwrap()));
                }
                else
                {
                    HBL2_CORE_ERROR(result.GetError());
                    continue;
                }

                auto& localTr = node->local_transform.translation;
                auto& localRot = node->euler_rotation;
                auto& localScl = node->local_transform.scale;

                meshPartDescriptor.importedLocalTransform.translation = { localTr.x, localTr.y, localTr.z };
                meshPartDescriptor.importedLocalTransform.rotation = { localRot.x, localRot.y, localRot.z };
                meshPartDescriptor.importedLocalTransform.scale = { localScl.x, localScl.y, localScl.z };
            }

            auto& imported = meshPartDescriptor.importedLocalTransform;

            if (skinned)
            {
                // Vertices were baked into world bind space, the joints drive everything (same as glTF).
                imported.translation = { 0.f, 0.f, 0.f };
                imported.rotation = { 0.f, 0.f, 0.f };
                imported.scale = { 1.f, 1.f, 1.f };
            }
            else
            {
                auto& localTr = node->local_transform.translation;
                auto& localRot = node->euler_rotation;
                auto& localScl = node->local_transform.scale;

                imported.translation = { localTr.x, localTr.y, localTr.z };
                imported.rotation = { localRot.x, localRot.y, localRot.z };
                imported.scale = { localScl.x, localScl.y, localScl.z };
            }

            auto vertexBuffer = ResourceManager::Instance->CreateBuffer({
                .debugName = "fbx-mesh-vertex-buffer",
                .usage = (skinned ? BitFlags<BufferUsage>{ BufferUsage::VERTEX, BufferUsage::STORAGE } : BufferUsage::VERTEX),
                .memoryUsage = MemoryUsage::GPU_ONLY,
                .byteSize = (uint32_t)(s_Vertices.size() * sizeof(Vertex)),
                .initialData = (void*)s_Vertices.data(),
            });

            meshPartDescriptor.vertexBuffers = { vertexBuffer };

            if (skinned && !s_SkinVertices.empty())
            {
                meshPartDescriptor.skinBuffer = ResourceManager::Instance->CreateBuffer({
                    .debugName = "fbx-mesh-skin-buffer",
                    .usage = BufferUsage::STORAGE,
                    .memoryUsage = MemoryUsage::GPU_ONLY,
                    .byteSize = (uint32_t)(s_SkinVertices.size() * sizeof(SkinVertex)),
                    .initialData = (void*)s_SkinVertices.data(),
                });
            }

            if (s_Indeces.size() > 0)
            {
                meshPartDescriptor.indexBuffer = ResourceManager::Instance->CreateBuffer({
                    .debugName = "fbx-mesh-index-buffer",
                    .usage = BufferUsage::INDEX,
                    .memoryUsage = MemoryUsage::GPU_ONLY,
                    .byteSize = (uint32_t)(s_Indeces.size() * sizeof(uint32_t)),
                    .initialData = (void*)s_Indeces.data(),
                });
            }
        }

        return meshPartDescriptor;
    }

    Result<SubMeshDescriptor> UFbxLoader::LoadSubMeshVertexData(const ufbx_node* node, uint32_t meshIndex, uint32_t subMeshIndex, const FbxMeshSkin& skin)
    {
        SubMeshDescriptor subMeshDescriptor{};
        subMeshDescriptor.debugName = node->materials.count > 0 ? strdup(node->materials[subMeshIndex]->name.data) : strdup(node->name.data);
        subMeshDescriptor.minVertex = { (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)() };
        subMeshDescriptor.maxVertex = { (std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::lowest)() };

        ufbx_mesh& fbxMesh = *node->mesh; // mesh for this node, contains submeshes
        const ufbx_mesh_part& fbxSubmesh = node->mesh->material_parts[subMeshIndex];
        size_t numFaces = fbxSubmesh.num_faces;

        if (node->mesh->materials.count > 0)
        {
            const std::string materialName = node->mesh->materials.data[subMeshIndex]->name.data;
            subMeshDescriptor.embededMaterial = s_MaterialNameToAssetHandle[materialName];
        }

        if (!(fbxSubmesh.num_triangles))
        {
            return Error("UFbxLoader::LoadSubMeshVertexData: only triangle meshes are supported");
        }

        size_t numVerticesBefore = s_Vertices.size();
        size_t numIndicesBefore = s_Indeces.size();

        subMeshDescriptor.vertexOffset = (uint32_t)numVerticesBefore;
        subMeshDescriptor.indexOffset = (uint32_t)numIndicesBefore;
        subMeshDescriptor.vertexCount = 0;
        subMeshDescriptor.indexCount = 0;

        bool hasUVs = fbxMesh.uv_sets.count;

        for (size_t fbxFaceIndex = 0; fbxFaceIndex < numFaces; ++fbxFaceIndex)
        {
            ufbx_face& fbxFace = fbxMesh.faces[fbxSubmesh.face_indices.data[fbxFaceIndex]];
            size_t numTriangleIndices = fbxMesh.max_face_triangles * 3;
            std::vector<uint32_t> verticesPerFaceIndexBuffer(numTriangleIndices);
            size_t numTriangles = ufbx_triangulate_face(verticesPerFaceIndexBuffer.data(), numTriangleIndices, &fbxMesh, fbxFace);
            size_t numVerticesPerFace = 3 * numTriangles;

            for (uint32_t vertexPerFace = 0; vertexPerFace < numVerticesPerFace; ++vertexPerFace)
            {
                uint32_t vertexPerFaceIndex = verticesPerFaceIndexBuffer[vertexPerFace];

                Vertex vertex{};

                // position (skinned meshes are baked into world bind space)
                uint32_t fbxVertexIndex = fbxMesh.vertex_indices[vertexPerFaceIndex];
                {
                    ufbx_vec3 positionFbx = fbxMesh.vertices[fbxVertexIndex];
                    if (skin.Valid)
                    {
                        positionFbx = ufbx_transform_position(&skin.GeometryToWorldBind, positionFbx);
                    }

                    vertex.Position = glm::vec3(positionFbx.x, positionFbx.y, positionFbx.z);

                    subMeshDescriptor.minVertex = glm::min(vertex.Position, subMeshDescriptor.minVertex);
                    subMeshDescriptor.maxVertex = glm::max(vertex.Position, subMeshDescriptor.maxVertex);
                }

                // normals, always defined if `ufbx_load_opts.generate_missing_normals` is used
                {
                    uint32_t fbxNormalIndex = fbxMesh.vertex_normal.indices[vertexPerFaceIndex];
                    HBL2_CORE_ASSERT(fbxNormalIndex < fbxMesh.vertex_normal.values.count, "LoadSubMeshVertexData: memory violation normals");

                    ufbx_vec3 normalFbx = fbxMesh.vertex_normal.values.data[fbxNormalIndex];
                    if (skin.Valid)
                    {
                        normalFbx = ufbx_transform_direction(&skin.NormalMatrix, normalFbx);
                    }

                    glm::vec3 normal(normalFbx.x, normalFbx.y, normalFbx.z);
                    const float len = glm::length(normal);
                    vertex.Normal = (skin.Valid && len > 1e-6f) ? normal / len : normal;
                }

                // uv coordinates
                if (hasUVs)
                {
                    uint32_t fbxUVIndex = fbxMesh.vertex_uv.indices[vertexPerFaceIndex];
                    HBL2_CORE_ASSERT(fbxUVIndex < fbxMesh.vertex_uv.values.count, "LoadSubMeshVertexData: memory violation uv coordinates");
                    ufbx_vec2& uvFbx = fbxMesh.vertex_uv.values.data[fbxUVIndex];
                    vertex.UV = glm::vec2(uvFbx.x, uvFbx.y);
                }

                s_Vertices.push_back(vertex);

                if (skin.Valid)
                {
                    s_SkinVertices.push_back(BuildSkinVertex(skin, fbxVertexIndex));
                }
            }
        }

        // resolve indices
        {
            uint32_t submeshAllVertices = (uint32_t)(s_Vertices.size() - numVerticesBefore);

            // Vertices only merge when every stream matches, so identical positions with different skin weights stay separate.
            ufbx_vertex_stream streams[2];
            streams[0].data = &s_Vertices[numVerticesBefore];
            streams[0].vertex_count = submeshAllVertices;
            streams[0].vertex_size = sizeof(Vertex);

            size_t numStreams = 1;
            if (skin.Valid)
            {
                streams[1].data = &s_SkinVertices[numVerticesBefore];
                streams[1].vertex_count = submeshAllVertices;
                streams[1].vertex_size = sizeof(SkinVertex);
                numStreams = 2;
            }

            s_Indeces.resize(numIndicesBefore + submeshAllVertices);

            ufbx_error ufbxError;
            size_t numVertices = ufbx_generate_indices(streams, numStreams, &s_Indeces[numIndicesBefore], submeshAllVertices, nullptr, &ufbxError);

            if (ufbxError.type != UFBX_ERROR_NONE)
            {
                char errorBuffer[512];
                ufbx_format_error(errorBuffer, sizeof(errorBuffer), &ufbxError);
                return Error(std::format("UFbxLoader::LoadSubMeshVertexData: creation of index buffer failed, error: {},  node: {}", errorBuffer, node->name.data));
            }

            s_Vertices.resize(numVerticesBefore + numVertices);
            if (skin.Valid)
            {
                s_SkinVertices.resize(numVerticesBefore + numVertices);
            }

            subMeshDescriptor.vertexCount = (uint32_t)numVertices;
            subMeshDescriptor.indexCount = submeshAllVertices;
        }

        return subMeshDescriptor;
    }
}
