#include "UFbxLoader.h"

#include <Resources\ResourceManager.h>

namespace HBL2
{
    Handle<Mesh> UFbxLoader::Load(const std::filesystem::path& path)
	{
        HBL2_FUNC_PROFILE();

        ufbx_load_opts loadOptions{};
        loadOptions.generate_missing_normals = true;
        ufbx_error error;
        m_Scene = ufbx_load_file(path.string().c_str(), &loadOptions, &error);
        if (!m_Scene)
        {
            HBL2_CORE_ERROR("Failed to load: {0}\n", error.description.data);
            ufbx_free_scene(m_Scene);
            return Handle<Mesh>();
        }

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
            .meshes = meshes,
        });

        return handle;
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

        return Ok(meshPartDescriptor);
    }

    Result<SubMeshDescriptor> UFbxLoader::LoadSubMeshVertexData(const ufbx_node* node, uint32_t meshIndex, uint32_t subMeshIndex)
    {
        SubMeshDescriptor subMeshDescriptor{};
        subMeshDescriptor.debugName = _strdup(node->name.data);
        subMeshDescriptor.minVertex = { (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)() };
        subMeshDescriptor.maxVertex = { (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)() };

        ufbx_mesh& fbxMesh = *node->mesh; // mesh for this node, contains submeshes
        const ufbx_mesh_part& fbxSubmesh = node->mesh->material_parts[subMeshIndex];
        size_t numFaces = fbxSubmesh.num_faces;

        if (!(fbxSubmesh.num_triangles))
        {
            HBL2_CORE_ERROR("UFbxLoader::LoadVertexData: only triangle meshes are supported");
            return Error("UFbxLoader::LoadVertexData: only triangle meshes are supported");
        }

        size_t numVerticesBefore = m_Vertices.size();
        size_t numIndicesBefore = m_Indeces.size();

        subMeshDescriptor.vertexOffset = numVerticesBefore;
        subMeshDescriptor.indexOffset = numIndicesBefore;
        subMeshDescriptor.vertexCount = 0;
        subMeshDescriptor.indexCount = 0;

        glm::vec4 diffuseColor;
        {
            ufbx_material_map& baseColorMap = node->materials[subMeshIndex]->pbr.base_color;
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
                    HBL2_CORE_ASSERT(fbxNormalIndex < fbxMesh.vertex_normal.values.count, "LoadVertexData: memory violation normals");
                    ufbx_vec3& normalFbx = fbxMesh.vertex_normal.values.data[fbxNormalIndex];
                    vertex.Normal = glm::vec3(normalFbx.x, normalFbx.y, normalFbx.z);
                }

                //// tangents (check `tangent space` in Blender when exporting fbx)
                //if (hasTangents)
                //{
                //    uint32_t fbxTangentIndex = fbxMesh.vertex_tangent.indices[vertexPerFaceIndex];
                //    HBL2_CORE_ASSERT(fbxTangentIndex < fbxMesh.vertex_tangent.values.count, "LoadVertexData: memory violation tangents");
                //    ufbx_vec3& tangentFbx = fbxMesh.vertex_tangent.values.data[fbxTangentIndex];
                //    vertex.Tangent = glm::vec3(tangentFbx.x, tangentFbx.y, tangentFbx.z);
                //}

                // uv coordinates
                if (hasUVs)
                {
                    uint32_t fbxUVIndex = fbxMesh.vertex_uv.indices[vertexPerFaceIndex];
                    HBL2_CORE_ASSERT(fbxUVIndex < fbxMesh.vertex_uv.values.count, "LoadVertexData: memory violation uv coordinates");
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
                const auto& errorMsg = std::format("UFbxLoader: creation of index buffer failed, error: {},  node: {}", errorBuffer, node->name.data);
                HBL2_CORE_FATAL(errorMsg);
                return Error(errorMsg);
            }

            // meshData.VertexBuffer can be downsized now
            m_Vertices.resize(numVerticesBefore + numVertices);
            subMeshDescriptor.vertexCount = numVertices;
            subMeshDescriptor.indexCount = submeshAllVertices;
        }

        return Ok(subMeshDescriptor);
    }
}
