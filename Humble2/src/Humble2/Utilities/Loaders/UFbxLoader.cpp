#include "UFbxLoader.h"

#include <Project\Project.h>

#include <Resources\ResourceManager.h>

#include <Utilities\ShaderUtilities.h>
#include <Utilities\TextureUtilities.h>

#include <Utilities\FileDialogs.h>

namespace HBL2
{
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
        m_Scene = ufbx_load_file(path.string().c_str(), &loadOptions, &error);
        if (!m_Scene)
        {
            HBL2_CORE_ERROR("Failed to load: {0}\n", error.description.data);
            ufbx_free_scene(m_Scene);
            return Handle<Mesh>();
        }

        LoadMaterials(path);

        std::vector<MeshPartDescriptor> meshes;
        uint32_t meshIndex = 0;

        for (size_t i = 0; i < m_Scene->nodes.count; i++)
        {
            ufbx_node* node = m_Scene->nodes.data[i];
            if (node->is_root)
            {
                continue;
            }

            HBL2_CORE_TRACE("Object: {0}\n", node->name.data);
            if (node->mesh)
            {
                auto result = LoadMeshData(node, meshIndex++);

                if (result.IsOk())
                {
                    meshes.push_back(result.Unwrap());
                }
            }
        }

        ufbx_free_scene(m_Scene);

        if (meshes.size() == 0)
        {
            return Handle<Mesh>();
        }

        Handle<Mesh> handle = HBL2::ResourceManager::Instance->CreateMesh({
            .debugName = _strdup(path.filename().stem().string().c_str()),
            .meshes = std::move(meshes),
        });

