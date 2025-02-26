#include "UFbxLoader.h"

#include "Utilities\MeshUtilities.h"

namespace HBL2
{
	bool UFbxLoader::Load(const std::filesystem::path& path, MeshData& meshData)
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
            return false;
        }

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
                bool result = LoadVertexData(node, meshData);
                if (result == false)
                {
                    ufbx_free_scene(m_Scene);
                    return false;
                }
            }
        }

        ufbx_free_scene(m_Scene);
        return true;
	}

    bool UFbxLoader::LoadVertexData(const ufbx_node* node, MeshData& meshData)
    {
        int submeshIndex = 0;

        ufbx_mesh& fbxMesh = *node->mesh; // mesh for this node, contains submeshes
        const ufbx_mesh_part& fbxSubmesh = node->mesh->material_parts[submeshIndex];
        size_t numFaces = fbxSubmesh.num_faces;

        if (!(fbxSubmesh.num_triangles))
        {
            HBL2_CORE_ERROR("UFbxLoader::LoadVertexData: only triangle meshes are supported");
            return false;
        }

        size_t numVerticesBefore = meshData.VertexBuffer.size();
        size_t numIndicesBefore = meshData.IndexBuffer.size();

        glm::vec4 diffuseColor;
        {
            ufbx_material_map& baseColorMap = node->materials[submeshIndex]->pbr.base_color;
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

        glm::vec3 minVertex = { (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)() };
        glm::vec3 maxVertex = { (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)() };

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

                    minVertex.x = glm::min(vertex.Position.x, minVertex.x);
                    minVertex.y = glm::min(vertex.Position.y, minVertex.y);
                    minVertex.z = glm::min(vertex.Position.z, minVertex.z);

                    maxVertex.x = glm::max(vertex.Position.x, maxVertex.x);
                    maxVertex.y = glm::max(vertex.Position.y, maxVertex.y);
                    maxVertex.z = glm::max(vertex.Position.z, maxVertex.z);
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

                meshData.VertexBuffer.push_back(vertex);
            }

            meshData.MeshExtents = { minVertex, maxVertex };
        }

        // resolve indices
        // A face has four vertices, while above loop generates at least six vertices for per face)
        {
            // get number of all vertices created from above (faces * trianglesPerFace * 3)
            uint32_t submeshAllVertices = meshData.VertexBuffer.size() - numVerticesBefore;

            // create a ufbx vertex stream with data pointing to the first vertex of this submesh
            // (meshData.VertexBuffer is for all submeshes)
            ufbx_vertex_stream streams;
            streams.data = &meshData.VertexBuffer[numVerticesBefore];
            streams.vertex_count = submeshAllVertices;
            streams.vertex_size = sizeof(Vertex);

            // index buffer: add space for all new vertices from above
            meshData.IndexBuffer.resize(numIndicesBefore + submeshAllVertices);

            // ufbx_generate_indices() will rearrange meshData.VertexBuffer (via streams.data) and fill meshData.IndexBuffer
            ufbx_error ufbxError;
            size_t numVertices = ufbx_generate_indices(&streams, 1, &meshData.IndexBuffer[numIndicesBefore], submeshAllVertices, nullptr, &ufbxError);

            // handle error
            if (ufbxError.type != UFBX_ERROR_NONE)
            {
                char errorBuffer[512];
                ufbx_format_error(errorBuffer, sizeof(errorBuffer), &ufbxError);
                HBL2_CORE_FATAL("UFbxLoader: creation of index buffer failed, error: {0},  node: {1}", errorBuffer, node->name.data);
            }

            // meshData.VertexBuffer can be downsized now
            meshData.VertexBuffer.resize(numVerticesBefore + numVertices);
        }

        return true;
    }
}
