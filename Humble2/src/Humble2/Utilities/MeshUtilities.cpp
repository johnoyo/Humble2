#include "MeshUtilities.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Utilities/tiny_obj_loader.h"

namespace HBL2
{
    MeshData MeshUtilities::Load(const std::string& path)
    {
		MeshData meshData;

		//attrib will contain the vertex arrays of the file
		tinyobj::attrib_t attrib;
		//shapes contains the info for each separate object in the file
		std::vector<tinyobj::shape_t> shapes;
		//materials contains the information about the material of each shape, but we won't use it.
		std::vector<tinyobj::material_t> materials;

		//error and warning output from the load function
		std::string warn;
		std::string err;

		//load the OBJ file
		bool result = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), std::filesystem::path(path).parent_path().string().c_str());

		//make sure to output the warnings to the console, in case there are issues with the file
		if (!warn.empty())
		{
			HBL2_CORE_WARN("WARN: {0}", warn);
		}

		//if we have any error, print it to the console, and break the mesh loading.
		//This happens if the file can't be found or is malformed
		if (!err.empty())
		{
			HBL2_CORE_ERROR("ERROR: {0}", err);
			meshData.Result = false;
			return meshData;
		}

		if (!result)
		{
			meshData.Result = false;
			return meshData;
		}

		for (int i = 0; i < shapes.size(); i++)
		{
			tinyobj::shape_t& shape = shapes[i];
			tinyobj::mesh_t& m = shape.mesh;

			for (int j = 0; j < m.indices.size(); j++)
			{
				tinyobj::index_t i = m.indices[j];
				glm::vec3 position =
				{
					attrib.vertices[i.vertex_index * 3],
					attrib.vertices[i.vertex_index * 3 + 1],
					attrib.vertices[i.vertex_index * 3 + 2]
				};
				glm::vec3 normal =
				{
					attrib.normals[i.normal_index * 3],
					attrib.normals[i.normal_index * 3 + 1],
					attrib.normals[i.normal_index * 3 + 2]
				};
				glm::vec2 texCoord =
				{
					attrib.texcoords[i.texcoord_index * 2],
					attrib.texcoords[i.texcoord_index * 2 + 1],
				};

				//copy it into our vertex
				MeshData::Vertex new_vert;
				new_vert.Position = position;
				new_vert.Normal = normal;
				new_vert.TextureCoord = texCoord;

				meshData.Data.push_back(new_vert);
			}
		}

		meshData.Result = true;
		return meshData;
    }
}

