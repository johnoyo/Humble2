#include "UFbxLoader.h"

namespace HBL2
{
	bool UFbxLoader::Load(const std::filesystem::path& path, std::vector<Vertex>& meshData)
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

    bool UFbxLoader::LoadVertexData(const ufbx_node* node, std::vector<Vertex>& meshData)
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
                }

                // normals, always defined if `ufbx_load_opts.generate_missing_normals` is used
                {
                    uint32_t fbxNormalIndex = fbxMesh.vertex_normal.indices[vertexPerFaceIndex];
                    HBL2_CORE_ASSERT(fbxNormalIndex < fbxMesh.vertex_normal.values.count, "LoadVertexData: memory violation normals");
                    ufbx_vec3& normalFbx = fbxMesh.vertex_normal.values.data[fbxNormalIndex];
                    vertex.Normal = glm::vec3(normalFbx.x, normalFbx.y, normalFbx.z);
                }

                // tangents (check `tangent space` in Blender when exporting fbx)
                if (hasTangents)
                {
                    uint32_t fbxTangentIndex = fbxMesh.vertex_tangent.indices[vertexPerFaceIndex];
                    HBL2_CORE_ASSERT(fbxTangentIndex < fbxMesh.vertex_tangent.values.count, "LoadVertexData: memory violation tangents");
                    ufbx_vec3& tangentFbx = fbxMesh.vertex_tangent.values.data[fbxTangentIndex];
                    vertex.Tangent = glm::vec3(tangentFbx.x, tangentFbx.y, tangentFbx.z);
                }

                // uv coordinates
                if (hasUVs)
                {
                    uint32_t fbxUVIndex = fbxMesh.vertex_uv.indices[vertexPerFaceIndex];
                    HBL2_CORE_ASSERT(fbxUVIndex < fbxMesh.vertex_uv.values.count, "LoadVertexData: memory violation uv coordinates");
                    ufbx_vec2& uvFbx = fbxMesh.vertex_uv.values.data[fbxUVIndex];
                    vertex.UV = glm::vec2(uvFbx.x, uvFbx.y);
                }

                // vertex colors
                if (hasVertexColors)
                {
                    uint32_t fbxColorIndex = fbxMesh.vertex_color.indices[vertexPerFaceIndex];
                    ufbx_vec4& colorFbx = fbxMesh.vertex_color.values.data[fbxColorIndex];

                    // convert from sRGB to linear
                    glm::vec3 linearColor = glm::pow(glm::vec3(colorFbx.x, colorFbx.y, colorFbx.z), glm::vec3(2.2f));
                    glm::vec4 vertexColor(linearColor.x, linearColor.y, linearColor.z, colorFbx.w);
                    vertex.Color = vertexColor * diffuseColor;
                }
                else
                {
                    vertex.Color = diffuseColor;
                }

                meshData.push_back(vertex);
            }
        }

        return true;
    }
}