        return handle;
	}

    void UFbxLoader::LoadMaterials(const std::filesystem::path& path)
    {
        m_MaterialNameToHandle.clear();

        uint32_t numMaterials = m_Scene->materials.count;

        for (uint32_t materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
        {
            const ufbx_material* fbxMaterial = m_Scene->materials[materialIndex];

            Handle<Asset> materialAssetHandle;
            Handle<Material> materialHandle;

            const auto& relativePath = std::filesystem::path("AutoImported") / path.filename().stem() / "Materials" / (std::string(fbxMaterial->name.data) + ".mat");

            if (!std::filesystem::exists(Project::GetAssetFileSystemPath(relativePath)))
            {
                materialAssetHandle = AssetManager::Instance->CreateAsset({
                    .debugName = "material-asset",
                    .filePath = relativePath,
                    .type = AssetType::Material,
                });                

                glm::vec4 albedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };
                Handle<Asset> albedoMapAssetHandle = LoadMaterial(path, fbxMaterial, UFBX_MATERIAL_PBR_BASE_COLOR, &albedoColor);

                double roughness = 1.0;
                Handle<Asset> roughnessMapAssetHandle = LoadMaterial(path, fbxMaterial, UFBX_MATERIAL_PBR_ROUGHNESS, &roughness);

                double metallicness = 1.0;
                Handle<Asset> metallicMapAssetHandle = LoadMaterial(path, fbxMaterial, UFBX_MATERIAL_PBR_METALNESS, &metallicness);

                Handle<Asset> normalMapAssetHandle = LoadMaterial(path, fbxMaterial, UFBX_MATERIAL_PBR_NORMAL_MAP);

                AssetManager::Instance->WaitForAsyncJobs();

                ShaderUtilities::Get().CreateMaterialAssetFile(materialAssetHandle, {
                    .ShaderAssetHandle = {}, // Use built-in shaders depending on material type.
                    .AlbedoColor = albedoColor,
                    .Glossiness = (float)roughness,
                    .AlbedoMapAssetHandle = albedoMapAssetHandle,
                    .NormalMapAssetHandle = normalMapAssetHandle,
                    .RoughnessMapAssetHandle = roughnessMapAssetHandle,
                    .MetallicMapAssetHandle = metallicMapAssetHandle,
                });

                if (materialAssetHandle.IsValid())
                {
                    uint32_t type = UINT32_MAX;

                    // TODO: Handle PBR models!
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

                    ShaderUtilities::Get().CreateMaterialMetadataFile(materialAssetHandle, type);
                }

                materialHandle = AssetManager::Instance->GetAsset<Material>(materialAssetHandle);
            }
            else
            {
                UUID materialAssetUUID = std::hash<std::string>()(relativePath.string());
                materialHandle = AssetManager::Instance->GetAsset<Material>(materialAssetUUID);
            }

            CleanUpResourceTasks();

            m_MaterialNameToHandle[fbxMaterial->name.data] = materialHandle;
        }
    }

    Handle<Asset> UFbxLoader::LoadMaterial(const std::filesystem::path& path, const ufbx_material* fbxMaterial, ufbx_material_pbr_map materialProperty, void* internalData)
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
                        textureAssetHandle = LoadTexture(materialMap.texture, m_AlbedoMapTask);

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
                        textureAssetHandle = LoadTexture(materialMap.texture, m_RoughnessMapTask);

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
                        textureAssetHandle = LoadTexture(materialMap.texture, m_MetallicMapTask);

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
                    textureAssetHandle = LoadTexture(materialMap.texture, m_NormalMapTask);
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

    Handle<Asset> UFbxLoader::LoadTexture(const ufbx_texture* texture, ResourceTask<Texture>* resourceTask)
    {
        Handle<Asset> textureAssetHandle;

        const auto& texturePath = std::filesystem::path(texture->filename.data);

        if (std::filesystem::exists(texturePath))
        {
            HBL2_CORE_INFO("UFbxLoader::LoadTexture::AlbedoMap located at: \"{}\".", texturePath);

            const auto& relativeTexturePath = FileUtils::RelativePath(texturePath, Project::GetAssetDirectory());

            UUID textureAssetUUID = std::hash<std::string>()(relativeTexturePath);
            textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

            if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
            {
                textureAssetHandle = AssetManager::Instance->CreateAsset({
                    .debugName = "texture-asset",
                    .filePath = relativeTexturePath,
                    .type = AssetType::Texture,
                });
            }

            if (textureAssetHandle.IsValid())
            {
                TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
            }

            resourceTask = AssetManager::Instance->GetAssetAsync<Texture>(textureAssetHandle);
            return textureAssetHandle;
        }

        if (texture->absolute_filename.length != 0)
        {
            const auto& texturePathAlt = Project::GetAssetFileSystemPath(std::filesystem::path(texture->absolute_filename.data));

            if (std::filesystem::exists(texturePathAlt))
            {
                HBL2_CORE_INFO("UFbxLoader::LoadTexture::AlbedoMap located at: \"{}\".", texturePathAlt);

                const auto& relativeTexturePath = FileUtils::RelativePath(texturePathAlt, Project::GetAssetDirectory());

                UUID textureAssetUUID = std::hash<std::string>()(relativeTexturePath);
                textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

                if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
                {
                    textureAssetHandle = AssetManager::Instance->CreateAsset({
                        .debugName = "texture-asset",
                        .filePath = relativeTexturePath,
                        .type = AssetType::Texture,
                    });
                }

                if (textureAssetHandle.IsValid())
                {
                    TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
                }

                resourceTask = AssetManager::Instance->GetAssetAsync<Texture>(textureAssetHandle);
                return textureAssetHandle;
            }
        }

        HBL2_CORE_ERROR("UFbxLoader::LoadTexture::AlbedoMap located at: \"{}\", not found!.", texture->filename.data);

        return textureAssetHandle;
    }

    void UFbxLoader::CleanUpResourceTasks()
    {
        if (m_AlbedoMapTask)
        {
            delete m_AlbedoMapTask;
            m_AlbedoMapTask = nullptr;
        }

        if (m_NormalMapTask)
        {
            delete m_NormalMapTask;
            m_NormalMapTask = nullptr;
        }

        if (m_RoughnessMapTask)
        {
            delete m_RoughnessMapTask;
            m_RoughnessMapTask = nullptr;
        }

        if (m_MetallicMapTask)
        {
            delete m_MetallicMapTask;
            m_MetallicMapTask = nullptr;
        }
    }

    Result<MeshPartDescriptor> UFbxLoader::LoadMeshData(const ufbx_node* node, uint32_t meshIndex)
    {
        m_Vertices.clear();
        m_Indeces.clear();

        MeshPartDescriptor meshPartDescriptor{};

        ufbx_mesh& fbxMesh = *node->mesh; // mesh for this node, contains submeshes

        uint32_t numSubMeshes = fbxMesh.material_parts.count;
        if (numSubMeshes)
        {
            meshPartDescriptor.debugName = _strdup(node->name.data);
            meshPartDescriptor.subMeshes.resize(numSubMeshes);
            for (uint32_t subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
            {
                auto result = LoadSubMeshVertexData(node, meshIndex, subMeshIndex);

                if (result.IsOk())
                {
                    meshPartDescriptor.subMeshes[subMeshIndex] = std::move(result.Unwrap());
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

            auto vertexBuffer = ResourceManager::Instance->CreateBuffer({
                .debugName = "fbx-mesh-vertex-buffer",
                .usage = BufferUsage::VERTEX,
                .memoryUsage = MemoryUsage::GPU_ONLY,
                .byteSize = (uint32_t)(m_Vertices.size() * sizeof(Vertex)),
                .initialData = (void*)m_Vertices.data(),
            });

            meshPartDescriptor.vertexBuffers = { vertexBuffer };

            if (m_Indeces.size() > 0)
            {
                meshPartDescriptor.indexBuffer = ResourceManager::Instance->CreateBuffer({
                    .debugName = "fbx-mesh-index-buffer",
                    .usage = BufferUsage::INDEX,
                    .memoryUsage = MemoryUsage::GPU_ONLY,
                    .byteSize = (uint32_t)(m_Indeces.size() * sizeof(uint32_t)),
                    .initialData = (void*)m_Indeces.data(),
                });
            }
        }

        return meshPartDescriptor;
    }

    Result<SubMeshDescriptor> UFbxLoader::LoadSubMeshVertexData(const ufbx_node* node, uint32_t meshIndex, uint32_t subMeshIndex)
    {
        SubMeshDescriptor subMeshDescriptor{};
        subMeshDescriptor.debugName = node->materials.count > 0 ? _strdup(node->materials[subMeshIndex]->name.data) : _strdup(node->name.data);
        subMeshDescriptor.minVertex = { (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)() };
        subMeshDescriptor.maxVertex = { (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)() };

        ufbx_mesh& fbxMesh = *node->mesh; // mesh for this node, contains submeshes
        const ufbx_mesh_part& fbxSubmesh = node->mesh->material_parts[subMeshIndex];
        size_t numFaces = fbxSubmesh.num_faces;

        if (node->mesh->materials.count > 0)
        {
            subMeshDescriptor.embededMaterial = m_MaterialNameToHandle[node->mesh->materials.data[subMeshIndex]->name.data];
        }

        if (!(fbxSubmesh.num_triangles))
        {
            return Error("UFbxLoader::LoadSubMeshVertexData: only triangle meshes are supported");
        }

        size_t numVerticesBefore = m_Vertices.size();
        size_t numIndicesBefore = m_Indeces.size();

        subMeshDescriptor.vertexOffset = numVerticesBefore;
        subMeshDescriptor.indexOffset = numIndicesBefore;
        subMeshDescriptor.vertexCount = 0;
        subMeshDescriptor.indexCount = 0;

        glm::vec4 diffuseColor;
        {
            ufbx_material_map baseColorMap{ .has_value = false };

            if (node->materials.count > 0)
            {
                baseColorMap = node->materials[subMeshIndex]->pbr.base_color;
            }

            diffuseColor = baseColorMap.has_value ? glm::vec4(baseColorMap.value_vec4.x, baseColorMap.value_vec4.y,
                baseColorMap.value_vec4.z, baseColorMap.value_vec4.w)
                : glm::vec4(1.0f);
        }

        bool hasTangents = fbxMesh.vertex_tangent.exists;
        bool hasUVs = fbxMesh.uv_sets.count;
        bool hasVertexColors = fbxMesh.vertex_color.exists;

        ufbx_skin_deformer* fbxSkin = nullptr;
        if (fbxMesh.skin_deformers.count)
        {
            fbxSkin = fbxMesh.skin_deformers.data[0];
        }

        for (size_t fbxFaceIndex = 0; fbxFaceIndex < numFaces; ++fbxFaceIndex)
        {
            ufbx_face& fbxFace = fbxMesh.faces[fbxSubmesh.face_indices.data[fbxFaceIndex]];
            size_t numTriangleIndices = fbxMesh.max_face_triangles * 3;
            std::vector<uint32_t> verticesPerFaceIndexBuffer(numTriangleIndices);
            size_t numTriangles = ufbx_triangulate_face(verticesPerFaceIndexBuffer.data(), numTriangleIndices, &fbxMesh, fbxFace);
            size_t numVerticesPerFace = 3 * numTriangles;

            for (uint32_t vertexPerFace = 0; vertexPerFace < numVerticesPerFace; ++vertexPerFace)
            {
                // if the face is a quad, then 2 triangles, numVerticesPerFace = 6
                uint32_t vertexPerFaceIndex = verticesPerFaceIndexBuffer[vertexPerFace];

                Vertex vertex{};

                // position
                uint32_t fbxVertexIndex = fbxMesh.vertex_indices[vertexPerFaceIndex];
                {
                    ufbx_vec3& positionFbx = fbxMesh.vertices[fbxVertexIndex];
                    vertex.Position = glm::vec3(positionFbx.x, positionFbx.y, positionFbx.z);

                    subMeshDescriptor.minVertex.x = glm::min(vertex.Position.x, subMeshDescriptor.minVertex.x);
                    subMeshDescriptor.minVertex.y = glm::min(vertex.Position.y, subMeshDescriptor.minVertex.y);
                    subMeshDescriptor.minVertex.z = glm::min(vertex.Position.z, subMeshDescriptor.minVertex.z);

                    subMeshDescriptor.maxVertex.x = glm::max(vertex.Position.x, subMeshDescriptor.maxVertex.x);
                    subMeshDescriptor.maxVertex.y = glm::max(vertex.Position.y, subMeshDescriptor.maxVertex.y);
                    subMeshDescriptor.maxVertex.z = glm::max(vertex.Position.z, subMeshDescriptor.maxVertex.z);
                }

                // normals, always defined if `ufbx_load_opts.generate_missing_normals` is used
                {
                    uint32_t fbxNormalIndex = fbxMesh.vertex_normal.indices[vertexPerFaceIndex];
                    HBL2_CORE_ASSERT(fbxNormalIndex < fbxMesh.vertex_normal.values.count, "LoadSubMeshVertexData: memory violation normals");
                    ufbx_vec3& normalFbx = fbxMesh.vertex_normal.values.data[fbxNormalIndex];
                    vertex.Normal = glm::vec3(normalFbx.x, normalFbx.y, normalFbx.z);
                }

                //// tangents (check `tangent space` in Blender when exporting fbx)
                //if (hasTangents)
                //{
                //    uint32_t fbxTangentIndex = fbxMesh.vertex_tangent.indices[vertexPerFaceIndex];
                //    HBL2_CORE_ASSERT(fbxTangentIndex < fbxMesh.vertex_tangent.values.count, "LoadSubMeshVertexData: memory violation tangents");
                //    ufbx_vec3& tangentFbx = fbxMesh.vertex_tangent.values.data[fbxTangentIndex];
                //    vertex.Tangent = glm::vec3(tangentFbx.x, tangentFbx.y, tangentFbx.z);
                //}

                // uv coordinates
                if (hasUVs)
                {
                    uint32_t fbxUVIndex = fbxMesh.vertex_uv.indices[vertexPerFaceIndex];
                    HBL2_CORE_ASSERT(fbxUVIndex < fbxMesh.vertex_uv.values.count, "LoadSubMeshVertexData: memory violation uv coordinates");
                    ufbx_vec2& uvFbx = fbxMesh.vertex_uv.values.data[fbxUVIndex];
                    vertex.UV = glm::vec2(uvFbx.x, uvFbx.y);
                }

                //// vertex colors
                //if (hasVertexColors)
                //{
                //    uint32_t fbxColorIndex = fbxMesh.vertex_color.indices[vertexPerFaceIndex];
                //    ufbx_vec4& colorFbx = fbxMesh.vertex_color.values.data[fbxColorIndex];

                //    // convert from sRGB to linear
                //    glm::vec3 linearColor = glm::pow(glm::vec3(colorFbx.x, colorFbx.y, colorFbx.z), glm::vec3(2.2f));
                //    glm::vec4 vertexColor(linearColor.x, linearColor.y, linearColor.z, colorFbx.w);
                //    vertex.Color = vertexColor * diffuseColor;
                //}
                //else
                //{
                //    vertex.Color = diffuseColor;
                //}

                m_Vertices.push_back(vertex);
            }
        }

        // resolve indices
        // A face has four vertices, while above loop generates at least six vertices for per face)
        {
            // get number of all vertices created from above (faces * trianglesPerFace * 3)
            uint32_t submeshAllVertices = m_Vertices.size() - numVerticesBefore;

            // create a ufbx vertex stream with data pointing to the first vertex of this submesh
            // (meshData.VertexBuffer is for all submeshes)
            ufbx_vertex_stream streams;
            streams.data = &m_Vertices[numVerticesBefore];
            streams.vertex_count = submeshAllVertices;
            streams.vertex_size = sizeof(Vertex);

            // index buffer: add space for all new vertices from above
            m_Indeces.resize(numIndicesBefore + submeshAllVertices);

            // ufbx_generate_indices() will rearrange meshData.VertexBuffer (via streams.data) and fill meshData.IndexBuffer
            ufbx_error ufbxError;
            size_t numVertices = ufbx_generate_indices(&streams, 1, &m_Indeces[numIndicesBefore], submeshAllVertices, nullptr, &ufbxError);

            // handle error
            if (ufbxError.type != UFBX_ERROR_NONE)
            {
                char errorBuffer[512];
                ufbx_format_error(errorBuffer, sizeof(errorBuffer), &ufbxError);
                return Error(std::format("UFbxLoader::LoadSubMeshVertexData: creation of index buffer failed, error: {},  node: {}", errorBuffer, node->name.data));
            }

            // meshData.VertexBuffer can be downsized now
            m_Vertices.resize(numVerticesBefore + numVertices);
            subMeshDescriptor.vertexCount = numVertices;
            subMeshDescriptor.indexCount = submeshAllVertices;
        }

        return subMeshDescriptor;
    }
}
